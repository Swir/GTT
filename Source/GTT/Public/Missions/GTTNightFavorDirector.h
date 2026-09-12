#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTNightFavorDirector.generated.h"

UENUM(BlueprintType)
enum class EGTTNightFavorStage : uint8
{
    Idle,
    CollectParts,
    ReachNeighbor,
    ReturnToTavern,
    Completed
};

UCLASS()
class GTT_API AGTTNightFavorDirector : public AActor
{
    GENERATED_BODY()
public:
    AGTTNightFavorDirector();

    UFUNCTION(BlueprintCallable, Category="GTT|SideMission") bool TryStart(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|SideMission") bool TryCollectParts(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|SideMission") bool TryHelpNeighbor(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|SideMission") bool TryFinish(APawn* PlayerPawn);
    UFUNCTION(BlueprintPure, Category="GTT|SideMission") bool IsActive() const;
    UFUNCTION(BlueprintPure, Category="GTT|SideMission") EGTTNightFavorStage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|SideMission") FString GetObjectiveText() const;

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|SideMission") int32 CompletionReward = 450;

private:
    bool NightlifeWindowOpen() const;
    void PushMessage(APawn* Pawn, const FString& Message, float Duration = 5.0f) const;
    EGTTNightFavorStage Stage = EGTTNightFavorStage::Idle;
};
