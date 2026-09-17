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
    SearchHoldElapsed = 0.0f;
    SearchHoldRequired = 0.0f;
    ObservedTargetSpeedKmh = 0.0f;
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
    bool bSearching)
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
    SearchHoldElapsed = 0.0f;
    SearchHoldRequired = 0.0f;
    ObservedTargetSpeedKmh = 0.0f;
    FleeDisplayUntilSeconds = 0.0f;
}
