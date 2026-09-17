#include "Vehicles/GTTNativeDriveDynamicsSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTNativeAxleTractionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "WheeledVehiclePawn.h"
#include "GTT.h"

namespace
{
    constexpr float BaseFieldmasterTopSpeedKmh = 43.0f;
    constexpr float EngineUpgradeSpeedBonusKmh = 3.0f;
    constexpr float CriticalConditionPercent = 8.0f;
    constexpr float LowTireIntegrityThreshold = 0.35f;
    constexpr float DynamicsEvidenceIntervalSeconds = 5.0f;

    constexpr float DirectionInputDeadzone = 0.05f;
    constexpr float DirectionShiftReleaseSpeedKmh = 3.5f;
    constexpr float DirectionInterlockBrakeMin = 0.38f;
    constexpr float DirectionInterlockBrakeMax = 0.82f;
    constexpr float NeutralEngineBrakeMin = 0.10f;
    constexpr float NeutralEngineBrakeMax = 0.30f;
    constexpr float StationaryHoldSpeedKmh = 1.6f;
    constexpr float StationaryHoldBrake = 0.24f;
    constexpr float DrivetrainEvidenceIntervalSeconds = 4.0f;
    constexpr float MinimumAxleSteeringScale = 0.55f;

    int32 SignToDirection(float Value)
    {
        return Value < 0.0f ? -1 : 1;
    }

    bool GearMatchesDirection(int32 Gear, int32 Direction)
    {
        return Direction < 0 ? Gear < 0 : Gear > 0;
    }
}

void UGTTNativeDriveDynamicsSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* NativePawn = *It;
        ApplyDriveDynamics(NativePawn, DeltaTime);

        UChaosWheeledVehicleMovementComponent* Movement = NativePawn
            ? Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent()) : nullptr;
        const FGTTVehicleMigrationSnapshot State = NativePawn ? NativePawn->GetMigrationSnapshot() : FGTTVehicleMigrationSnapshot();
        const bool bEligible = NativePawn && NativePawn->IsLegacyTakeoverActive() && NativePawn->IsNativeFieldmasterReady()
            && NativePawn->IsOccupied() && State.ConditionPercent > CriticalConditionPercent && State.FuelLiters > KINDA_SMALL_NUMBER;
        ApplyDrivetrainAuthority(NativePawn, Movement, TEXT("RustyFieldmaster60"), bEligible,
            State.TireIntegrity, State.TireUpgradeLevel, DeltaTime);
    }

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* NativePawn = *It;
        UChaosWheeledVehicleMovementComponent* Movement = NativePawn
            ? Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent()) : nullptr;
        const FGTTRoadVehicleMigrationSnapshot State = NativePawn ? NativePawn->GetMigrationSnapshot() : FGTTRoadVehicleMigrationSnapshot();
        const bool bEligible = NativePawn && NativePawn->IsLegacyTakeoverActive() && NativePawn->IsNativeReady()
            && NativePawn->GetDriverPawn() != nullptr && State.ConditionPercent > KINDA_SMALL_NUMBER && State.FuelLiters > KINDA_SMALL_NUMBER;
        ApplyDrivetrainAuthority(NativePawn, Movement,
            NativePawn ? NativePawn->GetPersistentVehicleId() : NAME_None,
            bEligible, State.TireIntegrity, State.TireUpgradeLevel, DeltaTime);
    }
}

TStatId UGTTNativeDriveDynamicsSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeDriveDynamicsSubsystem, STATGROUP_Tickables);
}

bool UGTTNativeDriveDynamicsSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativeDriveDynamicsSubsystem::ApplyDriveDynamics(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn) return;

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive() || !NativePawn->IsNativeFieldmasterReady())
    {
        EvidenceSeconds.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive()) return;

    const FGTTVehicleMigrationSnapshot State = NativePawn->GetMigrationSnapshot();
    const float ConditionAlpha = FMath::Clamp(State.ConditionPercent / 100.0f, 0.0f, 1.0f);
    const float TireAlpha = FMath::Clamp(State.TireIntegrity, 0.0f, 1.0f);
    const int32 EngineLevel = FMath::Clamp(State.EngineUpgradeLevel, 0, 3);
    const int32 TireLevel = FMath::Clamp(State.TireUpgradeLevel, 0, 3);

    const float TunedTopSpeed = BaseFieldmasterTopSpeedKmh + EngineLevel * EngineUpgradeSpeedBonusKmh;
    const float ConditionSpeedFactor = FMath::Lerp(0.45f, 1.0f, ConditionAlpha);
    const float TireSpeedFactor = FMath::Lerp(0.65f, 1.0f, TireAlpha);
    const float EffectiveTopSpeedKmh = TunedTopSpeed * FMath::Min(ConditionSpeedFactor, TireSpeedFactor);
    const float SpeedKmh = NativePawn->GetVelocity().Size() * 0.036f;

    float AppliedBrake = 0.0f;
    bool bGovernorActive = false;
    const bool bCriticalBreakdown = State.ConditionPercent <= CriticalConditionPercent || State.FuelLiters <= KINDA_SMALL_NUMBER;

    if (bCriticalBreakdown)
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        AppliedBrake = 1.0f;
        Movement->SetBrakeInput(AppliedBrake);
    }
    else
    {
        if (SpeedKmh > EffectiveTopSpeedKmh)
        {
            bGovernorActive = true;
            const float Overspeed = SpeedKmh - EffectiveTopSpeedKmh;
            AppliedBrake = FMath::Clamp(0.12f + Overspeed / 18.0f, 0.12f, 0.72f);
            Movement->SetThrottleInput(0.0f);
            Movement->SetBrakeInput(AppliedBrake);
        }
        if (TireAlpha < LowTireIntegrityThreshold && SpeedKmh > 12.0f)
        {
            const float TireDeficit = 1.0f - TireAlpha / LowTireIntegrityThreshold;
            const float TireDrag = FMath::Clamp(0.08f + TireDeficit * 0.28f - TireLevel * 0.025f, 0.05f, 0.34f);
            AppliedBrake = FMath::Max(AppliedBrake, TireDrag);
            Movement->SetBrakeInput(AppliedBrake);
        }
    }

    float& LogSeconds = EvidenceSeconds.FindOrAdd(Key);
    LogSeconds += DeltaTime;
    if (LogSeconds >= DynamicsEvidenceIntervalSeconds)
    {
        LogSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_DRIVE_DYNAMICS vehicle=RustyFieldmaster60 speed_kmh=%.1f cap_kmh=%.1f condition=%.1f tire=%.2f engine_level=%d tire_level=%d governor=%s brake=%.2f critical=%s"),
            SpeedKmh, EffectiveTopSpeedKmh, State.ConditionPercent, State.TireIntegrity, EngineLevel, TireLevel,
            bGovernorActive ? TEXT("YES") : TEXT("NO"), AppliedBrake, bCriticalBreakdown ? TEXT("YES") : TEXT("NO"));
    }
}

void UGTTNativeDriveDynamicsSubsystem::ApplyDrivetrainAuthority(
    APawn* NativePawn,
    UChaosWheeledVehicleMovementComponent* Movement,
    FName VehicleId,
    bool bAuthorityEligible,
    float TireIntegrity,
    int32 TireUpgradeLevel,
    float DeltaTime)
{
    if (!NativePawn || !Movement || !Movement->IsActive() || !bAuthorityEligible)
    {
        RemoveAuthorityState(NativePawn);
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(NativePawn->GetController());
    if (!PlayerController)
    {
        RemoveAuthorityState(NativePawn);
        return;
    }

    const TWeakObjectPtr<APawn> Key(NativePawn);
    FGTTNativeDrivetrainAuthorityState& Authority = AuthorityStates.FindOrAdd(Key);
    const float RequestedThrottle = FMath::Clamp(PlayerController->GetInputAxisValue(TEXT("VehicleThrottle")), -1.0f, 1.0f);
    const float RequestedSteering = FMath::Clamp(PlayerController->GetInputAxisValue(TEXT("VehicleSteer")), -1.0f, 1.0f);
    const float SignedSpeedKmh = FVector::DotProduct(NativePawn->GetVelocity(), NativePawn->GetActorForwardVector()) * 0.036f;
    const float AbsoluteSpeedKmh = FMath::Abs(SignedSpeedKmh);
    const int32 MotionDirection = SignToDirection(SignedSpeedKmh);
    const bool bDirectionRequested = FMath::Abs(RequestedThrottle) > DirectionInputDeadzone;
    const int32 RequestedDirection = bDirectionRequested ? SignToDirection(RequestedThrottle) : Authority.StableDirection;
    const int32 CurrentGear = Movement->GetCurrentGear();

    if (!Authority.bInitialized)
    {
        Authority.StableDirection = AbsoluteSpeedKmh > DirectionShiftReleaseSpeedKmh
            ? MotionDirection : (CurrentGear < 0 ? -1 : 1);
        Authority.LastObservedGear = CurrentGear;
        Authority.bInitialized = true;
    }
    else if (CurrentGear > 0 && Authority.LastObservedGear > 0 && CurrentGear != Authority.LastObservedGear)
    {
        ++Authority.AutomaticForwardGearChangeCount;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_AUTOMATIC_GEAR_SHIFT vehicle=%s from=%d to=%d speed_kmh=%.2f changes=%d"),
            *VehicleId.ToString(), Authority.LastObservedGear, CurrentGear, SignedSpeedKmh,
            Authority.AutomaticForwardGearChangeCount);
    }
    Authority.LastObservedGear = CurrentGear;

    Authority.bDirectionInterlock = false;
    Authority.bEngineBrakeActive = false;
    Authority.bAxleTorqueCut = false;

    float FinalThrottle = Movement->GetThrottleInput();
    float FinalBrake = Movement->GetBrakeInput();
    float FinalSteering = Movement->GetSteeringInput();
    float DrivetrainBrake = 0.0f;
    bool bGearCommandIssued = false;

    if (bDirectionRequested)
    {
        const bool bDirectionChangeRequested = RequestedDirection != Authority.StableDirection;
        const bool bMovingAgainstRequest = AbsoluteSpeedKmh > DirectionShiftReleaseSpeedKmh && MotionDirection != RequestedDirection;
        if ((bDirectionChangeRequested || bMovingAgainstRequest) && AbsoluteSpeedKmh > DirectionShiftReleaseSpeedKmh)
        {
            // Do not force first/reverse gear while the vehicle is still moving in the old direction.
            // Braking the current gear down to the release threshold protects the drivetrain and, for
            // forward travel, lets Chaos retain whichever automatic forward gear it selected.
            Authority.bDirectionInterlock = true;
            FinalThrottle = 0.0f;
            FinalSteering = FMath::Clamp(FinalSteering, -0.45f, 0.45f);
            DrivetrainBrake = FMath::Clamp(DirectionInterlockBrakeMin + AbsoluteSpeedKmh / 120.0f,
                DirectionInterlockBrakeMin, DirectionInterlockBrakeMax);
        }
        else
        {
            bool bDirectionShiftCommitted = false;
            if (RequestedDirection != Authority.StableDirection)
            {
                Authority.StableDirection = RequestedDirection;
                ++Authority.DirectionShiftCommitCount;
                bDirectionShiftCommitted = true;
                UE_LOG(LogGTT, Log, TEXT("NATIVE_DIRECTION_SHIFT_COMMIT vehicle=%s direction=%s speed_kmh=%.2f commits=%d"),
                    *VehicleId.ToString(), Authority.StableDirection < 0 ? TEXT("REVERSE") : TEXT("FORWARD"),
                    SignedSpeedKmh, Authority.DirectionShiftCommitCount);
            }

            // Only write a target gear when direction actually changes, or when Chaos is neutral / in
            // the opposite direction. Re-sending +1 every frame pins an automatic transmission to first
            // gear, so once a forward gear is engaged we leave subsequent 1->2->3... shifts to Chaos.
            const int32 EngagedGear = Movement->GetCurrentGear();
            if (bDirectionShiftCommitted || !GearMatchesDirection(EngagedGear, Authority.StableDirection))
            {
                Movement->SetTargetGear(Authority.StableDirection, true);
                ++Authority.GearCommandCount;
                bGearCommandIssued = true;
            }
        }
    }
    else
    {
        FinalThrottle = 0.0f;
        Authority.bEngineBrakeActive = true;
        DrivetrainBrake = AbsoluteSpeedKmh <= StationaryHoldSpeedKmh
            ? StationaryHoldBrake
            : FMath::Clamp(NeutralEngineBrakeMin + AbsoluteSpeedKmh / 180.0f, NeutralEngineBrakeMin, NeutralEngineBrakeMax);
    }

    FGTTNativeAxleTractionSnapshot AxleSnapshot;
    if (UWorld* World = GetWorld())
    {
        if (UGTTNativeAxleTractionSubsystem* AxleSubsystem = World->GetSubsystem<UGTTNativeAxleTractionSubsystem>())
        {
            AxleSnapshot = AxleSubsystem->SampleSnapshot(Movement, TireIntegrity, TireUpgradeLevel);
        }
    }

    float AxleBrake = 0.0f;
    if (AxleSnapshot.bActive)
    {
        Authority.bAxleTorqueCut = AxleSnapshot.bTorqueCut;
        Authority.bSuspensionRuntimeReady = AxleSnapshot.bSuspensionRuntimeReady;
        if (AxleSnapshot.bTorqueCut) FinalThrottle = 0.0f;
        AxleBrake = AxleSnapshot.BrakeAssist;

        const float TractionSteeringScale = FMath::Clamp(
            MinimumAxleSteeringScale + (1.0f - MinimumAxleSteeringScale) * AxleSnapshot.TractionAuthority,
            MinimumAxleSteeringScale, 1.0f);
        const float ImbalanceSteeringScale = FMath::Clamp(1.0f - AxleSnapshot.AxleImbalance * 0.35f, 0.68f, 1.0f);
        FinalSteering *= FMath::Min(TractionSteeringScale, ImbalanceSteeringScale);
    }
    else
    {
        Authority.bSuspensionRuntimeReady = false;
    }

    FinalBrake = FMath::Max3(FinalBrake, DrivetrainBrake, AxleBrake);
    FinalThrottle = FMath::Clamp(FinalThrottle, -1.0f, 1.0f);
    FinalBrake = FMath::Clamp(FinalBrake, 0.0f, 1.0f);
    FinalSteering = FMath::Clamp(FinalSteering, -1.0f, 1.0f);

    Movement->SetThrottleInput(FinalThrottle);
    Movement->SetBrakeInput(FinalBrake);
    Movement->SetSteeringInput(FinalSteering);

    Authority.FinalThrottle = FinalThrottle;
    Authority.FinalBrake = FinalBrake;
    Authority.FinalSteering = FinalSteering;

    Authority.EvidenceSeconds += DeltaTime;
    if (Authority.EvidenceSeconds >= DrivetrainEvidenceIntervalSeconds)
    {
        Authority.EvidenceSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_COMMAND_COMPOSITION_EVIDENCE vehicle=%s speed_kmh=%.2f raw_throttle=%.2f final_throttle=%.2f final_brake=%.2f final_steer=%.2f current_gear=%d stable_direction=%d interlock=%s engine_brake=%s axle_cut=%s axle_authority=%.2f suspension_ready=%s gear_command=%s gear_commands=%d auto_forward_changes=%d direction_commits=%d"),
            *VehicleId.ToString(), SignedSpeedKmh, RequestedThrottle, FinalThrottle, FinalBrake, FinalSteering,
            Movement->GetCurrentGear(), Authority.StableDirection,
            Authority.bDirectionInterlock ? TEXT("YES") : TEXT("NO"),
            Authority.bEngineBrakeActive ? TEXT("YES") : TEXT("NO"),
            Authority.bAxleTorqueCut ? TEXT("YES") : TEXT("NO"),
            AxleSnapshot.TractionAuthority,
            Authority.bSuspensionRuntimeReady ? TEXT("YES") : TEXT("NO"),
            bGearCommandIssued ? TEXT("YES") : TEXT("NO"),
            Authority.GearCommandCount,
            Authority.AutomaticForwardGearChangeCount,
            Authority.DirectionShiftCommitCount);

        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_AUTOMATIC_GEARBOX_EVIDENCE vehicle=%s current_gear=%d stable_direction=%d gear_commands=%d auto_forward_changes=%d direction_commits=%d interlock=%s speed_kmh=%.2f"),
            *VehicleId.ToString(), Movement->GetCurrentGear(), Authority.StableDirection,
            Authority.GearCommandCount, Authority.AutomaticForwardGearChangeCount,
            Authority.DirectionShiftCommitCount,
            Authority.bDirectionInterlock ? TEXT("YES") : TEXT("NO"), SignedSpeedKmh);

        // Keep the 0.0.75 telemetry contract alive for existing playtests/log parsers while
        // 0.0.77+ adds richer command-composition and automatic-gearbox evidence above.
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_DRIVETRAIN_AUTHORITY_EVIDENCE vehicle=%s speed_kmh=%.2f raw_throttle=%.2f raw_steer=%.2f current_gear=%d stable_direction=%d interlock=%s engine_brake=%s authority_brake=%.2f"),
            *VehicleId.ToString(), SignedSpeedKmh, RequestedThrottle, RequestedSteering,
            Movement->GetCurrentGear(), Authority.StableDirection,
            Authority.bDirectionInterlock ? TEXT("YES") : TEXT("NO"),
            Authority.bEngineBrakeActive ? TEXT("YES") : TEXT("NO"),
            FinalBrake);
    }

    if (Authority.bDirectionInterlock)
    {
        UE_LOG(LogGTT, Verbose, TEXT("NATIVE_DIRECTION_INTERLOCK vehicle=%s requested=%d stable=%d speed_kmh=%.2f brake=%.2f"),
            *VehicleId.ToString(), RequestedDirection, Authority.StableDirection, SignedSpeedKmh, FinalBrake);
    }
    if (Authority.bAxleTorqueCut || AxleBrake > 0.0f)
    {
        UE_LOG(LogGTT, Verbose,
            TEXT("NATIVE_COMMAND_COMPOSITION_LIMIT vehicle=%s axle_cut=%s axle_brake=%.2f drivetrain_brake=%.2f final_brake=%.2f final_throttle=%.2f final_steer=%.2f"),
            *VehicleId.ToString(), Authority.bAxleTorqueCut ? TEXT("YES") : TEXT("NO"), AxleBrake, DrivetrainBrake,
            FinalBrake, FinalThrottle, FinalSteering);
    }
}

void UGTTNativeDriveDynamicsSubsystem::RemoveAuthorityState(APawn* NativePawn)
{
    if (NativePawn) AuthorityStates.Remove(TWeakObjectPtr<APawn>(NativePawn));
}
