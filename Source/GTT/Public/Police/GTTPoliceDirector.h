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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.25"))
    float EvaluationInterval = 2.0f;

private:
    void SpawnPoliceUnit();
    int32 GetPlayerWantedLevel() const;
    void RemoveInvalidUnits();

    UPROPERTY()
    TArray<TObjectPtr<APawn>> ActivePoliceUnits;

    FTimerHandle EvaluationTimer;
    int32 LastResponseLevel = INDEX_NONE;
};
