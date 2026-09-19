#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTTRoadsideResponderVehicle.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Lightweight physical county-road-service responder used by the civilian
 * incident handoff layer. It is an original runtime-built vehicle and owns no
 * player economy, wanted, ranger or civilian-dispatch authority.
 */
UCLASS()
class GTT_API AGTTRoadsideResponderVehicle : public AGTTVehicleBase
{
    GENERATED_BODY()

public:
    AGTTRoadsideResponderVehicle();
    virtual void Tick(float DeltaSeconds) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    void InitializeIncidentResponse(FName InIncidentId, const FVector& InSceneLocation, bool bStartAtScene);
    void BeginSceneClearance();

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    bool IsParkedAtScene() const { return bParkedAtScene; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    bool IsSafetyCorridorDeployed() const { return bSafetyCorridorDeployed; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    bool IsSceneClearing() const { return bSceneClearing; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    FName GetAssignedIncidentId() const { return AssignedIncidentId; }

private:
    void UpdateBeacon(float DeltaSeconds);
    void DriveTowardScene(float DeltaSeconds);
    void SetSafetyCorridorDeployed(bool bDeployed);

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder")
    TObjectPtr<UStaticMeshComponent> BeaconLeft;

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder")
    TObjectPtr<UStaticMeshComponent> BeaconRight;

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder")
    TObjectPtr<UTextRenderComponent> ServiceLabel;

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder|Safety")
    TObjectPtr<UStaticMeshComponent> SafetyConeFrontLeft;

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder|Safety")
    TObjectPtr<UStaticMeshComponent> SafetyConeFrontRight;

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder|Safety")
    TObjectPtr<UStaticMeshComponent> SafetyConeRearLeft;

    UPROPERTY(VisibleAnywhere, Category="GTT|Traffic|Responder|Safety")
    TObjectPtr<UStaticMeshComponent> SafetyConeRearRight;

    FName AssignedIncidentId = NAME_None;
    FVector SceneLocation = FVector::ZeroVector;
    float BeaconClock = 0.0f;
    bool bParkedAtScene = false;
    bool bSafetyCorridorDeployed = false;
    bool bSceneClearing = false;

    static constexpr float ArrivalRadiusCm = 430.0f;
    static constexpr float TargetCruiseSpeedCm = 820.0f;
    static constexpr float ResponseDriveForce = 1180.0f;
    static constexpr float ResponseSteeringTorque = 92.0f;
};
