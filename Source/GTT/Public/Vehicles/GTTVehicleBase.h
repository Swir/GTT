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
    void RestorePersistentState(const FTransform& InTransform, float ConditionPercent, float FuelLiters, bool bOwned);

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
    void SetEngineRunning(bool bNewRunning);

    TWeakObjectPtr<APawn> PreviousPawn;
    bool bOccupied = false;
    bool bEngineRunning = false;
    bool bTheftReported = false;
    bool bOwnedByPlayer = false;
    float LastThrottleInput = 0.0f;
};
