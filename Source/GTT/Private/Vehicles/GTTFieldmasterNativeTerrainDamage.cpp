#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GTT.h"

namespace
{
    constexpr float NativeMudHoldSeconds = 0.35f;
    constexpr float NativeMudReferenceDrag = 4.0f;
    constexpr float NativeMudMinimumThrottle = 0.34f;
    constexpr float NativeMudWearSpeedKmh = 8.0f;
    constexpr float NativeImpactThresholdKmh = 18.0f;
    constexpr float NativeImpactSevereKmh = 42.0f;
    constexpr float NativeImpactCooldownSeconds = 0.22f;
}

float AGTTFieldmasterNativePawn::GetNativeMudSeverity() const
{
    const UWorld* World = GetWorld();
    if (!World || World->GetTimeSeconds() - LastNativeMudResponseTimeSeconds > NativeMudHoldSeconds)
    {
        return 0.0f;
    }
    return FMath::Clamp(LastNativeMudSeverity, 0.0f, 1.0f);
}

float AGTTFieldmasterNativePawn::GetNativeTerrainGripFactor() const
{
    const float MudSeverity = GetNativeMudSeverity();
    const float TireLevelAssist = static_cast<float>(FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3)) * 0.035f;
    return FMath::Clamp(1.0f - MudSeverity * 0.48f + TireLevelAssist, 0.42f, 1.0f);
}

void AGTTFieldmasterNativePawn::ApplyNativeMudResponse(float DragStrength, float TireWearPerSecond, float DeltaSeconds)
{
    if (!bNativeReady || !bTakeoverActive || DeltaSeconds <= 0.0f)
    {
        return;
    }

    UWorld* World = GetWorld();
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
    USkeletalMeshComponent* VehicleMesh = GetMesh();
    if (!World || !Movement || !VehicleMesh)
    {
        return;
    }

    const float ClampedDrag = FMath::Max(0.0f, DragStrength);
    const float MudSeverity = FMath::Clamp(ClampedDrag / NativeMudReferenceDrag, 0.0f, 1.0f);
    LastNativeMudResponseTimeSeconds = World->GetTimeSeconds();
    LastNativeMudSeverity = MudSeverity;

    const int32 TireLevel = FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3);
    const float TireAssist = static_cast<float>(TireLevel) * 0.055f;
    const float TerrainGrip = GetNativeTerrainGripFactor();
    const float ThrottleLimit = FMath::Clamp(0.92f - MudSeverity * 0.58f + TireAssist, NativeMudMinimumThrottle, 0.94f);

    if (bOccupied && MigrationSnapshot.FuelLiters > KINDA_SMALL_NUMBER)
    {
        Movement->SetThrottleInput(FMath::Abs(LastThrottleInput) * ThrottleLimit);
    }

    FVector HorizontalVelocity = VehicleMesh->GetPhysicsLinearVelocity();
    HorizontalVelocity.Z = 0.0f;
    if (VehicleMesh->IsSimulatingPhysics() && !HorizontalVelocity.IsNearlyZero())
    {
        const float MassKg = FMath::Max(1.0f, VehicleMesh->GetMass());
        const float DragForceScale = ClampedDrag * FMath::Lerp(0.24f, 0.12f, TireAssist);
        VehicleMesh->AddForce(-HorizontalVelocity * MassKg * DragForceScale, NAME_None, false);
    }

    const float SpeedKmh = HorizontalVelocity.Size() * 0.036f;
    if (SpeedKmh >= NativeMudWearSpeedKmh && TireWearPerSecond > 0.0f)
    {
        const float UpgradeWearReduction = 1.0f - static_cast<float>(TireLevel) * 0.10f;
        const float LoadWear = FMath::Lerp(0.65f, 1.20f, MudSeverity);
        MigrationSnapshot.TireIntegrity = FMath::Clamp(
            MigrationSnapshot.TireIntegrity - TireWearPerSecond * DeltaSeconds * UpgradeWearReduction * LoadWear,
            0.0f,
            1.0f);
    }

    UE_LOG(LogGTT, VeryVerbose,
        TEXT("NATIVE_TERRAIN_RESPONSE vehicle=RustyFieldmaster60 mud=%.2f grip=%.2f throttle_limit=%.2f speed_kmh=%.1f tire_integrity=%.2f tire_level=%d"),
        MudSeverity, TerrainGrip, ThrottleLimit, SpeedKmh, MigrationSnapshot.TireIntegrity, TireLevel);
}

void AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale)
{
    UWorld* World = GetWorld();
    if (!World || !bTakeoverActive || ImpactSpeedKmh < NativeImpactThresholdKmh || DamageScale <= 0.0f)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();
    if (Now - LastImpactDamageTimeSeconds < NativeImpactCooldownSeconds)
    {
        return;
    }
    LastImpactDamageTimeSeconds = Now;

    const float NormalizedImpact = FMath::Clamp((ImpactSpeedKmh - NativeImpactThresholdKmh) / 54.0f, 0.0f, 1.0f);
    const float ConditionDamage = FMath::Lerp(2.0f, 18.0f, NormalizedImpact) * DamageScale;
    MigrationSnapshot.ConditionPercent = FMath::Clamp(MigrationSnapshot.ConditionPercent - ConditionDamage, 0.0f, 100.0f);

    if (ImpactSpeedKmh >= NativeImpactSevereKmh)
    {
        const float TireDamage = FMath::Clamp(0.025f + NormalizedImpact * 0.10f, 0.0f, 0.14f) * DamageScale;
        MigrationSnapshot.TireIntegrity = FMath::Clamp(MigrationSnapshot.TireIntegrity - TireDamage, 0.0f, 1.0f);
    }

    if (MigrationSnapshot.ConditionPercent <= KINDA_SMALL_NUMBER)
    {
        LastThrottleInput = 0.0f;
        if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
        {
            Movement->SetThrottleInput(0.0f);
            Movement->SetBrakeInput(1.0f);
        }
    }

    SyncLegacyMirror();
    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_IMPACT_DAMAGE vehicle=RustyFieldmaster60 speed_kmh=%.1f condition=%.1f tire_integrity=%.2f scale=%.2f"),
        ImpactSpeedKmh, MigrationSnapshot.ConditionPercent, MigrationSnapshot.TireIntegrity, DamageScale);
}

void AGTTFieldmasterNativePawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

    if (!bTakeoverActive || Other == this)
    {
        return;
    }

    const float ChassisSpeedKmh = GetVelocity().Size() * 0.036f;
    float ImpulseSpeedKmh = 0.0f;
    if (USkeletalMeshComponent* VehicleMesh = GetMesh())
    {
        const float MassKg = FMath::Max(1.0f, VehicleMesh->GetMass());
        ImpulseSpeedKmh = NormalImpulse.Size() / MassKg * 0.036f;
    }

    const float ImpactSpeedKmh = FMath::Max(ChassisSpeedKmh, ImpulseSpeedKmh);
    ApplyNativeImpactDamage(ImpactSpeedKmh, 1.0f);
}
