#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GTTGameMode.generated.h"
class APawn;
class AGTTVehicleBase;
class AGTTDayNightCycle;
class UGTTMissionComponent;
UCLASS()
class GTT_API AGTTGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AGTTGameMode();
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintPure, Category="GTT|Mission") UGTTMissionComponent* GetMissionComponent() const { return MissionComponent; }
    UFUNCTION(BlueprintCallable, Category="GTT|Mission") void NotifyVehicleStolen(AGTTVehicleBase* Vehicle, APawn* Offender);
    UFUNCTION(BlueprintCallable, Category="GTT|Mission") bool TryCompleteBorrowedTractor(AGTTVehicleBase* Vehicle);
    UFUNCTION(BlueprintCallable, Category="GTT|Save") bool SaveProgress();
    UFUNCTION(BlueprintCallable, Category="GTT|Save") bool LoadProgress();
    UFUNCTION(BlueprintCallable, Category="GTT|Police") bool TryArrestPlayer(APawn* PursuedPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob") bool StartFarmJob(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob") bool CompleteFarmJob(APawn* PlayerPawn);
    UFUNCTION(BlueprintPure, Category="GTT|FarmJob") bool IsFarmJobActive() const { return bFarmJobActive; }
    UFUNCTION(BlueprintPure, Category="GTT|World|Time") AGTTDayNightCycle* GetDayNightCycle() const { return DayNightCycle.Get(); }
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Mission") TObjectPtr<UGTTMissionComponent> MissionComponent;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Mission|Rewards", meta=(ClampMin="0")) int32 BorrowedTractorCashReward = 300;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0")) int32 FarmJobReward = 180;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Save") FString SaveSlotName = TEXT("GTT_Prototype_01");
private:
    void PushPlayerMessage(APawn* Pawn, const FString& Message, float Duration = 4.0f) const;
    bool bFarmJobActive = false;
    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
};
