#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTPoliceDirector.generated.h"

class AGTTPolicePursuitVehicle;
class AGTTRoadblock;

UCLASS(Blueprintable)
class GTT_API AGTTPoliceDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTPoliceDirector();

    UFUNCTION(BlueprintPure, Category="GTT|Police")
    int32 GetActiveFootUnitCount() const { return ActivePoliceUnits.Num(); }

    UFUNCTION(BlueprintPure, Category="GTT|Police")
    int32 GetActivePursuitVehicleCount() const { return ActivePursuitVehicles.Num(); }

    UFUNCTION(BlueprintPure, Category="GTT|Police")
    int32 GetActiveRoadblockCount() const { return ActiveRoadblocks.Num(); }

    UFUNCTION(BlueprintPure, Category="GTT|Police|Interception")
    bool IsRoadNodeInterceptionActive() const { return LastInterceptionNodeIndex != INDEX_NONE && GetPlayerWantedLevel() >= RoadblockEscalationWantedLevel; }

    UFUNCTION(BlueprintPure, Category="GTT|Police|Interception")
    FString GetLastInterceptionNodeLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Police|Interception")
    int32 GetRoadNodeCount() const { return RoadNodes.Num(); }

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="GTT|Police")
    void EvaluatePoliceResponse();

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Police")
    void OnResponseLevelChanged(int32 WantedLevel, int32 DesiredPoliceUnits);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TSubclassOf<APawn> PolicePawnClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TSubclassOf<AGTTPolicePursuitVehicle> PursuitVehicleClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TSubclassOf<AGTTRoadblock> RoadblockClass;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="GTT|Police")
    TArray<TObjectPtr<AActor>> SpawnPoints;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1"))
    int32 UnitsPerWantedLevel = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1"))
    int32 MaxPoliceUnits = 10;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1"))
    int32 MaxPursuitVehicles = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1", ClampMax="5"))
    int32 VehicleEscalationWantedLevel = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police|Roadblock", meta=(ClampMin="1", ClampMax="5"))
    int32 RoadblockEscalationWantedLevel = 4;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police|Roadblock", meta=(ClampMin="1", ClampMax="4"))
    int32 MaxRoadblocks = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float MinFallbackSpawnDistance = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float MaxFallbackSpawnDistance = 2200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.25"))
    float EvaluationInterval = 1.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police|Interception", meta=(ClampMin="0.5", ClampMax="8.0"))
    float InterceptPredictionSeconds = 3.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police|Interception", meta=(ClampMin="300.0"))
    float MinimumInterceptLeadDistance = 850.0f;

private:
    void SpawnPoliceUnit();
    void SpawnPursuitVehicle(int32 WantedLevel);
    void SpawnRoadblock(int32 WantedLevel);
    int32 GetPlayerWantedLevel() const;
    void RemoveInvalidUnits();
    void DespawnExcessUnits(int32 DesiredUnits, int32 DesiredVehicles, int32 DesiredRoadblocks);
    void BuildRuntimeRoadNetwork();
    int32 SelectInterceptionRoadNode(const APawn* PlayerPawn, bool bPreferFartherNode) const;
    FTransform MakeRoadNodeTransform(int32 NodeIndex, const APawn* PlayerPawn) const;
    FTransform SelectSpawnTransform(float DistanceScale = 1.0f) const;
    FTransform SelectPursuitInterceptTransform(int32 WantedLevel) const;
    FTransform SelectRoadblockTransform(int32 WantedLevel);

    UPROPERTY()
    TArray<TObjectPtr<APawn>> ActivePoliceUnits;

    UPROPERTY()
    TArray<TObjectPtr<AGTTPolicePursuitVehicle>> ActivePursuitVehicles;

    UPROPERTY()
    TArray<TObjectPtr<AGTTRoadblock>> ActiveRoadblocks;

    TArray<FVector> RoadNodes;
    TArray<FString> RoadNodeLabels;
    FTimerHandle EvaluationTimer;
    int32 LastResponseLevel = INDEX_NONE;
    int32 LastInterceptionNodeIndex = INDEX_NONE;
};
