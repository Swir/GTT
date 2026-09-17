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

    LastRequestedSignedThrottle = RequestedThrottle;
    LastTerrainGripFactor = Terrain01;
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

    if (!FMath::IsNearlyZero(RequestedThrottle, DirectionDeadZone))
    {
        SetTargetGear(RequestedThrottle < 0.0f ? -1 : 1, true);
    }
}

void UGTTFieldmasterChaosMovementComponent::HoldFieldmasterStopped()
{
    LastRequestedSignedThrottle = 0.0f;
    EffectiveThrottle = 0.0f;
    EffectiveSteering = 0.0f;
    SetThrottleInput(0.0f);
    SetSteeringInput(0.0f);
    SetBrakeInput(1.0f);
}

FGTTFieldmasterRuntimeTelemetry UGTTFieldmasterChaosMovementComponent::CaptureRuntimeTelemetry()
{
    FGTTFieldmasterRuntimeTelemetry Telemetry;
    Telemetry.bConfigurationValid = bFieldmasterConfigurationValid;
    Telemetry.bMovementActive = IsActive();
    Telemetry.CurrentGear = GetCurrentGear();
    Telemetry.TargetGear = GetTargetGear();
    Telemetry.EngineRpm = GetEngineRotationSpeed();
    Telemetry.EngineMaxRpm = GetEngineMaxRotationSpeed();
    Telemetry.ForwardSpeedKmh = GetForwardSpeed() * 0.036f;
    Telemetry.RequestedSignedThrottle = LastRequestedSignedThrottle;
    Telemetry.EffectiveThrottle = EffectiveThrottle;
    Telemetry.EffectiveSteering = EffectiveSteering;
    Telemetry.DriveHealthFactor = DriveHealthFactor;
    Telemetry.SteeringGripFactor = SteeringGripFactor;
    Telemetry.TerrainGripFactor = LastTerrainGripFactor;

    float SuspensionMin = 1.0f;
    float SuspensionMax = 0.0f;
    const int32 WheelCount = GetNumWheels();
    Telemetry.Wheels.Reserve(WheelCount);

    for (int32 WheelIndex = 0; WheelIndex < WheelCount; ++WheelIndex)
    {
        const FWheelStatus& Status = GetWheelState(WheelIndex);
        FGTTFieldmasterWheelRuntimeTelemetry Wheel;
        Wheel.WheelIndex = WheelIndex;
        Wheel.bValid = Status.bIsValid;
        Wheel.bInContact = Status.bInContact;
        Wheel.bSlipping = Status.bIsSlipping;
        Wheel.bSkidding = Status.bIsSkidding;
        Wheel.NormalizedSuspensionLength = Status.NormalizedSuspensionLength;
        Wheel.SpringForce = Status.SpringForce;
        Wheel.SlipAngle = Status.SlipAngle;
        Wheel.SlipMagnitude = Status.SlipMagnitude;
        Wheel.SkidMagnitude = Status.SkidMagnitude;
        Wheel.DriveTorque = Status.DriveTorque;
        Wheel.BrakeTorque = Status.BrakeTorque;
        Telemetry.Wheels.Add(Wheel);

        if (!Status.bIsValid)
        {
            continue;
        }

        ++Telemetry.ValidWheelCount;
        if (Status.bInContact) ++Telemetry.ContactCount;
        if (Status.bIsSlipping) ++Telemetry.SlippingWheelCount;
        if (Status.bIsSkidding) ++Telemetry.SkiddingWheelCount;

        if (FMath::IsFinite(Status.NormalizedSuspensionLength) &&
            Status.NormalizedSuspensionLength >= 0.0f && Status.NormalizedSuspensionLength <= 1.0f)
        {
            ++Telemetry.SuspensionSampleCount;
            SuspensionMin = FMath::Min(SuspensionMin, Status.NormalizedSuspensionLength);
            SuspensionMax = FMath::Max(SuspensionMax, Status.NormalizedSuspensionLength);
        }

        Telemetry.TotalSpringForce += FMath::Abs(Status.SpringForce);
        Telemetry.MaxSlipMagnitude = FMath::Max(Telemetry.MaxSlipMagnitude, FMath::Abs(Status.SlipMagnitude));
        Telemetry.TotalDriveTorque += FMath::Abs(Status.DriveTorque);
        Telemetry.TotalBrakeTorque += FMath::Abs(Status.BrakeTorque);
    }

    if (Telemetry.SuspensionSampleCount > 0)
    {
        Telemetry.SuspensionMin = SuspensionMin;
        Telemetry.SuspensionMax = SuspensionMax;
    }

    return Telemetry;
}
