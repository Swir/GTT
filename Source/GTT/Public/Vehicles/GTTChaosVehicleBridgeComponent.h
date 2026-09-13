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

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativeMovementReady() const { return bNativeMovementReady; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativeRigContractValid() const { return bRigContractValid; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    bool IsNativeWheelSetupValid() const { return bNativeWheelSetupValid; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    EGTTChaosBridgeState GetBridgeState() const { return BridgeState; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetBridgeStatusSummary() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetRigValidationSummary() const { return RigValidationSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FString GetNativeSetupValidationSummary() const { return NativeSetupValidationSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FGTTChaosVehicleSpec GetResolvedSpec() const { return ResolvedSpec; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    FGTTChaosRigContract GetResolvedRigContract() const { return ResolvedRigContract; }

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    bool TryGetNativeHitchTransform(FTransform& OutTransform) const;

private:
    void ResolveSpec();
    void ResolveRigContract();
    bool ValidateNativeRig();
    bool ValidateNativeWheelSetup();
    void UpdateRuntimeState();
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
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FString RigValidationSummary = TEXT("No native skeletal rig bound");
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") FString NativeSetupValidationSummary = TEXT("No native wheel setup bound");
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") float EffectivePowerScale = 1.0f;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Chaos") float EffectiveGripScale = 1.0f;

    float RawThrottleInput = 0.0f;
    float RawSteeringInput = 0.0f;
    float RebindCountdown = 0.0f;
    bool bLegacyDynamicsDisabled = false;
};
