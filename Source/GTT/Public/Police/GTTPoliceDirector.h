#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTPoliceDirector.generated.h"

UCLASS(Blueprintable)
class GTT_API AGTTPoliceDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTPoliceDirector();

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="GTT|Police")
    void EvaluatePoliceResponse();

    UFUNCTION(BlueprintImplementableEvent, Category="GTT|Police")
    void OnResponseLevelChanged(int32 WantedLevel, int32 DesiredPoliceUnits);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TSubclassOf<APawn> PolicePawnClass;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="GTT|Police")
    TArray<TObjectPtr<AActor>> SpawnPoints;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1"))
    int32 UnitsPerWantedLevel = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1"))
    int32 MaxPoliceUnits = 10;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float MinFallbackSpawnDistance = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float MaxFallbackSpawnDistance = 2200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.25"))
    float EvaluationInterval = 1.25f;

private:
    void SpawnPoliceUnit();
    int32 GetPlayerWantedLevel() const;
    void RemoveInvalidUnits();
    void DespawnExcessUnits(int32 DesiredUnits);
    FTransform SelectSpawnTransform() const;

    UPROPERTY()
    TArray<TObjectPtr<APawn>> ActivePoliceUnits;

    FTimerHandle EvaluationTimer;
    int32 LastResponseLevel = INDEX_NONE;
};
