#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTFarmJobDirector.generated.h"

class AGTTFarmVanPawn;
class AGTTMuleboxNativePawn;

UENUM(BlueprintType)
enum class EGTTFarmJobStage : uint8
{
    Idle,
    ReachPickup,
    DeliverCargo,
    DeliverFinalStop
};

UCLASS()
class GTT_API AGTTFarmJobDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTFarmJobDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryStartJob(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryPickupCargo(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryCompleteJob(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryCompleteFinalStop(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    EGTTFarmJobStage GetStage() const { return Stage; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    bool IsJobActive() const { return Stage != EGTTFarmJobStage::Idle; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    float GetTimeRemaining() const { return TimeRemaining; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    float GetCargoIntegrity() const { return CargoIntegrity; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    FString GetObjectiveText() const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="30.0"))
    float DeliveryTimeLimit = 165.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0.0"))
    float ExtendedRouteExtraTime = 90.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0"))
    int32 BaseReward = 220;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0"))
    int32 FastDeliveryBonus = 90;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FastDeliveryThreshold = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0.0"))
    float DamagedVehicleCargoLossPerSecond = 0.035f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0"))
    int32 MuleboxRoleBonus = 45;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0"))
    int32 TrustedChainBonus = 70;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|FarmJob", meta=(ClampMin="0"))
    int32 ReliableChainBonus = 120;

private:
    bool CompleteCargoContract(APawn* PlayerPawn, bool bExtendedRoute);
    bool IsDeliveryVehiclePresent(APawn* PlayerPawn) const;
    bool IsPoliceBlockingHandoff(APawn* PlayerPawn, const FString& LocationLabel);
    void FailJob(APawn* PlayerPawn, const FString& Reason);
    void PushMessage(APawn* Pawn, const FString& Message, float Duration = 4.0f) const;
    APawn* ResolvePlayerPawn() const;
    void ClearLoadedVehicleCargoState();
    float ResolveCargoVehicleConditionRatio() const;
    void SpawnLogisticsDispatchers();

    EGTTFarmJobStage Stage = EGTTFarmJobStage::Idle;
    float TimeRemaining = 0.0f;
    float CargoIntegrity = 1.0f;
    float FleetPayoutMultiplier = 1.0f;
    float MarketMultiplierAtStart = 1.0f;
    int32 RouteTierAtStart = 1;
    int32 CargoUnitsReserved = 0;
    FString CargoCommodityAtStart = TEXT("ANIMAL FEED");
    bool bPoliceIncidentDuringRun = false;
    TWeakObjectPtr<AGTTFarmVanPawn> LoadedMulebox;
    TWeakObjectPtr<AGTTMuleboxNativePawn> LoadedNativeMulebox;
};
