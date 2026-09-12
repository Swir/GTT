#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTOldCarPawn.h"
#include "GTTTrafficCarPawn.generated.h"

class UTextRenderComponent;

UCLASS()
class GTT_API AGTTTrafficCarPawn : public AGTTOldCarPawn
{
    GENERATED_BODY()

public:
    AGTTTrafficCarPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Traffic")
    void InitializeRoute(const TArray<FVector>& InRoute, int32 StartIndex = 0);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Traffic")
    TObjectPtr<UTextRenderComponent> HornText;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="100.0"))
    float RoutePointRadius = 280.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="0.0"))
    float TrafficDriveForce = 620.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="0.0"))
    float TrafficSteeringTorque = 44.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="100.0"))
    float TargetCruiseSpeedCm = 720.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Avoidance", meta=(ClampMin="100.0"))
    float ObstacleProbeDistance = 760.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Avoidance", meta=(ClampMin="0.1"))
    float HornCooldownSeconds = 2.2f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Avoidance", meta=(ClampMin="0.5"))
    float StuckRecoverySeconds = 3.5f;

private:
    TArray<FVector> RoutePoints;
    int32 CurrentRoutePoint = 0;
    float HornCooldownRemaining = 0.0f;
    float HornVisualRemaining = 0.0f;
    float StuckTime = 0.0f;
};
