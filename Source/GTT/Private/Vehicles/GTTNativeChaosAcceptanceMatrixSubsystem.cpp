#include "Vehicles/GTTNativeChaosAcceptanceMatrixSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr int32 RequiredWheelCount = 4;
    constexpr float EvidenceIntervalSeconds = 5.0f;
    constexpr float SmokeReadySeconds = 20.0f;
    constexpr float RequiredSuspensionTravel = 0.02f;
}

void UGTTNativeChaosAcceptanceMatrixSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* Pawn = *It;
        EvaluatePawn(Pawn, Pawn->GetPersistentVehicleId(), Pawn->IsLegacyTakeoverActive(), Pawn->IsNativeFieldmasterReady(), DeltaTime);
    }

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Pawn = *It;
        EvaluatePawn(Pawn, Pawn->GetPersistentVehicleId(), Pawn->IsLegacyTakeoverActive(), Pawn->IsNativeReady(), DeltaTime);
    }
}

TStatId UGTTNativeChaosAcceptanceMatrixSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeChaosAcceptanceMatrixSubsystem, STATGROUP_Tickables);
}

bool UGTTNativeChaosAcceptanceMatrixSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

bool UGTTNativeChaosAcceptanceMatrixSubsystem::EvaluateLiveMatrix(
    APawn* Pawn,
    FName VehicleId,
    bool bPawnReady,
    UChaosWheeledVehicleMovementComponent* Movement,
    USkeletalMeshComponent* Mesh,
    int32& OutValidWheels,
    int32& OutGroundContacts,
    int32& OutSuspensionSamples,
    float& OutMinSuspension,
    float& OutMaxSuspension,
    bool& bOutWheelConfig,
    bool& bOutPowertrainConfig,
    FString& OutWheelSummary,
    FString& OutPowertrainSummary) const
{
    OutValidWheels = 0;
    OutGroundContacts = 0;
    OutSuspensionSamples = 0;
    OutMinSuspension = 1.0f;
    OutMaxSuspension = 0.0f;
    bOutWheelConfig = false;
    bOutPowertrainConfig = false;

    if (!Pawn || VehicleId.IsNone() || !Movement || !Movement->IsActive()
        || Movement->GetNumWheels() < RequiredWheelCount || !Mesh || !Mesh->GetPhysicsAsset()
        || Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
    {
        OutWheelSummary = TEXT("live movement/mesh contract unavailable");
        OutPowertrainSummary = TEXT("live movement/mesh contract unavailable");
        return false;
    }

    bOutWheelConfig = UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(Movement, VehicleId, OutWheelSummary);
    bOutPowertrainConfig = UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(Movement, VehicleId, OutPowertrainSummary);

    for (int32 WheelIndex = 0; WheelIndex < RequiredWheelCount; ++WheelIndex)
    {
        const FWheelStatus Wheel = Movement->GetWheelState(WheelIndex);
        if (!Wheel.bIsValid) continue;

        ++OutValidWheels;
        if (Wheel.bInContact) ++OutGroundContacts;
        if (FMath::IsFinite(Wheel.NormalizedSuspensionLength)
            && Wheel.NormalizedSuspensionLength >= 0.0f
            && Wheel.NormalizedSuspensionLength <= 1.0f)
        {
            ++OutSuspensionSamples;
            OutMinSuspension = FMath::Min(OutMinSuspension, Wheel.NormalizedSuspensionLength);
            OutMaxSuspension = FMath::Max(OutMaxSuspension, Wheel.NormalizedSuspensionLength);
        }
    }

    return bPawnReady
        && bOutWheelConfig
        && bOutPowertrainConfig
        && OutValidWheels == RequiredWheelCount
        && OutSuspensionSamples == RequiredWheelCount;
}

void UGTTNativeChaosAcceptanceMatrixSubsystem::EvaluatePawn(
    APawn* Pawn,
    FName VehicleId,
    bool bTakeoverActive,
    bool bPawnReady,
    float DeltaTime)
{
    if (!Pawn) return;
    const TWeakObjectPtr<APawn> Key(Pawn);
    if (!bTakeoverActive)
    {
        States.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(
        Cast<AWheeledVehiclePawn>(Pawn) ? Cast<AWheeledVehiclePawn>(Pawn)->GetVehicleMovementComponent() : nullptr);
    USkeletalMeshComponent* Mesh = Cast<AWheeledVehiclePawn>(Pawn) ? Cast<AWheeledVehiclePawn>(Pawn)->GetMesh() : nullptr;

    int32 ValidWheels = 0;
    int32 GroundContacts = 0;
    int32 SuspensionSamples = 0;
    float MinSuspension = 1.0f;
    float MaxSuspension = 0.0f;
    bool bWheelConfig = false;
    bool bPowertrainConfig = false;
    FString WheelSummary;
    FString PowertrainSummary;
    const bool bAccepted = EvaluateLiveMatrix(Pawn, VehicleId, bPawnReady, Movement, Mesh,
        ValidWheels, GroundContacts, SuspensionSamples, MinSuspension, MaxSuspension,
        bWheelConfig, bPowertrainConfig, WheelSummary, PowertrainSummary);

    FGTTNativeChaosAcceptanceState& State = States.FindOrAdd(Key);
    State.EvidenceSeconds += DeltaTime;
    if (bAccepted)
    {
        State.ConsecutiveAcceptedSeconds += DeltaTime;
        State.PeakGroundContacts = FMath::Max(State.PeakGroundContacts, GroundContacts);
        State.MinObservedSuspension = FMath::Min(State.MinObservedSuspension, MinSuspension);
        State.MaxObservedSuspension = FMath::Max(State.MaxObservedSuspension, MaxSuspension);
    }
    else
    {
        State.ConsecutiveAcceptedSeconds = 0.0f;
        State.MinObservedSuspension = 1.0f;
        State.MaxObservedSuspension = 0.0f;
        State.PeakGroundContacts = 0;
        State.bSmokeReadyReported = false;
    }

    const float ObservedTravel = FMath::Max(0.0f, State.MaxObservedSuspension - State.MinObservedSuspension);
    const bool bRuntimeSmokeReady = bAccepted
        && State.ConsecutiveAcceptedSeconds >= SmokeReadySeconds
        && State.PeakGroundContacts >= 2
        && ObservedTravel >= RequiredSuspensionTravel;

    if (State.EvidenceSeconds >= EvidenceIntervalSeconds)
    {
        State.EvidenceSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_CHAOS_ACCEPTANCE_MATRIX vehicle=%s ready=%s wheels_cfg=%s powertrain_cfg=%s physics_asset=%s movement=%s wheels=%d/4 contacts=%d suspension=%d/4 observed_travel=%.3f stable_seconds=%.1f accepted=%s smoke_ready=%s"),
            *VehicleId.ToString(), bPawnReady ? TEXT("YES") : TEXT("NO"),
            bWheelConfig ? TEXT("PASS") : TEXT("FAIL"), bPowertrainConfig ? TEXT("PASS") : TEXT("FAIL"),
            Mesh && Mesh->GetPhysicsAsset() ? TEXT("YES") : TEXT("NO"), Movement && Movement->IsActive() ? TEXT("ACTIVE") : TEXT("INACTIVE"),
            ValidWheels, GroundContacts, SuspensionSamples, ObservedTravel, State.ConsecutiveAcceptedSeconds,
            bAccepted ? TEXT("YES") : TEXT("NO"), bRuntimeSmokeReady ? TEXT("YES") : TEXT("NO"));
    }

    if (bRuntimeSmokeReady && !State.bSmokeReadyReported)
    {
        State.bSmokeReadyReported = true;
        UE_LOG(LogGTT, Display,
            TEXT("NATIVE_CHAOS_SMOKE_READY vehicle=%s stable_seconds=%.1f peak_contacts=%d suspension_travel=%.3f wheels=%s powertrain=%s"),
            *VehicleId.ToString(), State.ConsecutiveAcceptedSeconds, State.PeakGroundContacts, ObservedTravel,
            *WheelSummary, *PowertrainSummary);
    }
}
