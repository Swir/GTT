#include "Vehicles/GTTNativeDriveDynamicsSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
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

    int32 SignToDirection(float Value)
    {
        return Value < 0.0f ? -1 : 1;
    }
}

void UGTTNativeDriveDynamicsSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* NativePawn = *It;
        ApplyDriveDynamics(NativePawn, DeltaTime);

        UChaosWheeledVehicleMovementComponent* Movement = NativePawn
            ? Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent())
            : nullptr;
        const FGTTVehicleMigrationSnapshot State = NativePawn
            ? NativePawn->GetMigrationSnapshot()
            : FGTTVehicleMigrationSnapshot();
        const bool bEligible = NativePawn &&
            NativePawn->IsLegacyTakeoverActive() &&
            NativePawn->IsNativeFieldmasterReady() &&
            NativePawn->IsOccupied() &&
            State.ConditionPercent > CriticalConditionPercent &&
            State.FuelLiters > KINDA_SMALL_NUMBER;
        ApplyDrivetrainAuthority(NativePawn, Movement, TEXT("RustyFieldmaster60"), bEligible, DeltaTime);
    }

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* NativePawn = *It;
        UChaosWheeledVehicleMovementComponent* Movement = NativePawn
            ? Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent())
            : nullptr;
        const FGTTRoadVehicleMigrationSnapshot State = NativePawn
            ? NativePawn->GetMigrationSnapshot()
            : FGTTRoadVehicleMigrationSnapshot();
        const bool bEligible = NativePawn &&
            NativePawn->IsLegacyTakeoverActive() &&
            NativePawn->IsNativeReady() &&
            NativePawn->GetDriverPawn() != nullptr &&
            State.ConditionPercent > KINDA_SMALL_NUMBER &&
            State.FuelLiters > KINDA_SMALL_NUMBER;
        ApplyDrivetrainAuthority(
            NativePawn,
            Movement,
            NativePawn ? NativePawn->GetPersistentVehicleId() : NAME_None,
            bEligible,
            DeltaTime);
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
    if (!NativePawn)
    {
        return;
    }

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive() || !NativePawn->IsNativeFieldmasterReady())
    {
        EvidenceSeconds.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive())
    {
        return;
    }

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
    bool bCriticalBreakdown = State.ConditionPercent <= CriticalConditionPercent || State.FuelLiters <= KINDA_SMALL_NUMBER;

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
            SpeedKmh,
            EffectiveTopSpeedKmh,
            State.ConditionPercent,
            State.TireIntegrity,
            EngineLevel,
            TireLevel,
            bGovernorActive ? TEXT("YES") : TEXT("NO"),
            AppliedBrake,
            bCriticalBreakdown ? TEXT("YES") : TEXT("NO"));
    }
}

void UGTTNativeDriveDynamicsSubsystem::ApplyDrivetrainAuthority(
    APawn* NativePawn,
    UChaosWheeledVehicleMovementComponent* Movement,
    FName VehicleId,
    bool bAuthorityEligible,
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

    if (!Authority.bInitialized)
    {
        Authority.StableDirection = AbsoluteSpeedKmh > DirectionShiftReleaseSpeedKmh
            ? MotionDirection
            : (Movement->GetCurrentGear() < 0 ? -1 : 1);
        Authority.bInitialized = true;
    }

    Authority.bDirectionInterlock = false;
    Authority.bEngineBrakeActive = false;
    float AuthorityBrake = 0.0f;

    if (bDirectionRequested)
    {
        const bool bDirectionChangeRequested = RequestedDirection != Authority.StableDirection;
        const bool bMovingAgainstRequest = AbsoluteSpeedKmh > DirectionShiftReleaseSpeedKmh && MotionDirection != RequestedDirection;
        if ((bDirectionChangeRequested || bMovingAgainstRequest) && AbsoluteSpeedKmh > DirectionShiftReleaseSpeedKmh)
        {
            Authority.bDirectionInterlock = true;
            Movement->SetTargetGear(Authority.StableDirection, true);
            Movement->SetThrottleInput(0.0f);
            Movement->SetSteeringInput(RequestedSteering * 0.45f);
            AuthorityBrake = FMath::Clamp(
                DirectionInterlockBrakeMin + AbsoluteSpeedKmh / 120.0f,
                DirectionInterlockBrakeMin,
                DirectionInterlockBrakeMax);
            Movement->SetBrakeInput(AuthorityBrake);
        }
        else
        {
            if (RequestedDirection != Authority.StableDirection)
            {
                Authority.StableDirection = RequestedDirection;
                UE_LOG(LogGTT, Log,
                    TEXT("NATIVE_DIRECTION_SHIFT_COMMIT vehicle=%s direction=%s speed_kmh=%.2f"),
                    *VehicleId.ToString(),
                    Authority.StableDirection < 0 ? TEXT("REVERSE") : TEXT("FORWARD"),
                    SignedSpeedKmh);
            }
            Movement->SetTargetGear(Authority.StableDirection, true);
        }
    }
    else
    {
        Movement->SetThrottleInput(0.0f);
        Authority.bEngineBrakeActive = true;
        AuthorityBrake = AbsoluteSpeedKmh <= StationaryHoldSpeedKmh
            ? StationaryHoldBrake
            : FMath::Clamp(
                NeutralEngineBrakeMin + AbsoluteSpeedKmh / 180.0f,
                NeutralEngineBrakeMin,
                NeutralEngineBrakeMax);
        Movement->SetBrakeInput(AuthorityBrake);
    }

    Authority.EvidenceSeconds += DeltaTime;
    if (Authority.EvidenceSeconds >= DrivetrainEvidenceIntervalSeconds)
    {
        Authority.EvidenceSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_DRIVETRAIN_AUTHORITY_EVIDENCE vehicle=%s speed_kmh=%.2f raw_throttle=%.2f raw_steer=%.2f current_gear=%d stable_direction=%d interlock=%s engine_brake=%s authority_brake=%.2f"),
            *VehicleId.ToString(),
            SignedSpeedKmh,
            RequestedThrottle,
            RequestedSteering,
            Movement->GetCurrentGear(),
            Authority.StableDirection,
            Authority.bDirectionInterlock ? TEXT("YES") : TEXT("NO"),
            Authority.bEngineBrakeActive ? TEXT("YES") : TEXT("NO"),
            AuthorityBrake);
    }

    if (Authority.bDirectionInterlock)
    {
        UE_LOG(LogGTT, Verbose,
            TEXT("NATIVE_DIRECTION_INTERLOCK vehicle=%s requested=%d stable=%d speed_kmh=%.2f brake=%.2f"),
            *VehicleId.ToString(), RequestedDirection, Authority.StableDirection, SignedSpeedKmh, AuthorityBrake);
    }
}

void UGTTNativeDriveDynamicsSubsystem::RemoveAuthorityState(APawn* NativePawn)
{
    if (NativePawn)
    {
        AuthorityStates.Remove(TWeakObjectPtr<APawn>(NativePawn));
    }
}
