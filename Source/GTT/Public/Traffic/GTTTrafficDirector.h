#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTTrafficDirector.generated.h"

class AGTTTrafficCarPawn;

UCLASS()
class GTT_API AGTTTrafficDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTTrafficDirector();
    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Traffic", meta=(ClampMin="1", ClampMax="12"))
    int32 TrafficCarCount = 6;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Traffic")
    TSubclassOf<AGTTTrafficCarPawn> TrafficCarClass;

private:
    void SpawnTrafficLoop();
};
