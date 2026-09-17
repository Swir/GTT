#include "Vehicles/GTTFieldmasterChaosMovementComponent.h"

#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float IdleBrakeInput = 0.15f;
    constexpr float DirectionDeadZone = 0.05f;
    constexpr float MinimumDamagedDriveFactor = 0.28f;
    constexpr float MinimumSteeringAuthority = 0.35f;
}

UGTTFieldmasterChaosMovementComponent::UGTTFieldmasterChaosMovementComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bMechanicalSimEnabled = true;
    bFieldmasterConfigurationValid = false;
}

float UGTTFieldmasterChaosMovementComponent::NormalizeCondition(float ConditionPercent)
{
    // The migration layer historically accepted both 0..1 ratios and old 0..100 values.
    // Keep the native movement authority defensive until the old save boundary disappears.
    const float Normalized = ConditionPercent > 1.0f ? ConditionPercent / 100.0f : ConditionPercent;
    return FMath::Clamp(Normalized, 0.0f, 1.0f);
}

bool UGTTFieldmasterChaosMovementComponent::ConfigureAndValidateFieldmaster(FString& OutSummary)
{
    FString WheelConfigureSummary;
    const bool bWheelsConfigured = UGTTChaosNativeSetupLibrary::ConfigureCanonicalWheelSetups(this, FieldmasterVehicleId, WheelConfigureSummary);

    FString PowertrainConfigureSummary;
    const bool bPowertrainConfigured = UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain(this, FieldmasterVehicleId, PowertrainConfigureSummary);

    FString WheelValidationSummary;
    const bool bWheelsValid = bWheelsConfigured &&
        UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(this, FieldmasterVehicleId, WheelValidationSummary);

    FString PowertrainValidationSummary;
    const bool bPowertrainValid = bPowertrainConfigured &&
        UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(this, FieldmasterVehicleId, PowertrainValidationSummary);

    bFieldmasterConfigurationValid = bWheelsValid && bPowertrainValid && bMechanicalSimEnabled;
    FieldmasterConfigurationSummary = FString::Printf(
        TEXT("DEDICATED MOVEMENT: %s | WHEELS: %s | POWERTRAIN: %s | MECHANICAL SIM: %s"),
        bFieldmasterConfigurationValid ? TEXT("VALID") : TEXT("INVALID"),
        bWheelsValid ? *WheelValidationSummary : *WheelConfigureSummary,
        bPowertrainValid ? *PowertrainValidationSummary : *PowertrainConfigureSummary,
        bMechanicalSimEnabled ? TEXT("ON") : TEXT("OFF"));

    OutSummary = FieldmasterConfigurationSummary;
    if (!bFieldmasterConfigurationValid)
    {
        HoldFieldmasterStopped();
    }
    return bFieldmasterConfigurationValid;
}

void UGTTFieldmasterChaosMovementComponent::ApplyFieldmasterDriveCommand(
    float SignedThrottle,
    float Steering,
    bool bHasFuel,
    float ConditionPercent,
    float TireIntegrity,
    float TerrainGripFactor)
{
    const float RequestedThrottle = FMath::Clamp(SignedThrottle, -1.0f, 1.0f);
    const float RequestedSteering = FMath::Clamp(Steering, -1.0f, 1.0f);
    const float Condition01 = NormalizeCondition(ConditionPercent);
    const float Tires01 = FMath::Clamp(TireIntegrity, 0.0f, 1.0f);
    const float Terrain01 = FMath::Clamp(TerrainGripFactor, 0.0f, 1.0f);

    DriveHealthFactor = bHasFuel
        ? FMath::Lerp(MinimumDamagedDriveFactor, 1.0f, Condition01)
        : 0.0f;
    SteeringGripFactor = FMath::Lerp(MinimumSteeringAuthority, 1.0f, Tires01 * Terrain01);

    if (!bFieldmasterConfigurationValid || !bHasFuel || Condition01 <= KINDA_SMALL_NUMBER)
    {
        HoldFieldmasterStopped();
        return;
    }

    EffectiveThrottle = FMath::Abs(RequestedThrottle) * DriveHealthFactor;
    EffectiveSteering = RequestedSteering * SteeringGripFactor;

    SetThrottleInput(EffectiveThrottle);
    SetSteeringInput(EffectiveSteering);
    SetBrakeInput(FMath::IsNearlyZero(RequestedThrottle, DirectionDeadZone) ? IdleBrakeInput : 0.0f);

    // Direction selection is intentionally NOT performed here. UGTTNativeDriveDynamicsSubsystem is
    // the single final drivetrain authority for Fieldmaster, Rattleback and Mulebox. Keeping
    // SetTargetGear out of the per-tick Fieldmaster command path lets Chaos automatic forward gears
    // upshift normally and prevents this component from bypassing the shared forward/reverse interlock.
}

void UGTTFieldmasterChaosMovementComponent::HoldFieldmasterStopped()
{
    EffectiveThrottle = 0.0f;
    EffectiveSteering = 0.0f;
    SetThrottleInput(0.0f);
    SetSteeringInput(0.0f);
    SetBrakeInput(1.0f);
}
