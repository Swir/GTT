#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTOldCarPawn.h"
#include "GTTTrafficCarPawn.generated.h"

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
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="100.0"))
    float RoutePointRadius = 280.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="0.0"))
    float TrafficDriveForce = 620.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="0.0"))
    float TrafficSteeringTorque = 44.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="100.0"))
    float TargetCruiseSpeedCm = 720.0f;

private:
    TArray<FVector> RoutePoints;
    int32 CurrentRoutePoint = 0;
};
