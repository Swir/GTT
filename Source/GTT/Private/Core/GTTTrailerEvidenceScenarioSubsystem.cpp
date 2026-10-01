#include "Core/GTTTrailerEvidenceScenarioSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTTrailerAuthoredRuntimeSubsystem.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace
{
    constexpr float StartDelaySeconds = 126.0f;
    constexpr float FocusedStartDelaySeconds = 8.0f;
    constexpr float GlobalDeadlineSeconds = 172.0f;
    constexpr float AuthoredRuntimeTimeoutSeconds = 10.0f;
    constexpr float PostAttachSettleSeconds = 1.0f;
    constexpr float LoadedMotionTimeoutSeconds = 22.0f;
    constexpr float ControlledStopTimeoutSeconds = 8.0f;
    constexpr float SampleIntervalSeconds = 0.25f;
    constexpr float TowThrottle = 1.0f;
    constexpr float MaxSteering = 0.0f;
    constexpr float MinimumEvidenceSpeedKmh = 4.0f;
    constexpr float MinimumTowDistanceCm = 900.0f;
    constexpr float StopSpeedKmh = 1.5f;
    constexpr float SafeHitchErrorCm = 80.0f;
    constexpr float HardHitchErrorCm = 110.0f;
    constexpr int32 MinimumMovingDualContactSamples = 8;
    const FName AuthoredTrailerTag(TEXT("GTT.AuthoredTrailerRig"));
    const FName TowEyeSocket(TEXT("socket_hitch"));
    const FVector TrailerEvidencePadLocation(8500.0f, 5000.0f, 0.0f);
    const FRotator TrailerEvidencePadRotation(0.0f, 0.0f, 0.0f);
}

void UGTTTrailerEvidenceScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    const TCHAR* CommandLine = FCommandLine::Get();
    const bool bFocusedTrailerRuntime = FParse::Param(CommandLine, TEXT("GTTTrailerRuntimeScenario"));
    const bool bDemoSmokeRuntime = FParse::Param(CommandLine, TEXT("GTTDemoSmokeScenario"));
    bEnabled = bFocusedTrailerRuntime ||
        (bDemoSmokeRuntime && !FParse::Param(CommandLine, TEXT("GTTDisableTrailerScenario")));
    if (bEnabled)
    {
        if (bFocusedTrailerRuntime && !bDemoSmokeRuntime)
        {
            Elapsed = StartDelaySeconds - FocusedStartDelaySeconds;
        }
        GTT_LOG( Log,
            TEXT("NATIVE_TRAILER_SCENARIO_BEGIN version=1 start_delay=%.1f effective_start_delay=%.1f deadline=%.1f min_distance_cm=%.0f min_speed_kmh=%.1f source=formal"),
            StartDelaySeconds, bFocusedTrailerRuntime && !bDemoSmokeRuntime ? FocusedStartDelaySeconds : StartDelaySeconds,
            GlobalDeadlineSeconds, MinimumTowDistanceCm, MinimumEvidenceSpeedKmh);
    }
}

TStatId UGTTTrailerEvidenceScenarioSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTTrailerEvidenceScenarioSubsystem, STATGROUP_Tickables);
}

bool UGTTTrailerEvidenceScenarioSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

AGTTFieldmasterNativePawn* UGTTTrailerEvidenceScenarioSubsystem::ResolveFieldmaster()
{
    if (Fieldmaster.IsValid()) return Fieldmaster.Get();
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* Candidate = *It;
        if (!Candidate || !Candidate->IsNativeFieldmasterReady()) continue;
        if (!Candidate->IsLegacyTakeoverActive())
        {
            for (TActorIterator<AGTTVehicleBase> LegacyIt(World); LegacyIt; ++LegacyIt)
            {
                AGTTVehicleBase* Legacy = *LegacyIt;
                if (!Legacy || Legacy->GetPersistentVehicleId() != TEXT("RustyFieldmaster60")) continue;
                if (!Legacy->IsOwnedByPlayer()) Legacy->MarkOwnedByPlayer();
                Legacy->RepairVehicle(100000.0f);
                Legacy->RefuelVehicle(100000.0f);
                Legacy->RepairTires();
                break;
            }
            Candidate->TryActivateLegacyTakeover();
        }
        if (Candidate->IsLegacyTakeoverActive())
        {
            FGTTVehicleMigrationSnapshot State = Candidate->GetMigrationSnapshot();
            State.ConditionPercent = 1.0f;
            State.FuelLiters = FMath::Max(State.FuelLiters, 10.0f);
            State.bOwnedByPlayer = true;
            State.TireIntegrity = 1.0f;
            Candidate->ApplyMigrationSnapshot(State);
            Fieldmaster = Candidate;
            return Candidate;
        }
    }
    return nullptr;
}

AGTTFarmTrailer* UGTTTrailerEvidenceScenarioSubsystem::ResolveTrailer()
{
    if (Trailer.IsValid()) return Trailer.Get();
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        AGTTFarmTrailer* Candidate = *It;
        if (Candidate)
        {
            Trailer = Candidate;
            return Candidate;
        }
    }
    return nullptr;
}

UChaosWheeledVehicleMovementComponent* UGTTTrailerEvidenceScenarioSubsystem::ResolveMovement(AGTTFieldmasterNativePawn* Pawn) const
{
    return Pawn ? Cast<UChaosWheeledVehicleMovementComponent>(Pawn->GetVehicleMovementComponent()) : nullptr;
}

UGTTTrailerAuthoredRuntimeSubsystem* UGTTTrailerEvidenceScenarioSubsystem::ResolveRuntime() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UGTTTrailerAuthoredRuntimeSubsystem>() : nullptr;
}

bool UGTTTrailerEvidenceScenarioSubsystem::StageTrailerAtHitch(AGTTFieldmasterNativePawn* Pawn, AGTTFarmTrailer* FarmTrailer)
{
    if (!Pawn || !FarmTrailer) return false;

    UWorld* World = GetWorld();
    USkeletalMeshComponent* VehicleBody = Pawn->GetMesh();
    if (!World || !VehicleBody) return false;

    FVector TowStageLocation = TrailerEvidencePadLocation;
    FHitResult GroundHit;
    FCollisionObjectQueryParams GroundObjects;
    GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(GTTTrailerEvidenceGround), false, Pawn);
    if (!World->LineTraceSingleByObjectType(
        GroundHit,
        TowStageLocation + FVector(0.0f, 0.0f, 1000.0f),
        TowStageLocation - FVector(0.0f, 0.0f, 1500.0f),
        GroundObjects,
        GroundQuery))
    {
        return false;
    }

    TowStageLocation.Z = GroundHit.ImpactPoint.Z + 4.0f;
    FarmTrailer->DetachTrailer();
    Pawn->ApplyAcceptanceDriveCommand(0.0f, 0.0f, 1.0f);
    VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
    VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Pawn->SetActorTransform(
        FTransform(TrailerEvidencePadRotation, TowStageLocation),
        false,
        nullptr,
        ETeleportType::ResetPhysics);
    VehicleBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
    VehicleBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    VehicleBody->WakeAllRigidBodies();

    FTransform HitchTransform;
    if (!Pawn->TryGetRearHitchTransform(HitchTransform)) return false;

    const FVector InitialLocation = HitchTransform.GetLocation() - Pawn->GetActorForwardVector() * 360.0f;
    FRotator StageRotation = Pawn->GetActorRotation();
    StageRotation.Yaw += 180.0f;
    StageRotation.Normalize();
    FTransform StageTransform(StageRotation, InitialLocation, FVector::OneVector);
    FarmTrailer->ResetTrailer(StageTransform);

    TArray<USkinnedMeshComponent*> SkeletalMeshes;
    FarmTrailer->GetComponents<USkinnedMeshComponent>(SkeletalMeshes);
    USkinnedMeshComponent* AuthoredRig = nullptr;
    for (USkinnedMeshComponent* Mesh : SkeletalMeshes)
    {
        if (Mesh && (Mesh->ComponentHasTag(AuthoredTrailerTag) || Mesh->GetFName() == TEXT("AuthoredTrailerMesh")))
        {
            AuthoredRig = Mesh;
            break;
        }
    }

    if (!AuthoredRig || !AuthoredRig->DoesSocketExist(TowEyeSocket))
    {
        GTT_LOG( Error,
            TEXT("NATIVE_TRAILER_SCENARIO phase=STAGE result=FAIL reason=authored-rig-or-tow-eye-unavailable source=formal"));
        return false;
    }

    const FVector TowEyeWorld = AuthoredRig->GetSocketTransform(TowEyeSocket, RTS_World).GetLocation();
    const FVector AlignmentDelta = HitchTransform.GetLocation() - TowEyeWorld;
    StageTransform.SetLocation(StageTransform.GetLocation() + AlignmentDelta);
    FarmTrailer->ResetTrailer(StageTransform);
    FarmTrailer->SetCargoLoaded(true);

    const bool bAttached = FarmTrailer->AttachToNativeFieldmaster(Pawn);
    GTT_LOG( Log,
        TEXT("NATIVE_TRAILER_SCENARIO phase=STAGE result=%s align_delta_cm=%.1f cargo=%d tow_load=%.2f tow_stage=(%.0f,%.0f,%.0f) source=formal"),
        bAttached ? TEXT("PASS") : TEXT("FAIL"), AlignmentDelta.Size(), FarmTrailer->HasCargo() ? 1 : 0, FarmTrailer->GetTowLoadFactor(),
        TowStageLocation.X, TowStageLocation.Y, TowStageLocation.Z);
    return bAttached;
}

void UGTTTrailerEvidenceScenarioSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    GTT_LOG( Error,
        TEXT("NATIVE_TRAILER_SCENARIO phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f source=formal"),
        Reason, Elapsed);
}

void UGTTTrailerEvidenceScenarioSubsystem::CompleteScenario(
    AGTTFieldmasterNativePawn* Pawn,
    AGTTFarmTrailer* FarmTrailer,
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
        && bAttachmentProven
        && bAuthoredRuntimeProven
        && bLoadedTowProven
        && bControlledStopProven
        && FarmTrailer
        && FarmTrailer->IsAttachedToNativeFieldmaster()
        && FarmTrailer->HasCargo()
        && MovingDualContactSamples >= MinimumMovingDualContactSamples
        && SafeMovingSamples >= MinimumMovingDualContactSamples
        && MaxTowSpeedKmh >= MinimumEvidenceSpeedKmh
        && MaxTowDistanceCm >= MinimumTowDistanceCm
        && MaxHitchErrorCm <= HardHitchErrorCm;

    GTT_LOG( Log,
        TEXT("NATIVE_TRAILER_SCENARIO_COMPLETE result=%s route=loaded-authored-tow attachment=%d authored=%d loaded=%d stopped=%d max_speed_kmh=%.2f distance_cm=%.1f dual_contact_samples=%d safe_samples=%d max_hitch_error_cm=%.1f max_articulation_deg=%.1f min_cargo_integrity=%.3f final_speed_kmh=%.2f reason=%s elapsed=%.2f source=formal"),
        bPass ? TEXT("PASS") : TEXT("FAIL"),
        bAttachmentProven ? 1 : 0, bAuthoredRuntimeProven ? 1 : 0, bLoadedTowProven ? 1 : 0, bControlledStopProven ? 1 : 0,
        MaxTowSpeedKmh, MaxTowDistanceCm, MovingDualContactSamples, SafeMovingSamples, MaxHitchErrorCm, MaxArticulationDeg,
        MinCargoIntegrity, Pawn ? Pawn->GetVelocity().Size2D() * 0.036f : 0.0f, Reason, Elapsed);

    Phase = ETrailerEvidencePhase::Complete;
    bFinished = true;
}

void UGTTTrailerEvidenceScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < StartDelaySeconds) return;

    AGTTFieldmasterNativePawn* Pawn = ResolveFieldmaster();
    AGTTFarmTrailer* FarmTrailer = ResolveTrailer();
    UChaosWheeledVehicleMovementComponent* Movement = ResolveMovement(Pawn);
    UGTTTrailerAuthoredRuntimeSubsystem* Runtime = ResolveRuntime();

    if (!Pawn || !FarmTrailer || !Movement || !Movement->IsActive() || !Runtime)
    {
        if (Elapsed >= GlobalDeadlineSeconds)
        {
            MarkFailure(TEXT("fieldmaster-trailer-chaos-or-runtime-unavailable"));
            CompleteScenario(Pawn, FarmTrailer, Movement, TEXT("global-deadline"));
        }
        return;
    }

    if (Elapsed >= GlobalDeadlineSeconds)
    {
        MarkFailure(TEXT("global-sequence-timeout"));
        CompleteScenario(Pawn, FarmTrailer, Movement, TEXT("global-deadline"));
        return;
    }

    switch (Phase)
    {
    case ETrailerEvidencePhase::Waiting:
        Phase = ETrailerEvidencePhase::StageAndAttach;
        PhaseStartedSeconds = Elapsed;
        break;

    case ETrailerEvidencePhase::StageAndAttach:
        bAttachmentProven = StageTrailerAtHitch(Pawn, FarmTrailer);
        if (!bAttachmentProven)
        {
            MarkFailure(TEXT("authored-trailer-staging-or-attachment-failed"));
            CompleteScenario(Pawn, FarmTrailer, Movement, TEXT("stage-failed"));
            return;
        }
        Phase = ETrailerEvidencePhase::AwaitAuthoredRuntime;
        PhaseStartedSeconds = Elapsed;
        break;

    case ETrailerEvidencePhase::AwaitAuthoredRuntime:
    {
        Pawn->ApplyAcceptanceDriveCommand(0.0f, 0.0f, 0.90f);
        const FGTTAuthoredTrailerRuntimeSnapshot Snapshot = Runtime->GetRuntimeSnapshot(FarmTrailer);
        const bool bReady = Elapsed - PhaseStartedSeconds >= PostAttachSettleSeconds
            && Runtime->IsAuthoredRuntimeActive(FarmTrailer)
            && Snapshot.bAuthoredRigValid
            && Snapshot.bAuthoredPresentationActive
            && Snapshot.bLeftWheelContact
            && Snapshot.bRightWheelContact
            && Snapshot.HitchAlignmentErrorCm <= SafeHitchErrorCm
            && Pawn->GetVelocity().Size2D() * 0.036f <= StopSpeedKmh;

        if (bReady)
        {
            bAuthoredRuntimeProven = true;
            MotionStartLocation = Pawn->GetActorLocation();
            Pawn->ApplyAcceptanceDriveCommand(TowThrottle, 0.0f, 0.0f);
            Phase = ETrailerEvidencePhase::LoadedMotion;
            PhaseStartedSeconds = Elapsed;
            SampleAccumulator = 0.0f;
            GTT_LOG( Log,
                TEXT("NATIVE_TRAILER_SCENARIO phase=AUTHORED_READY result=PASS contacts=%.1f hitch_error_cm=%.1f cargo=%d tow_load=%.2f source=formal"),
                Snapshot.ContactRatio, Snapshot.HitchAlignmentErrorCm, FarmTrailer->HasCargo() ? 1 : 0, FarmTrailer->GetTowLoadFactor());
        }
        else if (Elapsed - PhaseStartedSeconds >= AuthoredRuntimeTimeoutSeconds)
        {
            MarkFailure(TEXT("authored-runtime-not-stable-before-timeout"));
            CompleteScenario(Pawn, FarmTrailer, Movement, TEXT("authored-runtime-timeout"));
        }
        break;
    }

    case ETrailerEvidencePhase::LoadedMotion:
    {
        const float MotionElapsed = Elapsed - PhaseStartedSeconds;
        Pawn->ApplyAcceptanceDriveCommand(TowThrottle, FMath::Sin(MotionElapsed * 0.75f) * MaxSteering, 0.0f);

        const float SpeedKmh = Pawn->GetVelocity().Size2D() * 0.036f;
        const float DistanceCm = FVector::Dist2D(MotionStartLocation, Pawn->GetActorLocation());
        const FGTTAuthoredTrailerRuntimeSnapshot Snapshot = Runtime->GetRuntimeSnapshot(FarmTrailer);
        MaxTowSpeedKmh = FMath::Max(MaxTowSpeedKmh, SpeedKmh);
        MaxTowDistanceCm = FMath::Max(MaxTowDistanceCm, DistanceCm);
        MaxHitchErrorCm = FMath::Max(MaxHitchErrorCm, Snapshot.HitchAlignmentErrorCm);
        MaxArticulationDeg = FMath::Max(MaxArticulationDeg, Snapshot.ArticulationYawDeg);
        MinCargoIntegrity = FMath::Min(MinCargoIntegrity, FarmTrailer->GetCargoIntegrity());

        SampleAccumulator += DeltaTime;
        if (SampleAccumulator >= SampleIntervalSeconds)
        {
            SampleAccumulator = 0.0f;
            const bool bMoving = SpeedKmh >= MinimumEvidenceSpeedKmh;
            const bool bDualContact = Snapshot.bLeftWheelContact && Snapshot.bRightWheelContact && Snapshot.ContactRatio >= 0.999f;
            const bool bSafeHitch = Snapshot.HitchAlignmentErrorCm <= SafeHitchErrorCm;
            if (bMoving && bDualContact) ++MovingDualContactSamples;
            if (bMoving && bDualContact && bSafeHitch) ++SafeMovingSamples;

            GTT_LOG( Log,
                TEXT("NATIVE_TRAILER_SCENARIO_SAMPLE speed_kmh=%.2f distance_cm=%.1f loaded=%d attached=%d active=%d contacts=%.1f left=%d right=%d hitch_error_cm=%.1f articulation_deg=%.1f cargo_integrity=%.3f trailer_integrity=%.3f hitch_load=%.3f source=formal"),
                SpeedKmh, DistanceCm, FarmTrailer->HasCargo() ? 1 : 0, FarmTrailer->IsAttachedToNativeFieldmaster() ? 1 : 0,
                Runtime->IsAuthoredRuntimeActive(FarmTrailer) ? 1 : 0, Snapshot.ContactRatio,
                Snapshot.bLeftWheelContact ? 1 : 0, Snapshot.bRightWheelContact ? 1 : 0,
                Snapshot.HitchAlignmentErrorCm, Snapshot.ArticulationYawDeg, FarmTrailer->GetCargoIntegrity(),
                FarmTrailer->GetTrailerIntegrity(), FarmTrailer->GetHitchLoad());
        }

        const bool bEvidenceReady = MaxTowDistanceCm >= MinimumTowDistanceCm
            && MaxTowSpeedKmh >= MinimumEvidenceSpeedKmh
            && MovingDualContactSamples >= MinimumMovingDualContactSamples
            && SafeMovingSamples >= MinimumMovingDualContactSamples
            && FarmTrailer->HasCargo()
            && FarmTrailer->IsAttachedToNativeFieldmaster()
            && FarmTrailer->GetTowLoadFactor() >= 0.50f;

        if (bEvidenceReady || MotionElapsed >= LoadedMotionTimeoutSeconds)
        {
            bLoadedTowProven = bEvidenceReady;
            GTT_LOG( Log,
                TEXT("NATIVE_TRAILER_SCENARIO phase=LOADED_MOTION result=%s max_speed_kmh=%.2f distance_cm=%.1f dual_contact_samples=%d safe_samples=%d tow_load=%.2f source=formal"),
                bEvidenceReady ? TEXT("PASS") : TEXT("FAIL"), MaxTowSpeedKmh, MaxTowDistanceCm,
                MovingDualContactSamples, SafeMovingSamples, FarmTrailer->GetTowLoadFactor());
            if (!bEvidenceReady) MarkFailure(TEXT("loaded-motion-evidence-incomplete"));
            Pawn->ApplyAcceptanceDriveCommand(0.0f, 0.0f, 0.90f);
            Phase = ETrailerEvidencePhase::ControlledStop;
            PhaseStartedSeconds = Elapsed;
        }
        break;
    }

    case ETrailerEvidencePhase::ControlledStop:
    {
        Pawn->ApplyAcceptanceDriveCommand(0.0f, 0.0f, 0.90f);
        const float SpeedKmh = Pawn->GetVelocity().Size2D() * 0.036f;
        const bool bStopped = SpeedKmh <= StopSpeedKmh;
        const bool bTimedOut = Elapsed - PhaseStartedSeconds >= ControlledStopTimeoutSeconds;
        if (bStopped || bTimedOut)
        {
            bControlledStopProven = bStopped
                && FarmTrailer->IsAttachedToNativeFieldmaster()
                && FarmTrailer->HasCargo()
                && FarmTrailer->HasIntactAxle();
            GTT_LOG( Log,
                TEXT("NATIVE_TRAILER_SCENARIO phase=CONTROLLED_STOP result=%s speed_kmh=%.2f attached=%d cargo=%d axle_intact=%d source=formal"),
                bControlledStopProven ? TEXT("PASS") : TEXT("FAIL"), SpeedKmh,
                FarmTrailer->IsAttachedToNativeFieldmaster() ? 1 : 0, FarmTrailer->HasCargo() ? 1 : 0,
                FarmTrailer->HasIntactAxle() ? 1 : 0);
            if (!bControlledStopProven) MarkFailure(TEXT("controlled-stop-or-trailer-integrity-failed"));
            CompleteScenario(Pawn, FarmTrailer, Movement, bSequenceHealthy ? TEXT("sequence-complete") : TEXT("sequence-complete-with-failures"));
        }
        break;
    }

    case ETrailerEvidencePhase::Complete:
    default:
        break;
    }
}
