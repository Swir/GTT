#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTMainStoryDirector.generated.h"

UENUM(BlueprintType)
enum class EGTTMainStoryStage : uint8
{
    Locked = 0,
    NorthWoodPickup = 1,
    ShopDelivery = 2,
    TavernMeet = 3,
    EastRoadPickup = 4,
    EscapePolice = 5,
    WorkshopDelivery = 6,
    FinalFarmMeet = 7,
    Completed = 8
};

UCLASS()
class GTT_API AGTTMainStoryDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTMainStoryDirector();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryFarmContact(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryNorthWoodPickup(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryShopDelivery(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryTavernMeet(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryEastRoadPickup(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryWorkshopDelivery(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|Story") EGTTMainStoryStage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|Story") bool IsActive() const { return Stage != EGTTMainStoryStage::Locked && Stage != EGTTMainStoryStage::Completed; }
    UFUNCTION(BlueprintPure, Category="GTT|Story") FString GetObjectiveText() const;

    UFUNCTION(BlueprintCallable, Category="GTT|Story|Save") void RestoreStoryProgress(int32 SavedStage);
    UFUNCTION(BlueprintPure, Category="GTT|Story|Save") int32 GetStoryProgress() const { return static_cast<int32>(Stage); }

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 LedgerReward = 250;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 BackroadReward = 600;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 ArcCompletionReward = 350;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Crime") float BackroadPickupHeat = 58.0f;

private:
    bool IsBorrowedTractorComplete() const;
    bool HasAuthorityAttention(APawn* PlayerPawn) const;
    bool IsNightWindow() const;
    void SetStage(EGTTMainStoryStage NewStage, APawn* PlayerPawn, const FString& Message);
    void Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason);
    void PushMessage(APawn* PlayerPawn, const FString& Message, float Duration = 6.0f) const;

    EGTTMainStoryStage Stage = EGTTMainStoryStage::Locked;
    bool bEscapeMessageShown = false;
};
