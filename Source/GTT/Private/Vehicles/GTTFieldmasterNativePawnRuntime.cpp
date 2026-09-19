#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTFieldmasterChaosMovementComponent.h"

namespace
{
    const FName RuntimeFieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float NativeImpactCooldownSeconds = 0.30f;
    constexpr float NativeMinimumImpactSpeedKmh = 12.0f;
    constexpr float NativeHeavyImpactSpeedKmh = 95.0f;
    constexpr float NativeMaxImpactConditionLoss = 0.32f;
    constexpr float NativeMudResponseHoldSeconds = 0.45f;
    constexpr float NativeMudMinimumGrip = 0.38f;
    constexpr float NativeMudMaximumGripLoss = 0.46f;
}

void AGTTFieldmasterNativePawn::NotifyHit(
    UPrimitiveComponent* MyComp,
    AActor* Other,
    UPrimitiveComponent* OtherComp,
    bool bSelfMoved,
    FVector HitLocation,
    FVector HitNormal,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

    if (!bNativeReady || !bTakeoverActive || !GetWorld())
    {
        return;
    }

    const float NowSeconds = GetWorld()->GetTimeSeconds();
    if (NowSeconds - LastImpactDamageTimeSeconds < NativeImpactCooldownSeconds)
    {
        return;
    }

    const USkeletalMeshComponent* VehicleMesh = GetMesh();
    const float MassKg = VehicleMesh ? FMath::Max(VehicleMesh->GetMass(), 1.0f) : 1.0f;
    const float ImpulseSpeedKmh = (NormalImpulse.Size() / MassKg) * 0.036f;
    const float ActorSpeedKmh = GetVelocity().Size() * 0.036f;
    const float ImpactSpeedKmh = FMath::Max(ImpulseSpeedKmh, ActorSpeedKmh);

    if (ImpactSpeedKmh < NativeMinimumImpactSpeedKmh)
    {
        return;
    }

    LastImpactDamageTimeSeconds = NowSeconds;
    ApplyNativeImpactDamage(ImpactSpeedKmh);
}

void AGTTFieldmasterNativePawn::ApplyNativeMudResponse(
    float DragStrength,
    float TireWearPerSecond,
    float DeltaSeconds)
{
    if (!bNativeReady || !bTakeoverActive || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const float MudSeverity = FMath::Clamp(DragStrength, 0.0f, 1.0f);
    LastNativeMudSeverity = FMath::Max(LastNativeMudSeverity, MudSeverity);
    if (GetWorld())
    {
        LastNativeMudResponseTimeSeconds = GetWorld()->GetTimeSeconds();
    }

    if (USkeletalMeshComponent* VehicleMesh = GetMesh())
    {
        if (VehicleMesh->IsSimulatingPhysics())
        {
            FVector HorizontalVelocity = VehicleMesh->GetPhysicsLinearVelocity();
            HorizontalVelocity.Z = 0.0f;
            if (!HorizontalVelocity.IsNearlyZero())
            {
                VehicleMesh->AddForce(
                    -HorizontalVelocity * VehicleMesh->GetMass() * FMath::Max(DragStrength, 0.0f) * 0.35f,
                    NAME_None,
                    false);
            }

            if (HorizontalVelocity.SizeSquared() > FMath::Square(250.0f) && TireWearPerSecond > 0.0f)
            {
                const float TireReinforcement = 1.0f + FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3) * 0.35f;
                MigrationSnapshot.TireIntegrity = FMath::Clamp(
                    MigrationSnapshot.TireIntegrity - (TireWearPerSecond * DeltaSeconds) / TireReinforcement,
                    0.0f,
                    1.0f);
            }
        }
    }

    RefreshNativeDriveCommand();
}

void AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale)
{
    if (!bNativeReady || !bTakeoverActive || DamageScale <= 0.0f)
    {
        return;
    }

    const float NormalizedImpact = FMath::Clamp(
        (ImpactSpeedKmh - NativeMinimumImpactSpeedKmh) /
            FMath::Max(NativeHeavyImpactSpeedKmh - NativeMinimumImpactSpeedKmh, 1.0f),
        0.0f,
        1.0f);

    if (NormalizedImpact <= 0.0f)
    {
        return;
    }

    const float ConditionLoss = FMath::Clamp(
        FMath::Lerp(0.008f, NativeMaxImpactConditionLoss, NormalizedImpact) * DamageScale,
        0.0f,
        NativeMaxImpactConditionLoss);

    const float TireLoss = FMath::Clamp(
        ConditionLoss * FMath::Lerp(0.18f, 0.55f, NormalizedImpact),
        0.0f,
        0.16f);

    const float TireReinforcement = 1.0f + FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3) * 0.35f;
    MigrationSnapshot.ConditionPercent = FMath::Clamp(MigrationSnapshot.ConditionPercent - ConditionLoss, 0.0f, 1.0f);
    MigrationSnapshot.TireIntegrity = FMath::Clamp(
        MigrationSnapshot.TireIntegrity - TireLoss / TireReinforcement,
        0.0f,
        1.0f);

    RefreshNativeDriveCommand();
    SyncLegacyMirror();
}

float AGTTFieldmasterNativePawn::GetNativeMudSeverity() const
{
    if (!GetWorld() || LastNativeMudResponseTimeSeconds < 0.0f)
    {
        return 0.0f;
    }

    const float AgeSeconds = GetWorld()->GetTimeSeconds() - LastNativeMudResponseTimeSeconds;
    if (AgeSeconds >= NativeMudResponseHoldSeconds)
    {
        return 0.0f;
    }

    return FMath::Clamp(
        LastNativeMudSeverity * (1.0f - AgeSeconds / NativeMudResponseHoldSeconds),
        0.0f,
        1.0f);
}

float AGTTFieldmasterNativePawn::GetNativeTerrainGripFactor() const
{
    const float Severity = GetNativeMudSeverity();
    const float UpgradeProtection = FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3) * 0.05f;
    const float GripLoss = Severity * FMath::Max(0.12f, NativeMudMaximumGripLoss - UpgradeProtection);
    return FMath::Clamp(1.0f - GripLoss, NativeMudMinimumGrip, 1.0f);
}

bool AGTTFieldmasterNativePawn::TryGetRearHitchTransform(FTransform& OutTransform) const
{
    const USkeletalMeshComponent* VehicleMesh = GetMesh();
    if (!VehicleMesh)
    {
        return false;
    }

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(RuntimeFieldmasterVehicleId, Rig) ||
        Rig.HitchSocket.IsNone() ||
        !VehicleMesh->DoesSocketExist(Rig.HitchSocket))
    {
        return false;
    }

    OutTransform = VehicleMesh->GetSocketTransform(Rig.HitchSocket, RTS_World);
    return true;
}
