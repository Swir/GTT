#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTBrawlDirector.generated.h"

class AGTTCitizenPawn;

UCLASS()
class GTT_API AGTTBrawlDirector : public AActor
{
    GENERATED_BODY()
public:
    AGTTBrawlDirector();
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable, Category="GTT|Combat|Brawl") bool TryStartBrawl(APawn* PlayerPawn);
    UFUNCTION(BlueprintPure, Category="GTT|Combat|Brawl") bool IsBrawlActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="GTT|Combat|Brawl") FString GetObjectiveText() const;
private:
    void FinishBrawl(APawn* PlayerPawn);
    void ResetBrawl(bool bDestroyRemaining);
    UPROPERTY() TArray<TObjectPtr<AGTTCitizenPawn>> Brawlers;
    bool bActive = false;
    float TimeRemaining = 0.0f;
    int32 Reward = 260;
};
