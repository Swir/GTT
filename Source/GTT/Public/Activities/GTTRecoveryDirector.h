#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRecoveryDirector.generated.h"

UENUM(BlueprintType)
enum class EGTTRecoveryStage : uint8
{
    Idle,
    ReachBreakdown,
    TowToWorkshop
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
    bool TryFinishRecovery(APawn* PlayerPawn, const FVector& WorkshopLocation);

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    bool IsRecoveryActive() const { return Stage != EGTTRecoveryStage::Idle; }

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    EGTTRecoveryStage GetRecoveryStage() const { return Stage; }

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    FString GetObjectiveText() const;

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    float GetTimeRemaining() const { return TimeRemaining; }

    UFUNCTION(BlueprintPure, Category="GTT|Recovery")
    class AGTTVehicleBase* GetRecoveryTarget() const { return RecoveryTarget.Get(); }

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    float RecoveryTimeLimit = 260.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    int32 RecoveryBaseReward = 420;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    int32 RecoveryFastBonus = 140;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Recovery")
    float WorkshopDropRadius = 850.0f;

private:
    bool CanStartLegalRecovery(APawn* PlayerPawn) const;
    void SpawnRecoveryTarget();
    void FailRecovery(APawn* PlayerPawn, const FString& Reason);
    void PushMessage(APawn* Pawn, const FString& Message, float Duration = 5.0f) const;

    EGTTRecoveryStage Stage = EGTTRecoveryStage::Idle;
    float TimeRemaining = 0.0f;
    FVector CurrentBreakdownLocation = FVector::ZeroVector;
    TWeakObjectPtr<class AGTTVehicleBase> RecoveryTarget;
};
