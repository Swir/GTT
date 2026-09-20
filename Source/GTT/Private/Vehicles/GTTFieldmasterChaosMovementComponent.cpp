#include "Vehicles/GTTFieldmasterChaosMovementComponent.h"

#include "EngineUtils.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTFarmTrailer.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float IdleBrakeInput = 0.15f;
    constexpr float DirectionDeadZone = 0.05f;
    constexpr float MinimumDamagedDriveFactor = 0.28f;
    constexpr float MinimumSteeringAuthority = 0.35f;
    constexpr float MaximumTowThrottlePenalty = 0.30f;
    constexpr float MaximumTowSteeringPenalty = 0.12f;

    // 0.1.62 Fieldmaster hill-haul control. These values are deliberately
    // conservative: the system can only reduce throttle or add braking; it
    // never exceeds the 0.1.59 heavy-haul throttle/steering authority caps.
    constexpr float MinimumTerrainThrottleAuthority = 0.52f;
    constexpr float HillControlMinimumGradeDegrees = 4.0f;
    constexpr float HillControlFullGradeDegrees = 12.0f;
    constexpr float HillControlMinimumTowLoad = 0.15f;
    constexpr float HillHoldMaxSpeedKmh = 2.5f;
    constexpr float HillHoldBrakeMin = 0.30f;
    constexpr float HillHoldBrakeMax = 0.65f;
    constexpr float DownhillTowBrakeStartSpeedKmh = 10.0f;
    constexpr float DownhillTowBrakeFullSpeedKmh = 28.0f;
    constexpr float DownhillTowBrakeMin = 0.18f;
    constexpr float DownhillTowBrakeMax = 0.46f;
    constexpr float DownhillTowThrottleDeadZone = 0.10f;

    float ResolveAttachedTrailerLoad(const UActorComponent* Component)
    {
        const AActor* Owner = Component ? Component->GetOwner() : nullptr;
        UWorld* World = Owner ? Owner->GetWorld() : nullptr;
        if (!Owner || !World)
        {
            return 0.0f;
        }

        float StrongestLoad = 0.0f;
        for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
        {
            const AGTTFarmTrailer* Trailer = *It;
            if (!Trailer || !Trailer->IsAttached() || Trailer->GetTowActor() != Owner)
            {
                continue;
            }
            StrongestLoad = FMath::Max(StrongestLoad, Trailer->GetTowLoadFactor());
        }
        return FMath::Clamp(StrongestLoad, 0.0f, 1.0f);
    }

    float ResolveGradeAlpha(float GradeDegrees)
    {
        return FMath::Clamp(
            (FMath::Abs(GradeDegrees) - HillControlMinimumGradeDegrees) /
                (HillControlFullGradeDegrees - HillControlMinimumGradeDegrees),
            0.0f,
            1.0f);
    }
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

    TowLoadFactor = ResolveAttachedTrailerLoad(this);
    TowThrottleAuthority = 1.0f - TowLoadFactor * MaximumTowThrottlePenalty;
    TowSteeringAuthority = 1.0f - TowLoadFactor * MaximumTowSteeringPenalty;

    DriveHealthFactor = bHasFuel
        ? FMath::Lerp(MinimumDamagedDriveFactor, 1.0f, Condition01)
        : 0.0f;
    SteeringGripFactor = FMath::Lerp(MinimumSteeringAuthority, 1.0f, Tires01 * Terrain01);

    // Low-grip terrain now affects propulsion as well as steering. Square-root
    // shaping retains useful crawl torque while preventing full throttle on mud
    // or badly damaged tires. A healthy tractor on firm ground remains at 1.0.
    const float CombinedTraction = FMath::Sqrt(FMath::Clamp(Tires01 * Terrain01, 0.0f, 1.0f));
    TerrainThrottleAuthority = FMath::Lerp(MinimumTerrainThrottleAuthority, 1.0f, CombinedTraction);

    TravelGradeDegrees = 0.0f;
    HillHaulBrake = 0.0f;
    bHillHoldActive = false;
    bDownhillTowBrakeActive = false;

    const AActor* Owner = GetOwner();
    float AbsoluteSpeedKmh = 0.0f;
    if (Owner)
    {
        const FVector Forward = Owner->GetActorForwardVector();
        const float SignedSpeedKmh = FVector::DotProduct(Owner->GetVelocity(), Forward) * 0.036f;
        AbsoluteSpeedKmh = FMath::Abs(SignedSpeedKmh);

        const float ForwardGradeDegrees = FMath::RadiansToDegrees(
            FMath::Asin(FMath::Clamp(Forward.Z, -1.0f, 1.0f)));
        float TravelDirection = 1.0f;
        if (AbsoluteSpeedKmh > 0.5f)
        {
            TravelDirection = SignedSpeedKmh < 0.0f ? -1.0f : 1.0f;
        }
        else if (FMath::Abs(RequestedThrottle) > DirectionDeadZone)
        {
            TravelDirection = RequestedThrottle < 0.0f ? -1.0f : 1.0f;
        }
        TravelGradeDegrees = ForwardGradeDegrees * TravelDirection;
    }

    const float GradeAlpha = ResolveGradeAlpha(TravelGradeDegrees);
    const bool bTowLoaded = TowLoadFactor >= HillControlMinimumTowLoad;
    const bool bThrottleReleased = FMath::Abs(RequestedThrottle) <= DownhillTowThrottleDeadZone;

    // Loaded hill hold prevents the trailer from pulling a stopped/near-stopped
    // tractor backwards when the driver releases the throttle on a grade.
    if (bTowLoaded && bThrottleReleased && FMath::Abs(TravelGradeDegrees) >= HillControlMinimumGradeDegrees &&
        AbsoluteSpeedKmh <= HillHoldMaxSpeedKmh)
    {
        bHillHoldActive = true;
        const float LoadScale = FMath::Lerp(0.75f, 1.0f, TowLoadFactor);
        HillHaulBrake = FMath::Lerp(HillHoldBrakeMin, HillHoldBrakeMax, GradeAlpha) * LoadScale;
    }

    // When a loaded trailer is already descending, throttle-off driving receives
    // proportional tow braking. The effect grows with both grade and speed and
    // remains bounded so the shared drivetrain/axle composer can still apply the
    // stricter brake result later in the frame.
    if (bTowLoaded && bThrottleReleased && TravelGradeDegrees <= -HillControlMinimumGradeDegrees &&
        AbsoluteSpeedKmh >= DownhillTowBrakeStartSpeedKmh)
    {
        bDownhillTowBrakeActive = true;
        const float SpeedAlpha = FMath::Clamp(
            (AbsoluteSpeedKmh - DownhillTowBrakeStartSpeedKmh) /
                (DownhillTowBrakeFullSpeedKmh - DownhillTowBrakeStartSpeedKmh),
            0.0f,
            1.0f);
        const float TowBrakeSeverity = FMath::Max(GradeAlpha, SpeedAlpha);
        const float DownhillBrake = FMath::Lerp(DownhillTowBrakeMin, DownhillTowBrakeMax, TowBrakeSeverity) * TowLoadFactor;
        HillHaulBrake = FMath::Max(HillHaulBrake, DownhillBrake);
    }

    if (!bFieldmasterConfigurationValid || !bHasFuel || Condition01 <= KINDA_SMALL_NUMBER)
    {
        HoldFieldmasterStopped();
        return;
    }

    // Preserve the 0.1.59 heavy-haul authority as the baseline, then allow
    // terrain to reduce it further. This keeps full-load firm-road authority
    // exactly at the established 70% ceiling while mud/poor tires stay stricter.
    EffectiveThrottle = FMath::Abs(RequestedThrottle) * DriveHealthFactor * TowThrottleAuthority;
    EffectiveThrottle *= TerrainThrottleAuthority;
    EffectiveSteering = RequestedSteering * SteeringGripFactor * TowSteeringAuthority;

    SetThrottleInput(EffectiveThrottle);
    SetSteeringInput(EffectiveSteering);
    const float BaseBrake = FMath::IsNearlyZero(RequestedThrottle, DirectionDeadZone) ? IdleBrakeInput : 0.0f;
    SetBrakeInput(FMath::Clamp(FMath::Max(BaseBrake, HillHaulBrake), 0.0f, 1.0f));

    // Direction selection is intentionally NOT performed here. UGTTNativeDriveDynamicsSubsystem is
    // the single final drivetrain authority for Fieldmaster, Rattleback and Mulebox. Keeping
    // SetTargetGear out of the per-tick Fieldmaster command path lets Chaos automatic forward gears
    // upshift normally and prevents this component from bypassing the shared forward/reverse interlock.
}

void UGTTFieldmasterChaosMovementComponent::HoldFieldmasterStopped()
{
    EffectiveThrottle = 0.0f;
    EffectiveSteering = 0.0f;
    TerrainThrottleAuthority = 0.0f;
    HillHaulBrake = 0.0f;
    TravelGradeDegrees = 0.0f;
    bHillHoldActive = false;
    bDownhillTowBrakeActive = false;
    SetThrottleInput(0.0f);
    SetSteeringInput(0.0f);
    SetBrakeInput(1.0f);
}
