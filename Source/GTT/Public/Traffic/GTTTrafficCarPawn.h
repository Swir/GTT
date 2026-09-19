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

    UFUNCTION(BlueprintCallable, Category="GTT|Traffic|Incident")
    void RegisterCollisionIncident(float ImpactSpeedKmh, FVector SourceLocation);

    UFUNCTION(BlueprintCallable, Category="GTT|Traffic|Incident")
    void ReactToNearbyIncident(FVector SourceLocation, float Severity);

    UFUNCTION(BlueprintCallable, Category="GTT|Traffic|Roadside")
    bool BeginRoadsideAssistance(AActor* Helper);

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Incident") bool IsIncidentDisabled() const { return bIncidentDisabled; }
    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Incident") float GetIncidentResponseRemaining() const { return IncidentStopRemaining; }
    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Roadside") bool IsRoadsideAssistanceActive() const { return bRoadsideAssistanceActive; }
    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Roadside") float GetRoadsideAssistanceRemaining() const { return RoadsideAssistanceRemaining; }
    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Roadside") bool WasRoadsideAssistanceCompletedForIncident() const { return bRoadsideAssistanceCompletedForIncident; }
    UFUNCTION(BlueprintPure, Category="GTT|Traffic|RoadStop") bool IsYieldingForRangerStop() const { return bYieldingForRangerStop; }
    UFUNCTION(BlueprintPure, Category="GTT|Traffic|RoadStop") bool IsHoldingForRangerStop() const { return bHoldingForRangerStop; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Traffic")
    TObjectPtr<UTextRenderComponent> HornText;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="100.0")) float RoutePointRadius = 280.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="0.0")) float TrafficDriveForce = 620.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="0.0")) float TrafficSteeringTorque = 44.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic", meta=(ClampMin="100.0")) float TargetCruiseSpeedCm = 720.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Avoidance", meta=(ClampMin="100.0")) float ObstacleProbeDistance = 760.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Avoidance", meta=(ClampMin="0.1")) float HornCooldownSeconds = 2.2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Avoidance", meta=(ClampMin="0.5")) float StuckRecoverySeconds = 3.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Incident", meta=(ClampMin="0.5")) float CollisionStopSeconds = 4.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Incident", meta=(ClampMin="0.5")) float NearbyIncidentStopSeconds = 2.2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Incident", meta=(ClampMin="1.0")) float IncidentLimpSeconds = 16.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Incident", meta=(ClampMin="0.0", ClampMax="1.0")) float DisableConditionThreshold = 0.18f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Incident", meta=(ClampMin="100.0")) float IncidentAwarenessRadius = 2200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Roadside", meta=(ClampMin="1.0")) float RoadsideAssistanceDurationSeconds = 6.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Roadside", meta=(ClampMin="100.0")) float RoadsideAssistanceMaxDistance = 500.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Roadside", meta=(ClampMin="0.05", ClampMax="1.0")) float RoadsideRepairFraction = 0.45f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Roadside", meta=(ClampMin="0")) int32 RoadsideBasePayout = 65;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Roadside", meta=(ClampMin="0")) int32 RoadsideSeverityBonus = 45;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Roadside", meta=(ClampMin="1.0")) float RoadsidePostAssistLimpSeconds = 12.0f;

private:
    void CancelRoadsideAssistance(const TCHAR* Reason);
    void CompleteRoadsideAssistance();

    TArray<FVector> RoutePoints;
    int32 CurrentRoutePoint = 0;
    float HornCooldownRemaining = 0.0f;
    float HornVisualRemaining = 0.0f;
    float StuckTime = 0.0f;
    float IncidentStopRemaining = 0.0f;
    float IncidentLimpRemaining = 0.0f;
    float IncidentSteerBias = 0.0f;
    float LastIncidentSeverity = 0.0f;
    float LastObservedConditionPercent = 1.0f;
    float RoadsideAssistanceRemaining = 0.0f;
    TWeakObjectPtr<AActor> RoadsideHelper;
    bool bIncidentDisabled = false;
    bool bRoadsideAssistanceActive = false;
    bool bRoadsideAssistanceCompletedForIncident = false;
    bool bYieldingForRangerStop = false;
    bool bHoldingForRangerStop = false;
};
