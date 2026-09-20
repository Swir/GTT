#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTHeavyHaulDirector.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;
class AGTTVehicleBase;

UENUM(BlueprintType)
enum class EGTTHeavyHaulStage : uint8
{
    Idle,
    HitchTrailer,
    ReachWoodYard,
    LoadTimber,
    DeliverHillFarm,
    Completed
};

UCLASS()
class GTT_API AGTTHeavyHaulDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTHeavyHaulDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|HeavyHaul") bool TryStartContract(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|HeavyHaul") bool TryHitchTrailer(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|HeavyHaul") bool TryLoadTimber(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|HeavyHaul") bool TryDeliverTimber(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|HeavyHaul") bool TryRoadsideRepair(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul") EGTTHeavyHaulStage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul") FString GetObjectiveText() const;
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul") AGTTFarmTrailer* GetTrailer() const { return Trailer; }
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul") bool IsActive() const { return Stage != EGTTHeavyHaulStage::Idle && Stage != EGTTHeavyHaulStage::Completed; }
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul") int32 GetRoadsideRepairCount() const { return RoadsideRepairCount; }
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul|DrivingQuality") float GetSmoothHaulSeconds() const { return SmoothHaulSeconds; }
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul|DrivingQuality") float GetRoughHaulSeconds() const { return RoughHaulSeconds; }
    UFUNCTION(BlueprintPure, Category="GTT|HeavyHaul|DrivingQuality") bool IsSmoothHaulBonusArmed() const;

private:
    bool CanTakeContract(APawn* PlayerPawn) const;
    AGTTVehicleBase* FindEligibleTowVehicle(const FVector& Origin, float Radius) const;
    AGTTFieldmasterNativePawn* FindEligibleNativeTowVehicle(const FVector& Origin, float Radius) const;
    float GetContractTowConditionFactor() const;
    void UpdateDrivingQuality(float DeltaSeconds);
    void ResetDrivingQuality();
    void ResetContract(bool bResetTrailer);
    void PushMessage(APawn* Pawn, const FString& Message, float Duration = 5.0f) const;

    UPROPERTY()
    TObjectPtr<AGTTFarmTrailer> Trailer;

    UPROPERTY()
    TObjectPtr<AGTTVehicleBase> ContractTowVehicle;

    UPROPERTY()
    TObjectPtr<AGTTFieldmasterNativePawn> ContractNativeTowVehicle;

    EGTTHeavyHaulStage Stage = EGTTHeavyHaulStage::Idle;
    float TimeRemaining = 0.0f;
    int32 RoadsideRepairCount = 0;
    float SmoothHaulSeconds = 0.0f;
    float RoughHaulSeconds = 0.0f;
    bool bSmoothHaulBonusAnnounced = false;
    bool bRoughDrivingWarningIssued = false;

    FVector TrailerYardLocation = FVector(-3200.0f, -1500.0f, 95.0f);
    FVector WoodYardLoadLocation = FVector(7850.0f, 450.0f, 80.0f);
    FVector HillFarmDropLocation = FVector(5850.0f, 2550.0f, 80.0f);

    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul") float ContractTimeLimit = 330.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul") int32 BaseReward = 900;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul") int32 FastBonus = 250;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|Recovery") float RoadsideRepairTimePenalty = 22.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float SmoothHaulTargetSeconds = 60.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float RoughHaulAllowanceSeconds = 12.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") int32 SmoothHaulBonus = 180;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float SmoothMinSpeedKmh = 16.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float SmoothMaxSpeedKmh = 52.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float RoughSpeedKmh = 65.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float SmoothMaxHitchLoad = 0.32f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|HeavyHaul|DrivingQuality") float RoughHitchLoad = 0.58f;
};
