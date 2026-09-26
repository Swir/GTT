#include "Core/GTTDrivetrainEvidenceScenarioSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr float StartDelaySeconds = 76.0f;
    constexpr float GlobalDeadlineSeconds = 122.0f;
    constexpr float ForwardAccelerationTimeoutSeconds = 16.0f;
    constexpr float ReverseAccelerationTimeoutSeconds = 12.0f;
    constexpr float ForwardReturnTimeoutSeconds = 10.0f;
    constexpr float ShiftReleaseSpeedKmh = 3.5f;
    constexpr float SafeShiftEvidenceToleranceKmh = 0.25f;
    constexpr float ForwardEvidenceSpeedKmh = 6.0f;
    constexpr float ReverseEvidenceSpeedKmh = 5.0f;
    constexpr float ForwardReturnEvidenceSpeedKmh = 2.0f;
    constexpr float ForwardThrottle = 0.92f;
    constexpr float ReverseThrottle = -0.72f;
    constexpr float ReturnThrottle = 0.72f;
    constexpr float ShiftBrake = 0.85f;
}

void UGTTDrivetrainEvidenceScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEnabled)
    {
        GTT_LOG( Log,
            TEXT("NATIVE_DRIVETRAIN_SCENARIO_BEGIN version=1 start_delay=%.1f deadline=%.1f release_kmh=%.2f"),
            StartDelaySeconds, GlobalDeadlineSeconds, ShiftReleaseSpeedKmh);
    }
}

TStatId UGTTDrivetrainEvidenceScenarioSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTDrivetrainEvidenceScenarioSubsystem, STATGROUP_Tickables);
}

bool UGTTDrivetrainEvidenceScenarioSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

AGTTFieldmasterNativePawn* UGTTDrivetrainEvidenceScenarioSubsystem::ResolveFieldmaster()
{
    if (Fieldmaster.IsValid()) return Fieldmaster.Get();
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* Candidate = *It;
        if (Candidate && Candidate->IsNativeFieldmasterReady() && Candidate->IsLegacyTakeoverActive())
        {
            Fieldmaster = Candidate;
            return Candidate;
        }
    }
    return nullptr;
}

UChaosWheeledVehicleMovementComponent* UGTTDrivetrainEvidenceScenarioSubsystem::ResolveMovement(AGTTFieldmasterNativePawn* Pawn) const
{
    return Pawn ? Cast<UChaosWheeledVehicleMovementComponent>(Pawn->GetVehicleMovementComponent()) : nullptr;
}

float UGTTDrivetrainEvidenceScenarioSubsystem::GetSignedSpeedKmh(const AGTTFieldmasterNativePawn* Pawn) const
{
    return Pawn ? FVector::DotProduct(Pawn->GetVelocity(), Pawn->GetActorForwardVector()) * 0.036f : 0.0f;
}

void UGTTDrivetrainEvidenceScenarioSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    GTT_LOG( Error,
        TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason, Elapsed);
}

void UGTTDrivetrainEvidenceScenarioSubsystem::BeginForwardAcceleration(
    AGTTFieldmasterNativePawn* Pawn,
    UChaosWheeledVehicleMovementComponent* Movement)
{
    if (!Pawn || !Movement) return;

    Movement->SetBrakeInput(0.0f);
    Movement->SetSteeringInput(0.0f);
    Movement->SetTargetGear(1, true);
    Movement->SetThrottleInput(ForwardThrottle);
    Phase = EDrivetrainEvidencePhase::ForwardAcceleration;
    PhaseStartedSeconds = Elapsed;
    MaxForwardGearObserved = FMath::Max(MaxForwardGearObserved, Movement->GetCurrentGear());

    GTT_LOG( Log,
        TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=FORWARD_ACCELERATE result=START gear=%d signed_speed_kmh=%.2f throttle=%.2f"),
        Movement->GetCurrentGear(), GetSignedSpeedKmh(Pawn), ForwardThrottle);
}

void UGTTDrivetrainEvidenceScenarioSubsystem::CompleteScenario(
    AGTTFieldmasterNativePawn* Pawn,
    UChaosWheeledVehicleMovementComponent* Movement,
    const TCHAR* Reason)
{
    if (Movement)
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetBrakeInput(1.0f);
    }

    const bool bPass = bSequenceHealthy
        && bAutomaticUpshiftObserved
        && bReverseInterlockObserved
        && bSafeReverseCommitObserved
        && bReverseMotionObserved
        && bSafeForwardCommitObserved
        && bForwardReturnObserved;

    GTT_LOG( Log,
        TEXT("NATIVE_DRIVETRAIN_SCENARIO_COMPLETE result=%s route=forward-auto-reverse-forward max_forward_gear=%d reverse_interlock_speed_kmh=%.2f reverse_commit_speed_kmh=%.2f forward_commit_speed_kmh=%.2f final_signed_speed_kmh=%.2f reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), MaxForwardGearObserved, ReverseInterlockStartSpeedKmh,
        ReverseCommitSpeedKmh, ForwardCommitSpeedKmh, GetSignedSpeedKmh(Pawn), Reason, Elapsed);

    Phase = EDrivetrainEvidencePhase::Complete;
    bFinished = true;
}

void UGTTDrivetrainEvidenceScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < StartDelaySeconds) return;

    AGTTFieldmasterNativePawn* Pawn = ResolveFieldmaster();
    UChaosWheeledVehicleMovementComponent* Movement = ResolveMovement(Pawn);
    if (!Pawn || !Movement || !Movement->IsActive())
    {
        if (Elapsed >= GlobalDeadlineSeconds)
        {
            MarkFailure(TEXT("fieldmaster-or-chaos-movement-unavailable"));
            CompleteScenario(Pawn, Movement, TEXT("global-deadline"));
        }
        return;
    }

    if (Elapsed >= GlobalDeadlineSeconds)
    {
        MarkFailure(TEXT("global-sequence-timeout"));
        CompleteScenario(Pawn, Movement, TEXT("global-deadline"));
        return;
    }

    const float SignedSpeedKmh = GetSignedSpeedKmh(Pawn);
    const float AbsoluteSpeedKmh = FMath::Abs(SignedSpeedKmh);
    const int32 CurrentGear = Movement->GetCurrentGear();
    if (CurrentGear > 0) MaxForwardGearObserved = FMath::Max(MaxForwardGearObserved, CurrentGear);

    switch (Phase)
    {
    case EDrivetrainEvidencePhase::Waiting:
        BeginForwardAcceleration(Pawn, Movement);
        break;

    case EDrivetrainEvidencePhase::ForwardAcceleration:
    {
        Movement->SetBrakeInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetThrottleInput(ForwardThrottle);

        if (!bAutomaticUpshiftObserved && CurrentGear >= 2 && SignedSpeedKmh >= ForwardEvidenceSpeedKmh)
        {
            bAutomaticUpshiftObserved = true;
            GTT_LOG( Log,
                TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=AUTOMATIC_UPSHIFT result=PASS gear=%d speed_kmh=%.2f max_forward_gear=%d"),
                CurrentGear, SignedSpeedKmh, MaxForwardGearObserved);
        }

        const bool bReadyToBrake = bAutomaticUpshiftObserved && SignedSpeedKmh >= ForwardEvidenceSpeedKmh;
        const bool bTimedOut = Elapsed - PhaseStartedSeconds >= ForwardAccelerationTimeoutSeconds;
        if (bReadyToBrake || bTimedOut)
        {
            if (!bAutomaticUpshiftObserved)
            {
                GTT_LOG( Log,
                    TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=AUTOMATIC_UPSHIFT result=FAIL gear=%d speed_kmh=%.2f max_forward_gear=%d reason=timeout"),
                    CurrentGear, SignedSpeedKmh, MaxForwardGearObserved);
                MarkFailure(TEXT("automatic-upshift-not-observed"));
            }

            ReverseInterlockStartSpeedKmh = AbsoluteSpeedKmh;
            bReverseInterlockObserved = AbsoluteSpeedKmh > ShiftReleaseSpeedKmh;
            GTT_LOG( Log,
                TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=REVERSE_INTERLOCK result=%s speed_abs_kmh=%.2f gear=%d release_kmh=%.2f action=HOLD_GEAR_AND_BRAKE"),
                bReverseInterlockObserved ? TEXT("PASS") : TEXT("FAIL"), AbsoluteSpeedKmh, CurrentGear, ShiftReleaseSpeedKmh);
            if (!bReverseInterlockObserved) MarkFailure(TEXT("forward-speed-too-low-for-reverse-interlock-evidence"));

            Movement->SetThrottleInput(0.0f);
            Movement->SetBrakeInput(ShiftBrake);
            Movement->SetSteeringInput(0.0f);
            Phase = EDrivetrainEvidencePhase::BrakeForReverse;
            PhaseStartedSeconds = Elapsed;
        }
        break;
    }

    case EDrivetrainEvidencePhase::BrakeForReverse:
        Movement->SetThrottleInput(0.0f);
        Movement->SetBrakeInput(ShiftBrake);
        Movement->SetSteeringInput(0.0f);
        if (AbsoluteSpeedKmh <= ShiftReleaseSpeedKmh)
        {
            ReverseCommitSpeedKmh = AbsoluteSpeedKmh;
            bSafeReverseCommitObserved = ReverseCommitSpeedKmh <= ShiftReleaseSpeedKmh + SafeShiftEvidenceToleranceKmh;
            const int32 GearBefore = CurrentGear;
            Movement->SetTargetGear(-1, true);
            GTT_LOG( Log,
                TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=REVERSE_COMMIT result=%s speed_abs_kmh=%.2f gear_before=%d target=-1 release_kmh=%.2f"),
                bSafeReverseCommitObserved ? TEXT("PASS") : TEXT("FAIL"), ReverseCommitSpeedKmh, GearBefore, ShiftReleaseSpeedKmh);
            if (!bSafeReverseCommitObserved) MarkFailure(TEXT("reverse-commit-above-safe-window"));
            Movement->SetBrakeInput(0.0f);
            Movement->SetThrottleInput(ReverseThrottle);
            Phase = EDrivetrainEvidencePhase::ReverseAcceleration;
            PhaseStartedSeconds = Elapsed;
        }
        break;

    case EDrivetrainEvidencePhase::ReverseAcceleration:
    {
        Movement->SetBrakeInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetThrottleInput(ReverseThrottle);
        const bool bReverseProven = CurrentGear < 0 && SignedSpeedKmh <= -ReverseEvidenceSpeedKmh;
        const bool bTimedOut = Elapsed - PhaseStartedSeconds >= ReverseAccelerationTimeoutSeconds;
        if (bReverseProven || bTimedOut)
        {
            bReverseMotionObserved = bReverseProven;
            GTT_LOG( Log,
                TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=REVERSE_MOTION result=%s signed_speed_kmh=%.2f gear=%d target_speed_kmh=-%.2f"),
                bReverseProven ? TEXT("PASS") : TEXT("FAIL"), SignedSpeedKmh, CurrentGear, ReverseEvidenceSpeedKmh);
            if (!bReverseProven) MarkFailure(TEXT("reverse-motion-not-observed"));
            Movement->SetThrottleInput(0.0f);
            Movement->SetBrakeInput(ShiftBrake);
            Phase = EDrivetrainEvidencePhase::BrakeForForward;
            PhaseStartedSeconds = Elapsed;
        }
        break;
    }

    case EDrivetrainEvidencePhase::BrakeForForward:
        Movement->SetThrottleInput(0.0f);
        Movement->SetBrakeInput(ShiftBrake);
        Movement->SetSteeringInput(0.0f);
        if (AbsoluteSpeedKmh <= ShiftReleaseSpeedKmh)
        {
            ForwardCommitSpeedKmh = AbsoluteSpeedKmh;
            bSafeForwardCommitObserved = ForwardCommitSpeedKmh <= ShiftReleaseSpeedKmh + SafeShiftEvidenceToleranceKmh;
            const int32 GearBefore = CurrentGear;
            Movement->SetTargetGear(1, true);
            GTT_LOG( Log,
                TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=FORWARD_COMMIT result=%s speed_abs_kmh=%.2f gear_before=%d target=1 release_kmh=%.2f"),
                bSafeForwardCommitObserved ? TEXT("PASS") : TEXT("FAIL"), ForwardCommitSpeedKmh, GearBefore, ShiftReleaseSpeedKmh);
            if (!bSafeForwardCommitObserved) MarkFailure(TEXT("forward-commit-above-safe-window"));
            Movement->SetBrakeInput(0.0f);
            Movement->SetThrottleInput(ReturnThrottle);
            Phase = EDrivetrainEvidencePhase::ForwardReturn;
            PhaseStartedSeconds = Elapsed;
        }
        break;

    case EDrivetrainEvidencePhase::ForwardReturn:
    {
        Movement->SetBrakeInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetThrottleInput(ReturnThrottle);
        const bool bForwardProven = CurrentGear > 0 && SignedSpeedKmh >= ForwardReturnEvidenceSpeedKmh;
        const bool bTimedOut = Elapsed - PhaseStartedSeconds >= ForwardReturnTimeoutSeconds;
        if (bForwardProven || bTimedOut)
        {
            bForwardReturnObserved = bForwardProven;
            GTT_LOG( Log,
                TEXT("NATIVE_DRIVETRAIN_SCENARIO phase=FORWARD_MOTION result=%s signed_speed_kmh=%.2f gear=%d target_speed_kmh=%.2f"),
                bForwardProven ? TEXT("PASS") : TEXT("FAIL"), SignedSpeedKmh, CurrentGear, ForwardReturnEvidenceSpeedKmh);
            if (!bForwardProven) MarkFailure(TEXT("forward-return-motion-not-observed"));
            CompleteScenario(Pawn, Movement, bSequenceHealthy ? TEXT("sequence-complete") : TEXT("sequence-complete-with-failures"));
        }
        break;
    }

    case EDrivetrainEvidencePhase::Complete:
    default:
        break;
    }
}
