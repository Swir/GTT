#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GTTGameMode.generated.h"

class APawn;
class AGTTVehicleBase;
class UGTTMissionComponent;

UCLASS()
class GTT_API AGTTGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGTTGameMode();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category="GTT|Mission")
    UGTTMissionComponent* GetMissionComponent() const { return MissionComponent; }

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    void NotifyVehicleStolen(AGTTVehicleBase* Vehicle, APawn* Offender);

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    bool TryCompleteBorrowedTractor(AGTTVehicleBase* Vehicle);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Mission")
    TObjectPtr<UGTTMissionComponent> MissionComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Mission|Rewards", meta=(ClampMin="0"))
    int32 BorrowedTractorCashReward = 300;
};
