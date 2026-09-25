#include "Vehicles/GTTNativeChaosRuntimeGuardSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr float InvalidRuntimeGraceSeconds = 1.5f;
    constexpr float RuntimeEvidenceIntervalSeconds = 5.0f;
    constexpr int32 RequiredWheelCount = 4;
}

void UGTTNativeChaosRuntimeGuardSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        EvaluateFieldmaster(*It, DeltaTime);
    }
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        EvaluateRoadVehicle(*It, DeltaTime);
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

bool UGTTNativeChaosRuntimeGuardSubsystem::EvaluateLiveChaosContract(
    APawn* NativePawn,
    UChaosWheeledVehicleMovementComponent* Movement,
    USkeletalMeshComponent* Mesh,
    int32& OutValidWheels,
    int32& OutGroundContacts,
    int32& OutSuspensionSamples,
    float& OutMinSuspension,
    float& OutMaxSuspension) const
{
    OutValidWheels = 0;
    OutGroundContacts = 0;
    OutSuspensionSamples = 0;
    OutMinSuspension = 1.0f;
    OutMaxSuspension = 0.0f;

    if (!NativePawn || !Movement || !Movement->IsActive()
        || Movement->GetNumWheels() < RequiredWheelCount || !Mesh || !Mesh->GetPhysicsAsset()
        || Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
    {
        return false;
    }

    for (int32 WheelIndex = 0; WheelIndex < RequiredWheelCount; ++WheelIndex)
    {
        const FWheelStatus WheelState = Movement->GetWheelState(WheelIndex);
        if (!WheelState.bIsValid) continue;

        ++OutValidWheels;
        if (WheelState.bInContact) ++OutGroundContacts;
        if (FMath::IsFinite(WheelState.NormalizedSuspensionLength)
            && WheelState.NormalizedSuspensionLength >= 0.0f
            && WheelState.NormalizedSuspensionLength <= 1.0f)
        {
            ++OutSuspensionSamples;
            OutMinSuspension = FMath::Min(OutMinSuspension, WheelState.NormalizedSuspensionLength);
            OutMaxSuspension = FMath::Max(OutMaxSuspension, WheelState.NormalizedSuspensionLength);
        }
    }

    return OutValidWheels == RequiredWheelCount && OutSuspensionSamples == RequiredWheelCount;
}

void UGTTNativeChaosRuntimeGuardSubsystem::EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn) return;
    if (!NativePawn->IsLegacyTakeoverActive())
    {
        ClearRuntimeState(NativePawn);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    USkeletalMeshComponent* Mesh = NativePawn->GetMesh();
    int32 ValidWheels = 0, GroundContacts = 0, SuspensionSamples = 0;
    float MinSuspension = 1.0f, MaxSuspension = 0.0f;
    const bool bLiveContract = EvaluateLiveChaosContract(NativePawn, Movement, Mesh, ValidWheels, GroundContacts,
        SuspensionSamples, MinSuspension, MaxSuspension);
    const bool bRuntimeHealthy = NativePawn->IsNativeFieldmasterReady() && bLiveContract;

    const TWeakObjectPtr<APawn> Key(NativePawn);
    float& EvidenceSeconds = EvidenceLogSeconds.FindOrAdd(Key);
    EvidenceSeconds += DeltaTime;
    if (EvidenceSeconds >= RuntimeEvidenceIntervalSeconds)
    {
        EvidenceSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_CHAOS_RUNTIME_ACCEPTANCE vehicle=RustyFieldmaster60 takeover=YES ready=%s movement=%s physics_asset=%s wheels=%d/4 contacts=%d suspension=%d/4 suspension_range=%.3f..%.3f accepted=%s"),
            NativePawn->IsNativeFieldmasterReady() ? TEXT("YES") : TEXT("NO"),
            Movement && Movement->IsActive() ? TEXT("ACTIVE") : TEXT("INACTIVE"),
            Mesh && Mesh->GetPhysicsAsset() ? TEXT("YES") : TEXT("NO"),
            ValidWheels, GroundContacts, SuspensionSamples, MinSuspension, MaxSuspension,
            bRuntimeHealthy ? TEXT("YES") : TEXT("NO"));
    }

    if (bRuntimeHealthy)
    {
        InvalidRuntimeSeconds.Remove(Key);
        return;
    }

    float& InvalidSeconds = InvalidRuntimeSeconds.FindOrAdd(Key);
    InvalidSeconds += DeltaTime;
    if (InvalidSeconds < InvalidRuntimeGraceSeconds) return;

    UE_LOG(LogGTT, Error,
        TEXT("NATIVE_CHAOS_RUNTIME_FALLBACK vehicle=RustyFieldmaster60 unhealthy_seconds=%.2f ready=%s wheels=%d/4 suspension=%d/4"),
        InvalidSeconds, NativePawn->IsNativeFieldmasterReady() ? TEXT("YES") : TEXT("NO"), ValidWheels, SuspensionSamples);
    NativePawn->DeactivateLegacyTakeover();
    ClearRuntimeState(NativePawn);
}

void UGTTNativeChaosRuntimeGuardSubsystem::EvaluateRoadVehicle(AGTTRoadVehicleNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn) return;
    if (!NativePawn->IsLegacyTakeoverActive())
    {
        ClearRuntimeState(NativePawn);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    USkeletalMeshComponent* Mesh = NativePawn->GetMesh();
    int32 ValidWheels = 0, GroundContacts = 0, SuspensionSamples = 0;
    float MinSuspension = 1.0f, MaxSuspension = 0.0f;
    const bool bLiveContract = EvaluateLiveChaosContract(NativePawn, Movement, Mesh, ValidWheels, GroundContacts,
        SuspensionSamples, MinSuspension, MaxSuspension);
    const bool bRuntimeHealthy = NativePawn->IsNativeReady() && bLiveContract;
    const TWeakObjectPtr<APawn> Key(NativePawn);

    float& EvidenceSeconds = EvidenceLogSeconds.FindOrAdd(Key);
    EvidenceSeconds += DeltaTime;
    if (EvidenceSeconds >= RuntimeEvidenceIntervalSeconds)
    {
        EvidenceSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_CHAOS_RUNTIME_ACCEPTANCE vehicle=%s takeover=YES ready=%s movement=%s physics_asset=%s wheels=%d/4 contacts=%d suspension=%d/4 suspension_range=%.3f..%.3f accepted=%s"),
            *NativePawn->GetPersistentVehicleId().ToString(), NativePawn->IsNativeReady() ? TEXT("YES") : TEXT("NO"),
            Movement && Movement->IsActive() ? TEXT("ACTIVE") : TEXT("INACTIVE"),
            Mesh && Mesh->GetPhysicsAsset() ? TEXT("YES") : TEXT("NO"), ValidWheels, GroundContacts, SuspensionSamples,
            MinSuspension, MaxSuspension, bRuntimeHealthy ? TEXT("YES") : TEXT("NO"));
    }

    if (bRuntimeHealthy)
    {
        InvalidRuntimeSeconds.Remove(Key);
        return;
    }

    float& InvalidSeconds = InvalidRuntimeSeconds.FindOrAdd(Key);
    InvalidSeconds += DeltaTime;
    if (InvalidSeconds < InvalidRuntimeGraceSeconds) return;

    UE_LOG(LogGTT, Error,
        TEXT("NATIVE_CHAOS_RUNTIME_FALLBACK vehicle=%s unhealthy_seconds=%.2f ready=%s wheels=%d/4 suspension=%d/4"),
        *NativePawn->GetPersistentVehicleId().ToString(), InvalidSeconds,
        NativePawn->IsNativeReady() ? TEXT("YES") : TEXT("NO"), ValidWheels, SuspensionSamples);
    NativePawn->DeactivateLegacyTakeover();
    ClearRuntimeState(NativePawn);
}

void UGTTNativeChaosRuntimeGuardSubsystem::ClearRuntimeState(APawn* NativePawn)
{
    if (!NativePawn) return;
    const TWeakObjectPtr<APawn> Key(NativePawn);
    InvalidRuntimeSeconds.Remove(Key);
    EvidenceLogSeconds.Remove(Key);
}
