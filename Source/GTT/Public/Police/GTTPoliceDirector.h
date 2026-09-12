#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTPoliceDirector.generated.h"

class AGTTPolicePursuitVehicle;

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float MinFallbackSpawnDistance = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float MaxFallbackSpawnDistance = 2200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.25"))
    float EvaluationInterval = 1.25f;

private:
    void SpawnPoliceUnit();
    void SpawnPursuitVehicle(int32 WantedLevel);
    int32 GetPlayerWantedLevel() const;
    void RemoveInvalidUnits();
    void DespawnExcessUnits(int32 DesiredUnits, int32 DesiredVehicles);
    FTransform SelectSpawnTransform(float DistanceScale = 1.0f) const;

    UPROPERTY()
    TArray<TObjectPtr<APawn>> ActivePoliceUnits;

    UPROPERTY()
    TArray<TObjectPtr<AGTTPolicePursuitVehicle>> ActivePursuitVehicles;

    FTimerHandle EvaluationTimer;
    int32 LastResponseLevel = INDEX_NONE;
};
