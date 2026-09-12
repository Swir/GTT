#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRecoveryDirector.generated.h"

class AGTTVehicleBase;
class UPhysicsConstraintComponent;

UENUM(BlueprintType)
enum class EGTTRecoveryStage : uint8
{
    Idle,
    ReachBreakdown,
    HookVehicle,
    TowToWorkshop,
    Completed
};

UCLASS()
class GTT_API AGTTRecoveryDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTRecoveryDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Recovery")
    bool TryStartRecovery(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|Recovery")
    bool TryHookRecoveryVehicle(APawn* PlayerPawn);

    UFUNCTION(BlueprintCallable, Category="GTT|Recovery")
    bool TryFinishRecovery(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    EGTTRecoveryStage GetStage() const { return Stage; }

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    FString GetObjectiveText() const;

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    float GetTowCableLoad() const { return TowCableLoad; }

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    bool IsTowAttached() const { return bTowAttached; }

private:
    AGTTVehicleBase* FindNearbyPlayerVehicle(const FVector& Origin, float Radius) const;
    void SpawnRecoveryTarget();
    void DetachTow();
    bool CanTakeLegalJob(APawn* PlayerPawn) const;

    UPROPERTY()
    TObjectPtr<AGTTVehicleBase> DisabledVehicle;

    UPROPERTY()
    TObjectPtr<UPhysicsConstraintComponent> TowConstraint;

    UPROPERTY()
    TObjectPtr<AGTTVehicleBase> TowVehicle;

    EGTTRecoveryStage Stage = EGTTRecoveryStage::Idle;
    FVector BreakdownLocation = FVector(4300.0f, 1650.0f, 110.0f);
    FVector WorkshopDropLocation = FVector(-400.0f, 2650.0f, 90.0f);
    float ContractTimeRemaining = 0.0f;
    float TowCableLoad = 0.0f;
    bool bTowAttached = false;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    float ContractTimeLimit = 240.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    int32 BaseReward = 525;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    int32 FastBonus = 175;
};
