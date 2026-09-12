#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTArc3Director.generated.h"

class AGTTCitizenPawn;

UENUM(BlueprintType)
enum class EGTTArc3Stage : uint8
{
    Locked = 0,
    RedBarnApproach = 1,
    RedBarnFight = 2,
    RedBarnEvidence = 3,
    EscapePolice = 4,
    CountyDrop = 5,
    FinalFarm = 6,
    Completed = 7
};

UCLASS()
class GTT_API AGTTArc3Director : public AActor
{
    GENERATED_BODY()

public:
    AGTTArc3Director();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc3") bool TryFarmContact(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc3") bool TryRedBarn(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc3") bool TryCountyDrop(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|Story|Arc3") EGTTArc3Stage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|Story|Arc3") bool IsActive() const { return Stage != EGTTArc3Stage::Locked && Stage != EGTTArc3Stage::Completed; }
    UFUNCTION(BlueprintPure, Category="GTT|Story|Arc3") FString GetObjectiveText() const;
    UFUNCTION(BlueprintPure, Category="GTT|Story|Arc3") int32 GetHostilesRemaining() const;

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc3|Rewards") int32 EvidenceDeliveryReward = 900;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc3|Rewards") int32 Arc3CompletionReward = 1100;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc3|Crime") float EvidenceRaidHeat = 82.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc3|Combat") int32 HostileCount = 4;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc3|Save") FString SaveSlotName = TEXT("GTT_MainStory_Arc3_01");

private:
    bool AreEarlierArcsComplete() const;
    bool HasAuthorityAttention(APawn* PlayerPawn) const;
    bool HasUsableOwnedVan() const;
    void SpawnOrRefreshHostiles(APawn* PlayerPawn);
    void ClearHostiles();
    void SetStage(EGTTArc3Stage NewStage, APawn* PlayerPawn, const FString& Message);
    void SaveProgress();
    void LoadProgress();
    void Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason);
    void PushMessage(APawn* PlayerPawn, const FString& Message, float Duration = 6.0f) const;

    EGTTArc3Stage Stage = EGTTArc3Stage::Locked;
    TArray<TWeakObjectPtr<AGTTCitizenPawn>> Hostiles;
    float HostileRefreshTimeRemaining = 0.0f;
    bool bEscapeMessageShown = false;
};
