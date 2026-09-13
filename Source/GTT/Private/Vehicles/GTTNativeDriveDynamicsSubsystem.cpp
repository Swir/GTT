#include "Vehicles/GTTNativeDriveDynamicsSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr float BaseFieldmasterTopSpeedKmh = 43.0f;
    constexpr float EngineUpgradeSpeedBonusKmh = 3.0f;
    constexpr float CriticalConditionPercent = 8.0f;
    constexpr float LowTireIntegrityThreshold = 0.35f;
    constexpr float DynamicsEvidenceIntervalSeconds = 5.0f;
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
        ApplyDriveDynamics(*It, DeltaTime);
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
