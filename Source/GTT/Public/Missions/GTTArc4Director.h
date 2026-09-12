#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTArc4Director.generated.h"

class APawn;

UENUM(BlueprintType)
enum class EGTTArc4Stage : uint8
{
    Locked = 0,
    ProveGround = 1,
    PrepareContraband = 2,
    FenceRun = 3,
    NorthPass = 4,
    EscapePolice = 5,
    RidgeExchange = 6,
    FinalFarm = 7,
    Completed = 8
};

UCLASS()
class GTT_API AGTTArc4Director : public AActor
{
    GENERATED_BODY()
public:
    AGTTArc4Director();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc4") bool TryFarmContact(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc4") bool TryNorthPass(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc4") bool TryRidgeExchange(APawn* PlayerPawn);
    UFUNCTION(BlueprintPure, Category="GTT|Story|Arc4") EGTTArc4Stage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|Story|Arc4") FString GetObjectiveText() const;

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc4|Rewards") int32 NorthPassReward = 1250;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc4|Rewards") int32 Arc4CompletionReward = 1500;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc4|Crime") float NorthPassHeat = 92.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Story|Arc4|Save") FString SaveSlotName = TEXT("GTT_MainStory_Arc4_01");

private:
    bool IsArc3Complete() const;
    int32 GetFactionVictories() const;
    bool HasAuthorityAttention(APawn* PlayerPawn) const;
    void EvaluateProgress(APawn* PlayerPawn);
    void SetStage(EGTTArc4Stage NewStage, APawn* PlayerPawn, const FString& Message);
    void SaveProgress();
    void LoadProgress();
    void Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason);
    void PushMessage(APawn* PlayerPawn, const FString& Message, float Duration = 6.0f) const;

    EGTTArc4Stage Stage = EGTTArc4Stage::Locked;
    int32 StartingFactionVictories = 0;
    bool bContrabandPrepared = false;
    bool bEscapeMessageShown = false;
};
