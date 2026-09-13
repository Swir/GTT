#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vehicles/GTTChaosRigContract.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
}

void AGTTFieldmasterNativePawn::ApplyNativeMudResponse(float DragStrength, float TireWearPerSecond, float DeltaSeconds)
{
    if (!bNativeReady || !bTakeoverActive || !GetMesh() || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const float ClampedDrag = FMath::Clamp(DragStrength, 0.0f, 6.0f);
    const FVector Velocity = GetVelocity();
    FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
    const float SpeedKmh = HorizontalVelocity.Size() * 0.036f;

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        const float TireUpgradeBonus = 1.0f + 0.12f * FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3);
        const float TireHealth = FMath::Clamp(MigrationSnapshot.TireIntegrity, 0.2f, 1.0f);
        const float MudTraction = FMath::Clamp((1.0f - ClampedDrag * 0.075f) * TireUpgradeBonus * TireHealth, 0.32f, 1.0f);
        Movement->SetThrottleInput(FMath::Abs(LastThrottleInput) * MudTraction);
        Movement->SetBrakeInput(FMath::IsNearlyZero(LastThrottleInput) ? FMath::Clamp(0.12f + ClampedDrag * 0.035f, 0.12f, 0.32f) : 0.0f);
    }

    if (GetMesh()->IsSimulatingPhysics() && !HorizontalVelocity.IsNearlyZero())
    {
        GetMesh()->AddForce(-HorizontalVelocity * GetMesh()->GetMass() * ClampedDrag * 0.22f, NAME_None, false);
    }

    if (SpeedKmh > 9.0f)
    {
        const float UpgradeWearReduction = 1.0f - 0.16f * FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3);
        MigrationSnapshot.TireIntegrity = FMath::Clamp(
            MigrationSnapshot.TireIntegrity - FMath::Max(0.0f, TireWearPerSecond) * UpgradeWearReduction * DeltaSeconds,
            0.0f,
            1.0f);
    }
}

void AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale)
{
    if (!bNativeReady || !bTakeoverActive || ImpactSpeedKmh < 12.0f)
    {
        return;
    }

    const float Severity = FMath::Clamp((ImpactSpeedKmh - 12.0f) / 55.0f, 0.0f, 1.75f);
    const float BodyDamage = Severity * 13.0f * FMath::Max(0.0f, DamageScale);
    MigrationSnapshot.ConditionPercent = FMath::Clamp(MigrationSnapshot.ConditionPercent - BodyDamage, 0.0f, 100.0f);

    if (ImpactSpeedKmh > 34.0f)
    {
        const float TireDamage = Severity * 0.045f * FMath::Max(0.0f, DamageScale);
        MigrationSnapshot.TireIntegrity = FMath::Clamp(MigrationSnapshot.TireIntegrity - TireDamage, 0.0f, 1.0f);
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
