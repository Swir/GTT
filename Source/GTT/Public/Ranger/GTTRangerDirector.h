#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRangerDirector.generated.h"

class AGTTRangerPawn;

UCLASS()
class GTT_API AGTTRangerDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTRangerDirector();
    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger")
    TSubclassOf<AGTTRangerPawn> RangerClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.25"))
    float ResponseInterval = 1.0f;

private:
    void UpdateResponse();
    void CleanupInvalidRangers();

    FTimerHandle ResponseTimer;
    TArray<TWeakObjectPtr<AGTTRangerPawn>> ActiveRangers;
};
