#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTChaosVehicleSpec.h"
#include "GTTChaosVehicleBridgeComponent.generated.h"

class AGTTVehicleBase;
class UChaosWheeledVehicleMovementComponent;
class UGTTVehicleDynamicsComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EGTTChaosBridgeState : uint8
{
    WaitingForNativeRig,
    NativeReady,
    NativeDriving,
    NativeBlocked
};

USTRUCT(BlueprintType)
struct GTT_API FGTTChaosWheelRuntimeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bComplete = false;
    UPROPERTY(BlueprintReadOnly) int32 ValidWheels = 0;
    UPROPERTY(BlueprintReadOnly) int32 Contacts = 0;
    UPROPERTY(BlueprintReadOnly) int32 SlippingWheels = 0;
    UPROPERTY(BlueprintReadOnly) int32 SkiddingWheels = 0;
    UPROPERTY(BlueprintReadOnly) float MaxSlipMagnitude = 0.0f;
    UPROPERTY(BlueprintReadOnly) float MaxSlipAngle = 0.0f;
    UPROPERTY(BlueprintReadOnly) float RuntimeRisk = 0.0f;
    UPROPERTY(BlueprintReadOnly) float ThrottleLimit = 1.0f;
    UPROPERTY(BlueprintReadOnly) float BrakeAssist = 0.0f;
    UPROPERTY(BlueprintReadOnly) float SteeringLimit = 1.0f;
    UPROPERTY(BlueprintReadOnly) TArray<float> SuspensionLength;
    UPROPERTY(BlueprintReadOnly) TArray<float> SpringForce;
    UPROPERTY(BlueprintReadOnly) TArray<float> DriveTorque;
    UPROPERTY(BlueprintReadOnly) TArray<float> BrakeTorque;
};

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTChaosVehicleBridgeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTChaosVehicleBridgeComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    void CaptureThrottleInput(float Value);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    void CaptureSteeringInput(float Value);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    void RefreshNativeBinding();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    bool ConfigureNativeMovementFromCanonicalSpec(FString& OutSummary);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativeMovementReady() const { return bNativeMovementReady; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativeRigContractValid() const { return bRigContractValid; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativeWheelSetupValid() const { return bNativeWheelSetupValid; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativePowertrainValid() const { return bNativePowertrainValid; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    EGTTChaosBridgeState GetBridgeState() const { return BridgeState; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetBridgeStatusSummary() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetRigValidationSummary() const { return RigValidationSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetNativeSetupValidationSummary() const { return NativeSetupValidationSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetPowertrainValidationSummary() const { return PowertrainValidationSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FGTTChaosVehicleSpec GetResolvedSpec() const { return ResolvedSpec; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FGTTChaosRigContract GetResolvedRigContract() const { return ResolvedRigContract; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FGTTChaosWheelRuntimeSnapshot GetWheelRuntimeSnapshot() const { return WheelRuntimeSnapshot; }

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    bool TryGetNativeHitchTransform(FTransform& OutTransform) const;

private:
    void ResolveSpec();
    void ResolveRigContract();
    bool ValidateNativeRig();
    bool ValidateNativeWheelSetup();
    bool ValidateNativePowertrain();
    void UpdateRuntimeState();
    void UpdateFleetWheelRuntime(float DeltaTime);
    void RouteInputsToChaos();
    void DisableLegacyDynamicsIfNeeded();

    UPROPERTY(Transient) TObjectPtr<AGTTVehicleBase> VehicleOwner;
    UPROPERTY(Transient) TObjectPtr<UChaosWheeledVehicleMovementComponent> NativeMovement;
    UPROPERTY(Transient) TObjectPtr<UGTTVehicleDynamicsComponent> LegacyDynamics;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> NativeSkeletalBody;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FGTTChaosVehicleSpec ResolvedSpec;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FGTTChaosRigContract ResolvedRigContract;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") EGTTChaosBridgeState BridgeState = EGTTChaosBridgeState::WaitingForNativeRig;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") bool bNativeMovementReady = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") bool bRigContractValid = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") bool bNativeWheelSetupValid = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") bool bNativePowertrainValid = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FString RigValidationSummary = TEXT("No native skeletal rig bound");
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FString NativeSetupValidationSummary = TEXT("No native wheel setup bound");
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FString PowertrainValidationSummary = TEXT("No native powertrain bound");
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") float EffectivePowerScale = 1.0f;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") float EffectiveGripScale = 1.0f;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FGTTChaosWheelRuntimeSnapshot WheelRuntimeSnapshot;

    float RawThrottleInput = 0.0f;
    float RawSteeringInput = 0.0f;
    float RebindCountdown = 0.0f;
    float WheelEvidenceCountdown = 0.0f;
    bool bLegacyDynamicsDisabled = false;
};
