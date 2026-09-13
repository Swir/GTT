#include "Vehicles/GTTNativeChaosRuntimeGuardSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr float InvalidRuntimeGraceSeconds = 1.5f;
    constexpr float RuntimeEvidenceIntervalSeconds = 5.0f;
}

void UGTTNativeChaosRuntimeGuardSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        EvaluateFieldmaster(*It, DeltaTime);
    }
}

TStatId UGTTNativeChaosRuntimeGuardSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeChaosRuntimeGuardSubsystem, STATGROUP_Tickables);
}

bool UGTTNativeChaosRuntimeGuardSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativeChaosRuntimeGuardSubsystem::EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn)
    {
        return;
    }

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive())
    {
        InvalidRuntimeSeconds.Remove(Key);
        EvidenceLogSeconds.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    USkeletalMeshComponent* Mesh = NativePawn->GetMesh();
    const bool bPhysicsAssetPresent = Mesh && Mesh->GetPhysicsAsset() != nullptr;
    const bool bCollisionEnabled = Mesh && Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
    const bool bMovementActive = Movement && Movement->IsActive();
    const bool bRuntimeHealthy = NativePawn->IsNativeFieldmasterReady() && bPhysicsAssetPresent && bCollisionEnabled && bMovementActive;

    float& EvidenceSeconds = EvidenceLogSeconds.FindOrAdd(Key);
    EvidenceSeconds += DeltaTime;
    if (EvidenceSeconds >= RuntimeEvidenceIntervalSeconds)
    {
        EvidenceSeconds = 0.0f;
        const float SpeedKmh = NativePawn->GetVelocity().Size() * 0.036f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_CHAOS_EVIDENCE vehicle=RustyFieldmaster60 takeover=%s ready=%s movement=%s physics_asset=%s collision=%s occupied=%s speed_kmh=%.1f condition=%.1f fuel_l=%.1f"),
            NativePawn->IsLegacyTakeoverActive() ? TEXT("YES") : TEXT("NO"),
            NativePawn->IsNativeFieldmasterReady() ? TEXT("YES") : TEXT("NO"),
            bMovementActive ? TEXT("ACTIVE") : TEXT("INACTIVE"),
            bPhysicsAssetPresent ? TEXT("YES") : TEXT("NO"),
            bCollisionEnabled ? TEXT("YES") : TEXT("NO"),
            NativePawn->IsOccupied() ? TEXT("YES") : TEXT("NO"),
            SpeedKmh,
            NativePawn->GetMigrationSnapshot().ConditionPercent,
            NativePawn->GetMigrationSnapshot().FuelLiters);
    }

    if (bRuntimeHealthy)
    {
        InvalidRuntimeSeconds.Remove(Key);
        return;
    }

    float& InvalidSeconds = InvalidRuntimeSeconds.FindOrAdd(Key);
    InvalidSeconds += DeltaTime;
    if (InvalidSeconds < InvalidRuntimeGraceSeconds)
    {
        return;
    }

    UE_LOG(LogGTT, Error,
        TEXT("Native Fieldmaster runtime guard forced legacy fallback after %.2fs unhealthy runtime: ready=%s movement=%s physics_asset=%s collision=%s"),
        InvalidSeconds,
        NativePawn->IsNativeFieldmasterReady() ? TEXT("YES") : TEXT("NO"),
        bMovementActive ? TEXT("ACTIVE") : TEXT("INACTIVE"),
        bPhysicsAssetPresent ? TEXT("YES") : TEXT("NO"),
        bCollisionEnabled ? TEXT("YES") : TEXT("NO"));

    NativePawn->DeactivateLegacyTakeover();
    InvalidRuntimeSeconds.Remove(Key);
    EvidenceLogSeconds.Remove(Key);
}
