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
    DeliverCargo
};

UCLASS()
class GTT_API AGTTFarmJobDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTFarmJobDirector();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryStartJob(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryPickupCargo(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    bool TryCompleteJob(APawn* PlayerPawn);

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

private:
    void FailJob(APawn* PlayerPawn, const FString& Reason);
    void PushMessage(APawn* Pawn, const FString& Message, float Duration = 4.0f) const;
    APawn* ResolvePlayerPawn() const;
    void ClearLoadedVehicleCargoState();
    float ResolveCargoVehicleConditionRatio() const;

    EGTTFarmJobStage Stage = EGTTFarmJobStage::Idle;
    float TimeRemaining = 0.0f;
    float CargoIntegrity = 1.0f;
    float FleetPayoutMultiplier = 1.0f;
    TWeakObjectPtr<AGTTFarmVanPawn> LoadedMulebox;
    TWeakObjectPtr<AGTTMuleboxNativePawn> LoadedNativeMulebox;
};
