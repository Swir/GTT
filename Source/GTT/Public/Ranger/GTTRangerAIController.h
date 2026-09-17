#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GTTRangerAIController.generated.h"

class APawn;

UCLASS()
class GTT_API AGTTRangerAIController : public AAIController
{
    GENERATED_BODY()

public:
    AGTTRangerAIController();

    UFUNCTION(BlueprintPure, Category="GTT|Ranger|RoadStop")
    bool IsRoadStopActive() const { return bRoadStopActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger|RoadStop")
    bool IsRoadStopEvasionEscalated() const { return bRoadStopEvasionEscalated; }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger|RoadStop")
    float GetRoadStopSecondsRemaining() const { return RoadStopTimeRemaining; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.2"))
    float RepathInterval = 0.75f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="25.0"))
    float AcceptanceRadius = 120.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="50.0"))
    float CitationRadius = 155.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="100.0"))
    float BaseChaseSpeed = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.0"))
    float SpeedPerAlertLevel = 45.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="1", ClampMax="3"))
    int32 RoadStopAlertLevel = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="200.0"))
    float RoadStopOrderRadius = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="100.0"))
    float RoadStopSearchRadius = 275.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="0.0"))
    float RoadStopComplianceSpeedKmh = 2.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="1.0"))
    float RoadStopFleeSpeedKmh = 8.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="1.0"))
    float RoadStopGraceSeconds = 7.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="0.5"))
    float RoadStopComplianceHoldSeconds = 2.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger|RoadStop", meta=(ClampMin="0.0"))
    float RoadStopEvasionWantedHeat = 45.0f;

private:
    void UpdatePursuit();
    void ResetRoadStopState();
    bool IsVehicleTarget(APawn* Target) const;
    float GetTargetSpeedKmh(APawn* Target) const;
    void PushRangerMessage(APawn* Target, const FString& Message, float Duration = 4.5f) const;
    bool TryResolveRoadsideSearch(APawn* Target);
    void EscalateRoadStopEvasion(APawn* Target);

    FTimerHandle PursuitTimer;
    bool bRoadStopActive = false;
    bool bRoadStopEvasionEscalated = false;
    bool bSearchHoldMessageShown = false;
    float RoadStopTimeRemaining = 0.0f;
    float ComplianceHoldElapsed = 0.0f;
};
