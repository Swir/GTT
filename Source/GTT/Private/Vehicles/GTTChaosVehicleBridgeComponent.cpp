#include "Vehicles/GTTChaosVehicleBridgeComponent.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Vehicles/GTTVehicleDynamicsComponent.h"
#include "GTT.h"

UGTTChaosVehicleBridgeComponent::UGTTChaosVehicleBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.0f;
}

void UGTTChaosVehicleBridgeComponent::BeginPlay()
{
    Super::BeginPlay();
    VehicleOwner = Cast<AGTTVehicleBase>(GetOwner());
    ResolveSpec();
    ResolveRigContract();
    RefreshNativeBinding();
}

void UGTTChaosVehicleBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!VehicleOwner)
    {
        BridgeState = EGTTChaosBridgeState::NativeBlocked;
        return;
    }

    RebindCountdown -= DeltaTime;
    if ((!NativeMovement || !bNativeMovementReady) && RebindCountdown <= 0.0f)
    {
        RefreshNativeBinding();
        RebindCountdown = 1.0f;
    }

    UpdateRuntimeState();
    if (bNativeMovementReady)
    {
        RouteInputsToChaos();
    }
}

void UGTTChaosVehicleBridgeComponent::CaptureThrottleInput(float Value)
{
    RawThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void UGTTChaosVehicleBridgeComponent::CaptureSteeringInput(float Value)
{
    RawSteeringInput = FMath::Clamp(Value, -1.0f, 1.0f);
}

void UGTTChaosVehicleBridgeComponent::ResolveSpec()
{
    ResolvedSpec = FGTTChaosVehicleSpec();
    if (!VehicleOwner)
    {
        return;
    }

    if (!UGTTVehicleChaosSpecLibrary::GetSpecForVehicleId(VehicleOwner->GetPersistentVehicleId(), ResolvedSpec))
    {
        UE_LOG(LogGTT, Warning, TEXT("Chaos bridge has no canonical spec for %s"), *VehicleOwner->GetPersistentVehicleId().ToString());
    }
}

void UGTTChaosVehicleBridgeComponent::ResolveRigContract()
{
    ResolvedRigContract = FGTTChaosRigContract();
    if (!VehicleOwner)
    {
        return;
    }

    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(VehicleOwner->GetPersistentVehicleId(), ResolvedRigContract))
    {
        UE_LOG(LogGTT, Warning, TEXT("Chaos bridge has no rig contract for %s"), *VehicleOwner->GetPersistentVehicleId().ToString());
    }
}

bool UGTTChaosVehicleBridgeComponent::ValidateNativeRig()
{
    bRigContractValid = false;
    RigValidationSummary = TEXT("No native skeletal rig bound");

    if (!NativeSkeletalBody)
    {
        return false;
    }

    if (ResolvedRigContract.VehicleId.IsNone())
    {
        RigValidationSummary = TEXT("No rig contract resolved");
        return false;
    }

    TArray<FName> MissingBones;
    for (const FName BoneName : UGTTChaosRigContractLibrary::GetRequiredBoneNames(ResolvedRigContract))
    {
        if (BoneName.IsNone() || NativeSkeletalBody->GetBoneIndex(BoneName) == INDEX_NONE)
        {
            MissingBones.Add(BoneName);
        }
    }

    TArray<FName> MissingSockets;
    for (const FName SocketName : UGTTChaosRigContractLibrary::GetRequiredSocketNames(ResolvedRigContract))
    {
        if (SocketName.IsNone() || !NativeSkeletalBody->DoesSocketExist(SocketName))
        {
            MissingSockets.Add(SocketName);
        }
    }

    if (MissingBones.Num() > 0 || MissingSockets.Num() > 0)
    {
        TArray<FString> Problems;
        for (const FName BoneName : MissingBones)
        {
            Problems.Add(FString::Printf(TEXT("bone:%s"), *BoneName.ToString()));
        }
        for (const FName SocketName : MissingSockets)
        {
            Problems.Add(FString::Printf(TEXT("socket:%s"), *SocketName.ToString()));
        }
        RigValidationSummary = FString::Printf(TEXT("Missing %s"), *FString::Join(Problems, TEXT(", ")));
        return false;
    }

    bRigContractValid = true;
    RigValidationSummary = TEXT("Rig contract valid");
    return true;
}

void UGTTChaosVehicleBridgeComponent::RefreshNativeBinding()
{
    if (!VehicleOwner)
    {
        VehicleOwner = Cast<AGTTVehicleBase>(GetOwner());
    }

    if (VehicleOwner && ResolvedSpec.VehicleId.IsNone())
    {
        ResolveSpec();
    }
    if (VehicleOwner && ResolvedRigContract.VehicleId.IsNone())
    {
        ResolveRigContract();
    }

    NativeMovement = VehicleOwner ? VehicleOwner->FindComponentByClass<UChaosWheeledVehicleMovementComponent>() : nullptr;
    LegacyDynamics = VehicleOwner ? VehicleOwner->FindComponentByClass<UGTTVehicleDynamicsComponent>() : nullptr;
    NativeSkeletalBody = VehicleOwner ? VehicleOwner->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

    const bool bHasSpec = ResolvedSpec.VehicleId != NAME_None;
    const bool bRigValid = ValidateNativeRig();
    bNativeMovementReady = NativeMovement != nullptr && bHasSpec && bRigValid;
    BridgeState = bNativeMovementReady ? EGTTChaosBridgeState::NativeReady : EGTTChaosBridgeState::WaitingForNativeRig;

    if (bNativeMovementReady)
    {
        DisableLegacyDynamicsIfNeeded();
        UE_LOG(LogGTT, Log, TEXT("Chaos bridge native rig accepted for %s (%s)"), *ResolvedSpec.VehicleId.ToString(), *RigValidationSummary);
    }
    else if (NativeMovement || NativeSkeletalBody)
    {
        UE_LOG(LogGTT, Warning, TEXT("Chaos bridge native rig rejected for %s: movement=%s rig=%s"),
            ResolvedSpec.VehicleId.IsNone() ? TEXT("NO SPEC") : *ResolvedSpec.VehicleId.ToString(),
            NativeMovement ? TEXT("YES") : TEXT("NO"),
            *RigValidationSummary);
    }
}

void UGTTChaosVehicleBridgeComponent::UpdateRuntimeState()
{
    if (!VehicleOwner)
    {
        BridgeState = EGTTChaosBridgeState::NativeBlocked;
        EffectivePowerScale = 0.0f;
        EffectiveGripScale = 0.0f;
        return;
    }

    const float ConditionPower = FMath::Lerp(0.35f, 1.0f, VehicleOwner->GetConditionPercent());
    const float EngineTunePower = 1.0f + VehicleOwner->GetEngineUpgradeLevel() * 0.12f;
    EffectivePowerScale = ConditionPower * EngineTunePower;

    const float TireConditionGrip = FMath::Lerp(0.30f, 1.0f, VehicleOwner->GetTireIntegrity());
    const float TireTuneGrip = 1.0f + VehicleOwner->GetTireUpgradeLevel() * 0.08f;
    EffectiveGripScale = TireConditionGrip * TireTuneGrip;

    if (!bNativeMovementReady)
    {
        BridgeState = EGTTChaosBridgeState::WaitingForNativeRig;
        return;
    }

    BridgeState = VehicleOwner->IsOccupied() && VehicleOwner->IsEngineRunning()
        ? EGTTChaosBridgeState::NativeDriving
        : EGTTChaosBridgeState::NativeReady;
}

void UGTTChaosVehicleBridgeComponent::RouteInputsToChaos()
{
    if (!NativeMovement || !VehicleOwner)
    {
        return;
    }

    const bool bCanDrive = VehicleOwner->IsOccupied() && VehicleOwner->IsEngineRunning() &&
        VehicleOwner->GetFuelLiters() > KINDA_SMALL_NUMBER && VehicleOwner->GetConditionPercent() > 0.0f;

    if (!bCanDrive)
    {
        NativeMovement->SetThrottleInput(0.0f);
        NativeMovement->SetSteeringInput(0.0f);
        NativeMovement->SetBrakeInput(1.0f);
        return;
    }

    const float ScaledSteering = FMath::Clamp(RawSteeringInput * EffectiveGripScale, -1.0f, 1.0f);
    const float ScaledThrottle = FMath::Clamp(FMath::Abs(RawThrottleInput) * EffectivePowerScale, 0.0f, 1.0f);

    if (RawThrottleInput < -0.05f)
    {
        NativeMovement->SetTargetGear(-1, true);
        NativeMovement->SetThrottleInput(ScaledThrottle);
        NativeMovement->SetBrakeInput(0.0f);
    }
    else if (RawThrottleInput > 0.05f)
    {
        NativeMovement->SetTargetGear(1, false);
        NativeMovement->SetThrottleInput(ScaledThrottle);
        NativeMovement->SetBrakeInput(0.0f);
    }
    else
    {
        NativeMovement->SetThrottleInput(0.0f);
        NativeMovement->SetBrakeInput(0.18f);
    }

    NativeMovement->SetSteeringInput(ScaledSteering);
}

void UGTTChaosVehicleBridgeComponent::DisableLegacyDynamicsIfNeeded()
{
    if (bLegacyDynamicsDisabled || !LegacyDynamics)
    {
        return;
    }

    LegacyDynamics->SetDriverInputs(0.0f, 0.0f);
    LegacyDynamics->SetComponentTickEnabled(false);
    bLegacyDynamicsDisabled = true;
}

bool UGTTChaosVehicleBridgeComponent::TryGetNativeHitchTransform(FTransform& OutTransform) const
{
    if (!bRigContractValid || !NativeSkeletalBody || !ResolvedRigContract.bRequiresHitchSocket || ResolvedRigContract.HitchSocket.IsNone())
    {
        return false;
    }

    if (!NativeSkeletalBody->DoesSocketExist(ResolvedRigContract.HitchSocket))
    {
        return false;
    }

    OutTransform = NativeSkeletalBody->GetSocketTransform(ResolvedRigContract.HitchSocket, RTS_World);
    return true;
}

FString UGTTChaosVehicleBridgeComponent::GetBridgeStatusSummary() const
{
    const TCHAR* StateText = TEXT("WAITING");
    switch (BridgeState)
    {
        case EGTTChaosBridgeState::NativeReady: StateText = TEXT("READY"); break;
        case EGTTChaosBridgeState::NativeDriving: StateText = TEXT("DRIVING"); break;
        case EGTTChaosBridgeState::NativeBlocked: StateText = TEXT("BLOCKED"); break;
        default: break;
    }

    return FString::Printf(TEXT("CHAOS %s | %s | RIG %s | PWR %.0f%% | GRIP %.0f%%"),
        StateText,
        ResolvedSpec.VehicleId.IsNone() ? TEXT("NO SPEC") : *ResolvedSpec.VehicleId.ToString(),
        bRigContractValid ? TEXT("VALID") : TEXT("WAIT"),
        EffectivePowerScale * 100.0f,
        EffectiveGripScale * 100.0f);
}
