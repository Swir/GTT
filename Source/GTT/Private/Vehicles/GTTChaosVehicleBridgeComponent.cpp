#include "Vehicles/GTTChaosVehicleBridgeComponent.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Vehicles/GTTVehicleDynamicsComponent.h"
#include "GTT.h"

namespace
{
    constexpr float FleetWheelEvidenceInterval = 4.0f;
    constexpr float FleetTractionRiskThreshold = 0.34f;

    float ComputeFleetWheelRisk(const FGTTChaosWheelRuntimeSnapshot& Snapshot)
    {
        if (!Snapshot.bComplete) return 0.0f;
        const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(Snapshot.Contacts)) / 3.0f, 0.0f, 1.0f);
        const float SlipRisk = FMath::Clamp(static_cast<float>(Snapshot.SlippingWheels) / 3.0f, 0.0f, 1.0f);
        const float SkidRisk = FMath::Clamp(static_cast<float>(Snapshot.SkiddingWheels) / 2.0f, 0.0f, 1.0f);
        const float MagnitudeRisk = FMath::Clamp(Snapshot.MaxSlipMagnitude / 650.0f, 0.0f, 1.0f);
        const float AngleRisk = FMath::Clamp(Snapshot.MaxSlipAngle / 32.0f, 0.0f, 1.0f);

        float SuspensionSpread = 0.0f;
        if (Snapshot.SuspensionLength.Num() == 4)
        {
            float MinLength = 1.0f;
            float MaxLength = 0.0f;
            for (const float Length : Snapshot.SuspensionLength)
            {
                if (Length < 0.0f) continue;
                MinLength = FMath::Min(MinLength, Length);
                MaxLength = FMath::Max(MaxLength, Length);
            }
            SuspensionSpread = FMath::Clamp((MaxLength - MinLength) / 0.55f, 0.0f, 1.0f);
        }

        return FMath::Clamp(FMath::Max3(ContactRisk, SkidRisk, FMath::Max(SlipRisk, FMath::Max(MagnitudeRisk, FMath::Max(AngleRisk, SuspensionSpread * 0.72f)))), 0.0f, 1.0f);
    }
}

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
        UpdateFleetWheelRuntime(DeltaTime);
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
    if (!VehicleOwner) return;

    if (!UGTTVehicleChaosSpecLibrary::GetSpecForVehicleId(VehicleOwner->GetPersistentVehicleId(), ResolvedSpec))
    {
        UE_LOG(LogGTT, Warning, TEXT("Chaos bridge has no canonical spec for %s"), *VehicleOwner->GetPersistentVehicleId().ToString());
    }
}

void UGTTChaosVehicleBridgeComponent::ResolveRigContract()
{
    ResolvedRigContract = FGTTChaosRigContract();
    if (!VehicleOwner) return;

    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(VehicleOwner->GetPersistentVehicleId(), ResolvedRigContract))
    {
        UE_LOG(LogGTT, Warning, TEXT("Chaos bridge has no rig contract for %s"), *VehicleOwner->GetPersistentVehicleId().ToString());
    }
}

bool UGTTChaosVehicleBridgeComponent::ValidateNativeRig()
{
    bRigContractValid = false;
    RigValidationSummary = TEXT("No native skeletal rig bound");
    if (!NativeSkeletalBody) return false;
    if (ResolvedRigContract.VehicleId.IsNone())
    {
        RigValidationSummary = TEXT("No rig contract resolved");
        return false;
    }

    TArray<FName> MissingBones;
    for (const FName BoneName : UGTTChaosRigContractLibrary::GetRequiredBoneNames(ResolvedRigContract))
    {
        if (BoneName.IsNone() || NativeSkeletalBody->GetBoneIndex(BoneName) == INDEX_NONE) MissingBones.Add(BoneName);
    }

    TArray<FName> MissingSockets;
    for (const FName SocketName : UGTTChaosRigContractLibrary::GetRequiredSocketNames(ResolvedRigContract))
    {
        if (SocketName.IsNone() || !NativeSkeletalBody->DoesSocketExist(SocketName)) MissingSockets.Add(SocketName);
    }

    if (MissingBones.Num() > 0 || MissingSockets.Num() > 0)
    {
        TArray<FString> Problems;
        for (const FName BoneName : MissingBones) Problems.Add(FString::Printf(TEXT("bone:%s"), *BoneName.ToString()));
        for (const FName SocketName : MissingSockets) Problems.Add(FString::Printf(TEXT("socket:%s"), *SocketName.ToString()));
        RigValidationSummary = FString::Printf(TEXT("Missing %s"), *FString::Join(Problems, TEXT(", ")));
        return false;
    }

    bRigContractValid = true;
    RigValidationSummary = TEXT("Rig contract valid");
    return true;
}

bool UGTTChaosVehicleBridgeComponent::ValidateNativeWheelSetup()
{
    bNativeWheelSetupValid = false;
    NativeSetupValidationSummary = TEXT("No native wheel setup bound");
    if (!NativeMovement || ResolvedSpec.VehicleId.IsNone()) return false;

    bNativeWheelSetupValid = UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(NativeMovement, ResolvedSpec.VehicleId, NativeSetupValidationSummary);
    return bNativeWheelSetupValid;
}

bool UGTTChaosVehicleBridgeComponent::ValidateNativePowertrain()
{
    bNativePowertrainValid = false;
    PowertrainValidationSummary = TEXT("No native powertrain bound");
    if (!NativeMovement || ResolvedSpec.VehicleId.IsNone()) return false;

    bNativePowertrainValid = UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(NativeMovement, ResolvedSpec.VehicleId, PowertrainValidationSummary);
    return bNativePowertrainValid;
}

bool UGTTChaosVehicleBridgeComponent::ConfigureNativeMovementFromCanonicalSpec(FString& OutSummary)
{
    if (!VehicleOwner) VehicleOwner = Cast<AGTTVehicleBase>(GetOwner());
    if (!VehicleOwner)
    {
        OutSummary = TEXT("No GTT vehicle owner");
        return false;
    }

    if (ResolvedSpec.VehicleId.IsNone()) ResolveSpec();
    NativeMovement = VehicleOwner->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
    if (!NativeMovement || ResolvedSpec.VehicleId.IsNone())
    {
        OutSummary = TEXT("Native Chaos movement/spec unavailable");
        return false;
    }

    FString WheelSummary;
    FString PowertrainSummary;
    const bool bWheelsConfigured = UGTTChaosNativeSetupLibrary::ConfigureCanonicalWheelSetups(NativeMovement, ResolvedSpec.VehicleId, WheelSummary);
    const bool bPowertrainConfigured = UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain(NativeMovement, ResolvedSpec.VehicleId, PowertrainSummary);

    OutSummary = FString::Printf(TEXT("WHEELS: %s | POWERTRAIN: %s"), *WheelSummary, *PowertrainSummary);
    RefreshNativeBinding();
    return bWheelsConfigured && bPowertrainConfigured && bNativeMovementReady;
}

void UGTTChaosVehicleBridgeComponent::RefreshNativeBinding()
{
    if (!VehicleOwner) VehicleOwner = Cast<AGTTVehicleBase>(GetOwner());
    if (VehicleOwner && ResolvedSpec.VehicleId.IsNone()) ResolveSpec();
    if (VehicleOwner && ResolvedRigContract.VehicleId.IsNone()) ResolveRigContract();

    NativeMovement = VehicleOwner ? VehicleOwner->FindComponentByClass<UChaosWheeledVehicleMovementComponent>() : nullptr;
    LegacyDynamics = VehicleOwner ? VehicleOwner->FindComponentByClass<UGTTVehicleDynamicsComponent>() : nullptr;
    NativeSkeletalBody = VehicleOwner ? VehicleOwner->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

    const bool bHasSpec = ResolvedSpec.VehicleId != NAME_None;
    const bool bRigValid = ValidateNativeRig();
    const bool bWheelSetupValid = ValidateNativeWheelSetup();
    const bool bPowertrainValid = ValidateNativePowertrain();
    bNativeMovementReady = NativeMovement != nullptr && bHasSpec && bRigValid && bWheelSetupValid && bPowertrainValid;
    BridgeState = bNativeMovementReady ? EGTTChaosBridgeState::NativeReady : EGTTChaosBridgeState::WaitingForNativeRig;

    if (!bNativeMovementReady)
    {
        WheelRuntimeSnapshot = FGTTChaosWheelRuntimeSnapshot();
    }

    if (bNativeMovementReady)
    {
        DisableLegacyDynamicsIfNeeded();
        UE_LOG(LogGTT, Log, TEXT("Chaos bridge native setup accepted for %s (%s; %s; %s)"), *ResolvedSpec.VehicleId.ToString(), *RigValidationSummary, *NativeSetupValidationSummary, *PowertrainValidationSummary);
    }
    else if (NativeMovement || NativeSkeletalBody)
    {
        UE_LOG(LogGTT, Warning, TEXT("Chaos bridge native setup rejected for %s: movement=%s rig=%s wheels=%s powertrain=%s"),
            ResolvedSpec.VehicleId.IsNone() ? TEXT("NO SPEC") : *ResolvedSpec.VehicleId.ToString(), NativeMovement ? TEXT("YES") : TEXT("NO"), *RigValidationSummary, *NativeSetupValidationSummary, *PowertrainValidationSummary);
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

    BridgeState = VehicleOwner->IsOccupied() && VehicleOwner->IsEngineRunning() ? EGTTChaosBridgeState::NativeDriving : EGTTChaosBridgeState::NativeReady;
}

void UGTTChaosVehicleBridgeComponent::UpdateFleetWheelRuntime(float DeltaTime)
{
    WheelRuntimeSnapshot = FGTTChaosWheelRuntimeSnapshot();
    WheelRuntimeSnapshot.SuspensionLength.Init(-1.0f, 4);
    WheelRuntimeSnapshot.SpringForce.Init(0.0f, 4);
    WheelRuntimeSnapshot.DriveTorque.Init(0.0f, 4);
    WheelRuntimeSnapshot.BrakeTorque.Init(0.0f, 4);

    if (!NativeMovement || NativeMovement->GetNumWheels() != 4 || !VehicleOwner) return;

    for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
    {
        const FWheelStatus& WheelState = NativeMovement->GetWheelState(WheelIndex);
        if (!WheelState.bIsValid) continue;

        ++WheelRuntimeSnapshot.ValidWheels;
        WheelRuntimeSnapshot.Contacts += WheelState.bInContact ? 1 : 0;
        WheelRuntimeSnapshot.SlippingWheels += WheelState.bIsSlipping ? 1 : 0;
        WheelRuntimeSnapshot.SkiddingWheels += WheelState.bIsSkidding ? 1 : 0;
        WheelRuntimeSnapshot.MaxSlipMagnitude = FMath::Max(WheelRuntimeSnapshot.MaxSlipMagnitude, FMath::Abs(WheelState.SlipMagnitude));
        WheelRuntimeSnapshot.MaxSlipAngle = FMath::Max(WheelRuntimeSnapshot.MaxSlipAngle, FMath::Abs(WheelState.SlipAngle));
        WheelRuntimeSnapshot.SuspensionLength[WheelIndex] = WheelState.NormalizedSuspensionLength;
        WheelRuntimeSnapshot.SpringForce[WheelIndex] = WheelState.SpringForce;
        WheelRuntimeSnapshot.DriveTorque[WheelIndex] = WheelState.DriveTorque;
        WheelRuntimeSnapshot.BrakeTorque[WheelIndex] = WheelState.BrakeTorque;
    }

    WheelRuntimeSnapshot.bComplete = WheelRuntimeSnapshot.ValidWheels == 4;
    if (!WheelRuntimeSnapshot.bComplete) return;

    const float RawRuntimeRisk = ComputeFleetWheelRisk(WheelRuntimeSnapshot);
    const float TireIntegrityPenalty = 1.0f - FMath::Clamp(VehicleOwner->GetTireIntegrity(), 0.0f, 1.0f);
    const float TuneAssist = FMath::Clamp(static_cast<float>(VehicleOwner->GetTireUpgradeLevel()) * 0.035f, 0.0f, 0.105f);
    WheelRuntimeSnapshot.RuntimeRisk = FMath::Clamp(RawRuntimeRisk + TireIntegrityPenalty * 0.22f - TuneAssist, 0.0f, 1.0f);

    if (WheelRuntimeSnapshot.RuntimeRisk >= FleetTractionRiskThreshold)
    {
        WheelRuntimeSnapshot.ThrottleLimit = FMath::Clamp(0.96f - WheelRuntimeSnapshot.RuntimeRisk * 0.62f + TuneAssist, 0.24f, 0.90f);
        WheelRuntimeSnapshot.BrakeAssist = WheelRuntimeSnapshot.RuntimeRisk >= 0.62f
            ? FMath::Clamp(0.03f + WheelRuntimeSnapshot.RuntimeRisk * 0.18f, 0.0f, 0.22f)
            : 0.0f;
        WheelRuntimeSnapshot.SteeringLimit = WheelRuntimeSnapshot.RuntimeRisk >= 0.78f
            ? FMath::Clamp(1.20f - WheelRuntimeSnapshot.RuntimeRisk * 0.55f, 0.55f, 1.0f)
            : 1.0f;
    }

    WheelEvidenceCountdown -= DeltaTime;
    if (WheelEvidenceCountdown <= 0.0f)
    {
        WheelEvidenceCountdown = FleetWheelEvidenceInterval;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_FLEET_WHEEL_STATE_EVIDENCE vehicle=%s contacts=%d/4 slipping=%d skidding=%d slip_mag=%.2f slip_angle=%.2f suspension=[%.2f,%.2f,%.2f,%.2f] spring=[%.1f,%.1f,%.1f,%.1f] runtime_risk=%.2f throttle_limit=%.2f brake_assist=%.2f steering_limit=%.2f tire_integrity=%.2f tire_level=%d"),
            *VehicleOwner->GetPersistentVehicleId().ToString(), WheelRuntimeSnapshot.Contacts, WheelRuntimeSnapshot.SlippingWheels, WheelRuntimeSnapshot.SkiddingWheels,
            WheelRuntimeSnapshot.MaxSlipMagnitude, WheelRuntimeSnapshot.MaxSlipAngle,
            WheelRuntimeSnapshot.SuspensionLength[0], WheelRuntimeSnapshot.SuspensionLength[1], WheelRuntimeSnapshot.SuspensionLength[2], WheelRuntimeSnapshot.SuspensionLength[3],
            WheelRuntimeSnapshot.SpringForce[0], WheelRuntimeSnapshot.SpringForce[1], WheelRuntimeSnapshot.SpringForce[2], WheelRuntimeSnapshot.SpringForce[3],
            WheelRuntimeSnapshot.RuntimeRisk, WheelRuntimeSnapshot.ThrottleLimit, WheelRuntimeSnapshot.BrakeAssist, WheelRuntimeSnapshot.SteeringLimit,
            VehicleOwner->GetTireIntegrity(), VehicleOwner->GetTireUpgradeLevel());
    }
}

void UGTTChaosVehicleBridgeComponent::RouteInputsToChaos()
{
    if (!NativeMovement || !VehicleOwner) return;

    const bool bCanDrive = VehicleOwner->IsOccupied() && VehicleOwner->IsEngineRunning() && VehicleOwner->GetFuelLiters() > KINDA_SMALL_NUMBER && VehicleOwner->GetConditionPercent() > 0.0f;
    if (!bCanDrive)
    {
        NativeMovement->SetThrottleInput(0.0f);
        NativeMovement->SetSteeringInput(0.0f);
        NativeMovement->SetBrakeInput(1.0f);
        return;
    }

    const float RuntimeThrottleLimit = WheelRuntimeSnapshot.bComplete ? WheelRuntimeSnapshot.ThrottleLimit : 1.0f;
    const float RuntimeSteeringLimit = WheelRuntimeSnapshot.bComplete ? WheelRuntimeSnapshot.SteeringLimit : 1.0f;
    const float RuntimeBrakeAssist = WheelRuntimeSnapshot.bComplete ? WheelRuntimeSnapshot.BrakeAssist : 0.0f;
    const float ScaledSteering = FMath::Clamp(RawSteeringInput * EffectiveGripScale * RuntimeSteeringLimit, -1.0f, 1.0f);
    const float ScaledThrottle = FMath::Clamp(FMath::Abs(RawThrottleInput) * EffectivePowerScale * RuntimeThrottleLimit, 0.0f, 1.0f);

    if (RawThrottleInput < -0.05f)
    {
        NativeMovement->SetTargetGear(-1, true);
        NativeMovement->SetThrottleInput(ScaledThrottle);
        NativeMovement->SetBrakeInput(RuntimeBrakeAssist);
    }
    else if (RawThrottleInput > 0.05f)
    {
        NativeMovement->SetTargetGear(1, false);
        NativeMovement->SetThrottleInput(ScaledThrottle);
        NativeMovement->SetBrakeInput(RuntimeBrakeAssist);
    }
    else
    {
        NativeMovement->SetThrottleInput(0.0f);
        NativeMovement->SetBrakeInput(FMath::Max(0.18f, RuntimeBrakeAssist));
    }

    NativeMovement->SetSteeringInput(ScaledSteering);
}

void UGTTChaosVehicleBridgeComponent::DisableLegacyDynamicsIfNeeded()
{
    if (bLegacyDynamicsDisabled || !LegacyDynamics) return;
    LegacyDynamics->SetDriverInputs(0.0f, 0.0f);
    LegacyDynamics->SetComponentTickEnabled(false);
    bLegacyDynamicsDisabled = true;
}

bool UGTTChaosVehicleBridgeComponent::TryGetNativeHitchTransform(FTransform& OutTransform) const
{
    if (!bRigContractValid || !NativeSkeletalBody || !ResolvedRigContract.bRequiresHitchSocket || ResolvedRigContract.HitchSocket.IsNone()) return false;
    if (!NativeSkeletalBody->DoesSocketExist(ResolvedRigContract.HitchSocket)) return false;
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

    const FString WheelRuntime = WheelRuntimeSnapshot.bComplete
        ? FString::Printf(TEXT("RUNTIME %d/4 RISK %.0f%%"), WheelRuntimeSnapshot.Contacts, WheelRuntimeSnapshot.RuntimeRisk * 100.0f)
        : TEXT("RUNTIME WAIT");

    return FString::Printf(TEXT("CHAOS %s | %s | RIG %s | WHEELS %s | POWERTRAIN %s | %s | PWR %.0f%% | GRIP %.0f%%"),
        StateText,
        ResolvedSpec.VehicleId.IsNone() ? TEXT("NO SPEC") : *ResolvedSpec.VehicleId.ToString(),
        bRigContractValid ? TEXT("VALID") : TEXT("WAIT"),
        bNativeWheelSetupValid ? TEXT("VALID") : TEXT("WAIT"),
        bNativePowertrainValid ? TEXT("VALID") : TEXT("WAIT"),
        *WheelRuntime,
        EffectivePowerScale * 100.0f,
        EffectiveGripScale * 100.0f);
}
