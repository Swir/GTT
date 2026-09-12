#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTMissionSafeZone.generated.h"

class AGTTVehicleBase;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTMissionSafeZone : public AActor
{
    GENERATED_BODY()

public:
    AGTTMissionSafeZone();

    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Mission")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Mission")
    TObjectPtr<UStaticMeshComponent> GroundMarker;

private:
    TWeakObjectPtr<AGTTVehicleBase> TrackedVehicle;
    bool bMissionFinished = false;
};
