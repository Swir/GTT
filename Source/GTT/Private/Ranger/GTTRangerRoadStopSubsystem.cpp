#include "Ranger/GTTRangerRoadStopSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Ranger/GTTRangerAIController.h"

namespace
{
FVector ResolveRoadForward(const APawn* Target)
{
    if (!Target)
    {
        return FVector::ForwardVector;
    }

    FVector Forward = Target->GetVelocity().GetSafeNormal2D();
    if (Forward.IsNearlyZero())
    {
        Forward = Target->GetActorForwardVector().GetSafeNormal2D();
    }
    return Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
}
}

bool UGTTRangerRoadStopSubsystem::BeginStop(
    AGTTRangerAIController* Controller,
    APawn* Target,
    const FVector& RangerLocation,
    float GraceSeconds)
{
    if (!Controller || !Target)
    {
        return false;
    }

    const bool bExistingLiveStop =
        (Phase == EGTTRangerRoadStopPhase::Comply ||
         Phase == EGTTRangerRoadStopPhase::Search ||
         Phase == EGTTRangerRoadStopPhase::Flee) &&
        ActiveController.IsValid() && ActiveTarget.IsValid();
    if (bExistingLiveStop)
    {
        // COMPLY/SEARCH belongs to one primary ranger. FLEE remains latched for
        // the wildlife incident so reinforcement cannot immediately re-stop or
        // proximity-cite a driver that already accepted the police escalation.
        return ActiveController.Get() == Controller && Phase != EGTTRangerRoadStopPhase::Flee;
    }

    ActiveController = Controller;
    ActiveTarget = Target;
    Phase = EGTTRangerRoadStopPhase::Comply;
    RemainingSeconds = FMath::Max(0.0f, GraceSeconds);
    InitialGraceSeconds = RemainingSeconds;
    SearchHoldElapsed = 0.0f;
    SearchHoldRequired = 0.0f;
    ObservedTargetSpeedKmh = 0.0f;
    ComplianceSpeedLimitKmh = 2.5f;
    FleeDisplayUntilSeconds = 0.0f;
    RefreshRoadFrame(Target);

    const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadForward).GetSafeNormal2D();
    const float RangerSide = FVector::DotProduct(RangerLocation - StopLocation, RoadRight);
    ShoulderSide = RangerSide < 0.0f ? -1.0f : 1.0f;
    return true;
}

void UGTTRangerRoadStopSubsystem::UpdateStop(
    AGTTRangerAIController* Controller,
    APawn* Target,
    float SecondsRemaining,
    float HoldElapsed,
    float HoldRequired,
    float TargetSpeedKmh,
    bool bSearching,
    float InComplianceSpeedLimitKmh)
{
    if (!IsOwnedBy(Controller) || !Target || ActiveTarget.Get() != Target)
    {
        return;
    }

    RefreshRoadFrame(Target);
    RemainingSeconds = FMath::Max(0.0f, SecondsRemaining);
    SearchHoldElapsed = FMath::Max(0.0f, HoldElapsed);
    SearchHoldRequired = FMath::Max(0.0f, HoldRequired);
    ObservedTargetSpeedKmh = FMath::Max(0.0f, TargetSpeedKmh);
    ComplianceSpeedLimitKmh = FMath::Max(0.1f, InComplianceSpeedLimitKmh);
    Phase = bSearching ? EGTTRangerRoadStopPhase::Search : EGTTRangerRoadStopPhase::Comply;
}

void UGTTRangerRoadStopSubsystem::MarkFlee(AGTTRangerAIController* Controller, float DisplaySeconds)
{
    if (!IsOwnedBy(Controller))
    {
        return;
    }

    Phase = EGTTRangerRoadStopPhase::Flee;
    RemainingSeconds = 0.0f;
    SearchHoldElapsed = 0.0f;
    FleeDisplayUntilSeconds = GetWorld()
        ? GetWorld()->GetTimeSeconds() + FMath::Max(0.0f, DisplaySeconds)
        : 0.0f;
}

void UGTTRangerRoadStopSubsystem::EndStop(AGTTRangerAIController* Controller)
{
    if (!Controller || IsOwnedBy(Controller))
    {
        ClearStop();
    }
}

bool UGTTRangerRoadStopSubsystem::IsOwnedBy(const AGTTRangerAIController* Controller) const
{
    return Controller && ActiveController.Get() == Controller;
}

bool UGTTRangerRoadStopSubsystem::IsStopForTarget(const APawn* Target) const
{
    if (!Target || ActiveTarget.Get() != Target || !ActiveController.IsValid())
    {
        return false;
    }

    return Phase == EGTTRangerRoadStopPhase::Comply ||
        Phase == EGTTRangerRoadStopPhase::Search ||
        Phase == EGTTRangerRoadStopPhase::Flee;
}

bool UGTTRangerRoadStopSubsystem::HasTrafficControl() const
{
    return ActiveController.IsValid() && ActiveTarget.IsValid() &&
        (Phase == EGTTRangerRoadStopPhase::Comply || Phase == EGTTRangerRoadStopPhase::Search);
}

FVector UGTTRangerRoadStopSubsystem::GetRangerStagingPoint(float LateralOffsetCm, float RearOffsetCm) const
{
    const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadForward).GetSafeNormal2D();
    return StopLocation + RoadRight * ShoulderSide * FMath::Max(0.0f, LateralOffsetCm)
        - RoadForward * FMath::Max(0.0f, RearOffsetCm);
}

FVector UGTTRangerRoadStopSubsystem::GetRangerSupportPoint(float LateralOffsetCm, float RearOffsetCm) const
{
    const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadForward).GetSafeNormal2D();
    return StopLocation + RoadRight * ShoulderSide * FMath::Max(0.0f, LateralOffsetCm)
        - RoadForward * FMath::Max(0.0f, RearOffsetCm + 260.0f);
}

bool UGTTRangerRoadStopSubsystem::GetTrafficResponse(
    const FVector& VehicleLocation,
    const FVector& VehicleForward,
    float& OutSpeedScale,
    bool& bOutHold) const
{
    OutSpeedScale = 1.0f;
    bOutHold = false;
    if (!HasTrafficControl())
    {
        return false;
    }

    FVector ToStop = StopLocation - VehicleLocation;
    ToStop.Z = 0.0f;
    const float DistanceCm = ToStop.Size();
    if (DistanceCm > TrafficSlowRadiusCm || DistanceCm < KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const FVector VehicleHeading = VehicleForward.GetSafeNormal2D();
    const float ApproachDot = FVector::DotProduct(VehicleHeading, ToStop / DistanceCm);
    if (ApproachDot <= 0.20f)
    {
        return false;
    }

    // Only the lane travelling in the same direction as the stopped player yields.
    // This prevents an oncoming vehicle on the opposite lane from being frozen just
    // because it is physically approaching the same roadside contact point.
    const float SameDirectionDot = FVector::DotProduct(VehicleHeading, RoadForward);
    if (SameDirectionDot <= TrafficSameDirectionDot)
    {
        return false;
    }

    const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadForward).GetSafeNormal2D();
    const float LateralDistanceCm = FMath::Abs(FVector::DotProduct(VehicleLocation - StopLocation, RoadRight));
    if (LateralDistanceCm > TrafficCorridorHalfWidthCm)
    {
        return false;
    }

    const float HoldRadius = Phase == EGTTRangerRoadStopPhase::Search
        ? TrafficSearchHoldRadiusCm
        : TrafficHoldRadiusCm;
    bOutHold = DistanceCm <= HoldRadius;
    if (bOutHold)
    {
        OutSpeedScale = 0.0f;
        return true;
    }

    const float Alpha = FMath::Clamp(
        (DistanceCm - HoldRadius) / FMath::Max(1.0f, TrafficSlowRadiusCm - HoldRadius),
        0.0f,
        1.0f);
    OutSpeedScale = FMath::Lerp(0.22f, 0.82f, Alpha);
    return true;
}

bool UGTTRangerRoadStopSubsystem::GetCivilianResponse(
    const FVector& CitizenLocation,
    FVector& OutSafeLocation,
    FVector& OutFocusLocation,
    bool& bOutNeedsMove) const
{
    OutSafeLocation = CitizenLocation;
    OutFocusLocation = StopLocation;
    bOutNeedsMove = false;
    if (!HasTrafficControl())
    {
        return false;
    }

    FVector Relative = CitizenLocation - StopLocation;
    Relative.Z = 0.0f;
    if (Relative.SizeSquared2D() > FMath::Square(CivilianAwarenessRadiusCm))
    {
        return false;
    }

    const FVector RoadRight = FVector::CrossProduct(FVector::UpVector, RoadForward).GetSafeNormal2D();
    const float LateralCm = FVector::DotProduct(Relative, RoadRight);
    const float LongitudinalCm = FVector::DotProduct(Relative, RoadForward);
    if (FMath::Abs(LongitudinalCm) > CivilianLongitudinalWindowCm)
    {
        return false;
    }

    const float PreferredSide = FMath::Abs(LateralCm) > 80.0f
        ? (LateralCm < 0.0f ? -1.0f : 1.0f)
        : -ShoulderSide;
    const float SafeLongitudinal = FMath::Clamp(LongitudinalCm, -900.0f, 900.0f);
    OutSafeLocation = StopLocation
        + RoadRight * PreferredSide * CivilianSafeLateralCm
        + RoadForward * SafeLongitudinal;
    OutSafeLocation.Z = CitizenLocation.Z;
    bOutNeedsMove = FMath::Abs(LateralCm) < CivilianMoveThresholdCm;
    return true;
}

FGTTRangerRoadStopPresentation UGTTRangerRoadStopSubsystem::GetPresentationSnapshot() const
{
    FGTTRangerRoadStopPresentation Snapshot;
    Snapshot.Phase = Phase;
    Snapshot.SecondsRemaining = RemainingSeconds;
    Snapshot.ObservedSpeedKmh = ObservedTargetSpeedKmh;

    switch (Phase)
    {
    case EGTTRangerRoadStopPhase::Comply:
        if (!ActiveController.IsValid() || !ActiveTarget.IsValid())
        {
            return Snapshot;
        }
        Snapshot.bVisible = true;
        Snapshot.PhaseLabel = TEXT("COMPLY");
        Snapshot.Progress01 = InitialGraceSeconds > KINDA_SMALL_NUMBER
            ? FMath::Clamp(1.0f - RemainingSeconds / InitialGraceSeconds, 0.0f, 1.0f)
            : 1.0f;
        Snapshot.Instruction = ObservedTargetSpeedKmh > ComplianceSpeedLimitKmh
            ? FString::Printf(TEXT("PULL OVER | SLOW BELOW %.1f KM/H"), ComplianceSpeedLimitKmh)
            : TEXT("HOLD POSITION | WAIT FOR WARDEN");
        break;

    case EGTTRangerRoadStopPhase::Search:
        if (!ActiveController.IsValid() || !ActiveTarget.IsValid())
        {
            return Snapshot;
        }
        Snapshot.bVisible = true;
        Snapshot.PhaseLabel = TEXT("SEARCH");
        Snapshot.Progress01 = SearchHoldRequired > KINDA_SMALL_NUMBER
            ? FMath::Clamp(SearchHoldElapsed / SearchHoldRequired, 0.0f, 1.0f)
            : 0.0f;
        Snapshot.Instruction = TEXT("REMAIN STOPPED | VEHICLE SEARCH IN PROGRESS");
        break;

    case EGTTRangerRoadStopPhase::Flee:
        Snapshot.bVisible = IsFleeDisplayVisible();
        Snapshot.PhaseLabel = TEXT("FLEE");
        Snapshot.Progress01 = 1.0f;
        Snapshot.Instruction = TEXT("STOP FAILED | POLICE ESCALATION ACTIVE");
        break;

    default:
        break;
    }

    return Snapshot;
}

FString UGTTRangerRoadStopSubsystem::GetStatusText() const
{
    switch (Phase)
    {
    case EGTTRangerRoadStopPhase::Comply:
        if (ActiveController.IsValid() && ActiveTarget.IsValid())
        {
            return FString::Printf(TEXT("WARDEN STOP | COMPLY %.1fs | SPEED %.1f km/h"),
                RemainingSeconds, ObservedTargetSpeedKmh);
        }
        break;
    case EGTTRangerRoadStopPhase::Search:
        if (ActiveController.IsValid() && ActiveTarget.IsValid())
        {
            return FString::Printf(TEXT("WARDEN STOP | SEARCH %.1f/%.1fs | HOLD"),
                SearchHoldElapsed, SearchHoldRequired);
        }
        break;
    case EGTTRangerRoadStopPhase::Flee:
        if (IsFleeDisplayVisible())
        {
            return TEXT("WARDEN STOP | FLEE | POLICE ESCALATION");
        }
        break;
    default:
        break;
    }
    return FString();
}

void UGTTRangerRoadStopSubsystem::RefreshRoadFrame(APawn* Target)
{
    if (!Target)
    {
        return;
    }
    StopLocation = Target->GetActorLocation();
    RoadForward = ResolveRoadForward(Target);
}

bool UGTTRangerRoadStopSubsystem::IsFleeDisplayVisible() const
{
    return GetWorld() && GetWorld()->GetTimeSeconds() <= FleeDisplayUntilSeconds;
}

void UGTTRangerRoadStopSubsystem::ClearStop()
{
    ActiveController.Reset();
    ActiveTarget.Reset();
    Phase = EGTTRangerRoadStopPhase::None;
    StopLocation = FVector::ZeroVector;
    RoadForward = FVector::ForwardVector;
    ShoulderSide = 1.0f;
    RemainingSeconds = 0.0f;
    InitialGraceSeconds = 0.0f;
    SearchHoldElapsed = 0.0f;
    SearchHoldRequired = 0.0f;
    ObservedTargetSpeedKmh = 0.0f;
    ComplianceSpeedLimitKmh = 2.5f;
    FleeDisplayUntilSeconds = 0.0f;
}
