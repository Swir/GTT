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
    Arc1Completed = 8,
    WardenBriefing = 9,
    ForestCache = 10,
    EscapeRanger = 11,
    HillFarmEvidence = 12,
    Arc2FinalFarm = 13,
    Completed = 14
};

UCLASS()
class GTT_API AGTTMainStoryDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTMainStoryDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryFarmContact(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryNorthWoodPickup(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryShopDelivery(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryTavernMeet(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryEastRoadPickup(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryWorkshopDelivery(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryWardenBriefing(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryForestCache(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story") bool TryHillFarmEvidence(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|Story") EGTTMainStoryStage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|Story") bool IsActive() const { return Stage != EGTTMainStoryStage::Locked && Stage != EGTTMainStoryStage::Completed; }
    UFUNCTION(BlueprintPure, Category="GTT|Story") FString GetObjectiveText() const;

    UFUNCTION(BlueprintCallable, Category="GTT|Story|Save") void RestoreStoryProgress(int32 SavedStage);
    UFUNCTION(BlueprintPure, Category="GTT|Story|Save") int32 GetStoryProgress() const { return static_cast<int32>(Stage); }

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 LedgerReward = 250;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 BackroadReward = 600;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 Arc1CompletionReward = 350;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 RangerEvidenceReward = 500;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Rewards") int32 Arc2CompletionReward = 700;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Crime") float BackroadPickupHeat = 58.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Crime") float ForestCacheRangerSeverity = 72.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Save") FString StorySaveSlotName = TEXT("GTT_MainStory_01");

private:
    bool IsBorrowedTractorComplete() const;
    bool HasAuthorityAttention(APawn* PlayerPawn) const;
    bool IsNightWindow() const;
    bool HasUsableOwnedTractor(APawn* PlayerPawn) const;
    FString BuildRoadHint(APawn* PlayerPawn, const FVector& Destination) const;
    void SetStage(EGTTMainStoryStage NewStage, APawn* PlayerPawn, const FString& Message);
    void SaveStoryProgress();
    void LoadStoryProgress();
    void Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason);
    void PushMessage(APawn* PlayerPawn, const FString& Message, float Duration = 6.0f) const;

    EGTTMainStoryStage Stage = EGTTMainStoryStage::Locked;
    bool bEscapeMessageShown = false;
};
