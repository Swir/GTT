#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interaction/GTTInteractable.h"
#include "GTTVehicleBase.generated.h"

class UCameraComponent;
class UPrimitiveComponent;
class USpringArmComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGTTVehicleDriverEvent, APawn*, Driver);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGTTVehicleEvent);

UCLASS(Blueprintable)
class GTT_API AGTTVehicleBase : public APawn, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTVehicleBase();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void ExitVehicle();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void ApplyVehicleDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void RepairVehicle(float RepairAmount);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Fuel")
    void RefuelVehicle(float Liters);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Ownership")
    void MarkOwnedByPlayer();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Save")
    void RestorePersistentState(const FTransform& InTransform, float ConditionPercent, float FuelLiters, bool bOwned,
        int32 InEngineUpgradeLevel = 0, int32 InTireUpgradeLevel = 0, float InTireIntegrity = 1.0f);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Garage")
    bool RecallToTransform(const FTransform& Destination);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Tuning")
    bool InstallEngineUpgrade();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Tuning")
    bool InstallTireUpgrade();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Tuning")
    void RepairTires();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Tuning")
    void ApplyTireDamage(float Amount);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    float GetConditionPercent() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    float GetSpeedKmh() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Fuel")
    float GetFuelPercent() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Fuel")
    float GetFuelLiters() const { return CurrentFuelLiters; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Fuel")
    float GetFuelCapacity() const { return FuelCapacityLiters; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage")
    float GetEngineTemperatureC() const { return EngineTemperatureC; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage")
    int32 GetDetachedPartCount() const { return DetachedPartCount; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage")
    FString GetFaultStatusText() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Tuning")
    int32 GetEngineUpgradeLevel() const { return EngineUpgradeLevel; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Tuning")
    int32 GetTireUpgradeLevel() const { return TireUpgradeLevel; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Tuning")
    float GetTireIntegrity() const { return TireIntegrity; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Tuning")
    FString GetTuningSummary() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Ownership")
    bool IsOwnedByPlayer() const { return bOwnedByPlayer; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Save")
    FName GetPersistentVehicleId() const { return PersistentVehicleId; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    bool IsOccupied() const { return bOccupied; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    bool IsEngineRunning() const { return bEngineRunning; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    bool WasReportedStolen() const { return bTheftReported; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    APawn* GetDriverPawn() const { return PreviousPawn.Get(); }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    FText GetVehicleDisplayName() const { return VehicleDisplayName; }

    UPROPERTY(BlueprintAssignable, Category="GTT|Vehicle")
    FGTTVehicleDriverEvent OnDriverEntered;

    UPROPERTY(BlueprintAssignable, Category="GTT|Vehicle")
    FGTTVehicleDriverEvent OnDriverExited;

    UPROPERTY(BlueprintAssignable, Category="GTT|Vehicle")
    FGTTVehicleEvent OnVehicleStolen;

    UPROPERTY(BlueprintAssignable, Category="GTT|Vehicle|Fuel")
    FGTTVehicleEvent OnOutOfFuel;

protected:
    void HandleThrottle(float Value);
    void HandleSteering(float Value);
    void CycleRadio();
    void RegisterBreakablePart(UStaticMeshComponent* Part, float DetachAtConditionPercent, FName PartName);

    UFUNCTION()
    void HandleVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle", meta=(DisplayName="Throttle Input"))
    void OnThrottleInput(float Value);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle", meta=(DisplayName="Steering Input"))
    void OnSteeringInput(float Value);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle")
    void OnEngineStateChanged(bool bRunning);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle")
    void OnVehicleBrokenDown();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle")
    TObjectPtr<UStaticMeshComponent> VehicleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<UCameraComponent> VehicleCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage")
    TObjectPtr<UStaticMeshComponent> DamageSmokePuffA;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage")
    TObjectPtr<UStaticMeshComponent> DamageSmokePuffB;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage")
    TObjectPtr<UStaticMeshComponent> DamageSmokePuffC;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle")
    FText VehicleDisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Save")
    FName PersistentVehicleId = TEXT("Vehicle");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle", meta=(ClampMin="1.0"))
    float MaxCondition = 100.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="GTT|Vehicle")
    float Condition = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Driving", meta=(ClampMin="0.0"))
    float DriveAcceleration = 950.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Driving", meta=(ClampMin="0.0"))
    float SteeringAcceleration = 75.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Fuel", meta=(ClampMin="1.0"))
    float FuelCapacityLiters = 45.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Fuel", meta=(ClampMin="0.0"))
    float StartingFuelLiters = 22.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="GTT|Vehicle|Fuel")
    float CurrentFuelLiters = 0.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Fuel", meta=(ClampMin="0.0"))
    float IdleFuelBurnPerSecond = 0.025f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Fuel", meta=(ClampMin="0.0"))
    float FullThrottleFuelBurnPerSecond = 0.11f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="0.0"))
    float NormalEngineTemperatureC = 72.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="70.0"))
    float OverheatStartTemperatureC = 105.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="90.0"))
    float CriticalEngineTemperatureC = 120.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="0.0"))
    float LowConditionFaultChancePerSecond = 0.16f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="0.1"))
    float FaultRestartDelaySeconds = 2.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Crime")
    bool bIllegalToTake = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Crime", meta=(ClampMin="0.0"))
    float TheftHeat = 28.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="0.0"))
    float MinDamagingImpulse = 120000.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle|Damage", meta=(ClampMin="1.0"))
    float ImpulsePerDamagePoint = 45000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle")
    FVector ExitOffset = FVector(0.0f, 180.0f, 70.0f);

private:
    struct FBreakablePartRuntime
    {
        TWeakObjectPtr<UStaticMeshComponent> Component;
        FTransform OriginalRelativeTransform;
        float DetachThreshold = 0.5f;
        FName PartName = NAME_None;
        bool bDetached = false;
    };

    void SetEngineRunning(bool bNewRunning);
    void UpdateBreakableParts();
    void RestoreBreakableParts();
    void UpdateDamageSmoke(float DeltaSeconds);
    void TriggerMechanicalStall(const TCHAR* Reason);

    TWeakObjectPtr<APawn> PreviousPawn;
    TArray<FBreakablePartRuntime> BreakableParts;
    bool bOccupied = false;
    bool bEngineRunning = false;
    bool bTheftReported = false;
    bool bOwnedByPlayer = false;
    float LastThrottleInput = 0.0f;
    float EngineTemperatureC = 72.0f;
    float FaultRestartTimeRemaining = 0.0f;
    float DamageFxClock = 0.0f;
    int32 DetachedPartCount = 0;
    FString ActiveFaultStatus;

    int32 EngineUpgradeLevel = 0;
    int32 TireUpgradeLevel = 0;
    float TireIntegrity = 1.0f;
};
