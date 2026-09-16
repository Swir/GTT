#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float ImpactDamageCooldownSeconds = 0.30f;
    constexpr float MinimumImpactSpeedKmh = 12.0f;
    constexpr float SevereImpactSpeedKmh = 34.0f;
    constexpr float NativeMudHoldSeconds = 0.35f;
    constexpr float NativeMudReferenceDrag = 4.0f;
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
    const float TireHealth = FMath::Clamp(MigrationSnapshot.TireIntegrity, 0.20f, 1.0f);
    return FMath::Clamp((1.0f - MudSeverity * 0.48f) * TireHealth + TireLevelAssist, 0.32f, 1.0f);
}

void AGTTFieldmasterNativePawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

    if (!bNativeReady || !bTakeoverActive || !GetWorld() || Other == this)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastImpactDamageTimeSeconds < ImpactDamageCooldownSeconds)
    {
        return;
    }

    const float VehicleSpeedKmh = GetVelocity().Size() * 0.036f;
    float ImpulseEquivalentKmh = 0.0f;
    if (GetMesh() && GetMesh()->GetMass() > KINDA_SMALL_NUMBER)
    {
        ImpulseEquivalentKmh = (NormalImpulse.Size() / GetMesh()->GetMass()) * 0.036f;
    }

    const float ImpactSpeedKmh = FMath::Max(VehicleSpeedKmh, ImpulseEquivalentKmh);
    if (ImpactSpeedKmh < MinimumImpactSpeedKmh)
    {
        return;
    }

    LastImpactDamageTimeSeconds = Now;
    const float PreviousCondition = MigrationSnapshot.ConditionPercent;
    const float PreviousTires = MigrationSnapshot.TireIntegrity;
    ApplyNativeImpactDamage(ImpactSpeedKmh, 1.0f);

    if (!FMath::IsNearlyEqual(PreviousCondition, MigrationSnapshot.ConditionPercent) || !FMath::IsNearlyEqual(PreviousTires, MigrationSnapshot.TireIntegrity))
    {
        SyncLegacyMirror();
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_IMPACT_DAMAGE vehicle=RustyFieldmaster60 speed_kmh=%.1f condition=%.1f%% tire_integrity=%.2f condition_delta=%.1f%% tire_delta=%.3f"),
            ImpactSpeedKmh,
            MigrationSnapshot.ConditionPercent * 100.0f,
            MigrationSnapshot.TireIntegrity,
            (PreviousCondition - MigrationSnapshot.ConditionPercent) * 100.0f,
            PreviousTires - MigrationSnapshot.TireIntegrity);
    }
}

void AGTTFieldmasterNativePawn::ApplyNativeMudResponse(float DragStrength, float TireWearPerSecond, float DeltaSeconds)
{
    if (!bNativeReady || !bTakeoverActive || !GetMesh() || !GetWorld() || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const float ClampedDrag = FMath::Clamp(DragStrength, 0.0f, 6.0f);
    LastNativeMudResponseTimeSeconds = GetWorld()->GetTimeSeconds();
    LastNativeMudSeverity = FMath::Clamp(ClampedDrag / NativeMudReferenceDrag, 0.0f, 1.0f);

    const FVector Velocity = GetVelocity();
    FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
    const float SpeedKmh = HorizontalVelocity.Size() * 0.036f;
    const int32 TireLevel = FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3);
    const float TireUpgradeBonus = 1.0f + 0.12f * TireLevel;
    const float TerrainGrip = GetNativeTerrainGripFactor();
    const float MudTraction = FMath::Clamp(TerrainGrip * TireUpgradeBonus, 0.30f, 1.0f);

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(FMath::Abs(LastThrottleInput) * MudTraction);
        Movement->SetBrakeInput(FMath::IsNearlyZero(LastThrottleInput) ? FMath::Clamp(0.12f + ClampedDrag * 0.035f, 0.12f, 0.32f) : 0.0f);
    }

    if (GetMesh()->IsSimulatingPhysics() && !HorizontalVelocity.IsNearlyZero())
    {
        const float LoadDrag = FMath::Lerp(0.18f, 0.28f, LastNativeMudSeverity);
        GetMesh()->AddForce(-HorizontalVelocity * GetMesh()->GetMass() * ClampedDrag * LoadDrag, NAME_None, false);
    }

    if (SpeedKmh > 9.0f)
    {
        const float UpgradeWearReduction = 1.0f - 0.14f * TireLevel;
        const float SeverityWear = FMath::Lerp(0.70f, 1.25f, LastNativeMudSeverity);
        MigrationSnapshot.TireIntegrity = FMath::Clamp(
            MigrationSnapshot.TireIntegrity - FMath::Max(0.0f, TireWearPerSecond) * UpgradeWearReduction * SeverityWear * DeltaSeconds,
            0.0f,
            1.0f);
    }

    UE_LOG(LogGTT, VeryVerbose,
        TEXT("NATIVE_TERRAIN_RESPONSE vehicle=RustyFieldmaster60 mud=%.2f grip=%.2f throttle_limit=%.2f speed_kmh=%.1f tire_integrity=%.2f tire_level=%d"),
        GetNativeMudSeverity(), TerrainGrip, MudTraction, SpeedKmh, MigrationSnapshot.TireIntegrity, TireLevel);
}

void AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale)
{
    if (!bNativeReady || !bTakeoverActive || ImpactSpeedKmh < MinimumImpactSpeedKmh)
    {
        return;
    }

    const float Severity = FMath::Clamp((ImpactSpeedKmh - MinimumImpactSpeedKmh) / 55.0f, 0.0f, 1.75f);
    const float BodyDamageRatio = Severity * 0.13f * FMath::Max(0.0f, DamageScale);
    MigrationSnapshot.ConditionPercent = FMath::Clamp(MigrationSnapshot.ConditionPercent - BodyDamageRatio, 0.0f, 1.0f);

    if (ImpactSpeedKmh > SevereImpactSpeedKmh)
    {
        const float TireDamage = Severity * 0.045f * FMath::Max(0.0f, DamageScale);
        MigrationSnapshot.TireIntegrity = FMath::Clamp(MigrationSnapshot.TireIntegrity - TireDamage, 0.0f, 1.0f);
    }

    if (MigrationSnapshot.ConditionPercent <= KINDA_SMALL_NUMBER)
    {
        LastThrottleInput = 0.0f;
        if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
        {
            Movement->SetThrottleInput(0.0f);
            Movement->SetSteeringInput(0.0f);
            Movement->SetBrakeInput(1.0f);
        }
    }
}

bool AGTTFieldmasterNativePawn::TryGetRearHitchTransform(FTransform& OutTransform) const
{
    OutTransform = FTransform::Identity;
    if (!bNativeReady || !bTakeoverActive || !GetMesh())
    {
        return false;
    }

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig) || !Rig.bRequiresHitchSocket || Rig.HitchSocket.IsNone())
    {
        return false;
    }

    if (!GetMesh()->DoesSocketExist(Rig.HitchSocket))
    {
        return false;
    }

    OutTransform = GetMesh()->GetSocketTransform(Rig.HitchSocket, RTS_World);
    return true;
}
