#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRangerPatrolVehicle.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Original lightweight game-warden roadside response vehicle.
 *
 * This is intentionally not a second player-drivable vehicle stack. It is a
 * physical scene/support actor driven by the shared road-stop authority, so the
 * same incident decides where the pull-over marker, ranger and patrol unit sit.
 */
UCLASS()
class GTT_API AGTTRangerPatrolVehicle : public AActor
{
    GENERATED_BODY()

public:
    AGTTRangerPatrolVehicle();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|Ranger|Patrol")
    bool IsRoadsideDeployed() const { return bRoadsideDeployed; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> Cabin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> FrontBumper;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> WheelFrontLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> WheelFrontRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> WheelRearLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> WheelRearRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> BeaconLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UStaticMeshComponent> BeaconRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Patrol")
    TObjectPtr<UTextRenderComponent> WardenLabel;

private:
    void SetRoadsideDeployed(bool bDeployed);
    void UpdateBeacons(float DeltaSeconds);

    bool bRoadsideDeployed = false;
    float BeaconClock = 0.0f;
};