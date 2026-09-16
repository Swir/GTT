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
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|Mission")
    UGTTMissionComponent* GetMissionComponent() const { return MissionComponent; }

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    void NotifyVehicleStolen(AGTTVehicleBase* Vehicle, APawn* Offender);

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    bool TryCompleteBorrowedTractor(AGTTVehicleBase* Vehicle);

    UFUNCTION(BlueprintCallable, Category="GTT|Save")
    virtual bool SaveProgress();

    UFUNCTION(BlueprintCallable, Category="GTT|Save")
    virtual bool LoadProgress();

    UFUNCTION(BlueprintCallable, Category="GTT|Police")
    bool TryArrestPlayer(APawn* PursuedPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|Ranger")
    void ReportWildlifeCrime(APawn* Offender, float Severity);

    UFUNCTION(BlueprintCallable, Category="GTT|Ranger")
    bool TryRangerCitation(APawn* PursuedPawn);

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    int32 GetWildlifeAlertLevel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    float GetWildlifeHeatPercent() const { return WildlifeHeat / 100.0f; }

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool StartFarmJob(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool CompleteFarmJob(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    bool IsFarmJobActive() const { return bFarmJobActive; }

    UFUNCTION(BlueprintCallable, Category="GTT|Garage")
    bool TryRegisterVehicle(AGTTVehicleBase* Vehicle, APawn* PlayerPawn, int32 RegistrationCost);

    UFUNCTION(BlueprintPure, Category="GTT|Garage")
    int32 GetOwnedVehicleCount() const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage")
    int32 GetGarageCapacity() const { return GarageCapacity; }

    UFUNCTION(BlueprintPure, Category="GTT|World|Time")
    AGTTDayNightCycle* GetDayNightCycle() const { return DayNightCycle.Get(); }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Mission")
    TObjectPtr<UGTTMissionComponent> MissionComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Mission|Rewards", meta=(ClampMin="0"))
    int32 BorrowedTractorCashReward = 300;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0"))
    int32 FarmJobReward = 180;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Garage", meta=(ClampMin="1", ClampMax="12"))
    int32 GarageCapacity = 4;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.0"))
    float WildlifeHeatDecayPerSecond = 2.8f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.0"))
    float WildlifeQuietDelay = 18.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Save")
    FString SaveSlotName = TEXT("GTT_Prototype_01");

private:
    void PushPlayerMessage(APawn* Pawn, const FString& Message, float Duration = 4.0f) const;

    bool bFarmJobActive = false;
    float WildlifeHeat = 0.0f;
    float WildlifeQuietTimeRemaining = 0.0f;
    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
};
