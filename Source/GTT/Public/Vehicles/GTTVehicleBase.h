#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Interaction/GTTInteractable.h"
#include "GTTVehicleBase.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class GTT_API AGTTVehicleBase : public APawn, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTVehicleBase();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void ExitVehicle();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void ApplyVehicleDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void RepairVehicle(float RepairAmount);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    float GetConditionPercent() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    bool IsOccupied() const { return bOccupied; }

protected:
    void HandleThrottle(float Value);
    void HandleSteering(float Value);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle", meta=(DisplayName="Throttle Input"))
    void OnThrottleInput(float Value);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle", meta=(DisplayName="Steering Input"))
    void OnSteeringInput(float Value);

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Vehicle")
    void OnVehicleBrokenDown();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle")
    TObjectPtr<UStaticMeshComponent> VehicleMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<UCameraComponent> VehicleCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle", meta=(ClampMin="1.0"))
    float MaxCondition = 100.0f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="GTT|Vehicle")
    float Condition = 100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Vehicle")
    FVector ExitOffset = FVector(0.0f, 180.0f, 40.0f);

private:
    TWeakObjectPtr<APawn> PreviousPawn;
    bool bOccupied = false;
};
