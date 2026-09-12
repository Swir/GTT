#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRuralWorkDirector.generated.h"

UENUM(BlueprintType)
enum class EGTTRuralWorkType : uint8
{
    None,
    TimberHaul,
    FieldMowing
};

UENUM(BlueprintType)
enum class EGTTRuralWorkStage : uint8
{
    Idle,
    ReachTimberPickup,
    DeliverTimber,
    MowingField
};

UCLASS()
class GTT_API AGTTRuralWorkDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTRuralWorkDirector();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|RuralWork") bool TryStartTimber(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|RuralWork") bool TryPickupTimber(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|RuralWork") bool TryDeliverTimber(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|RuralWork") bool TryStartMowing(APawn* PlayerPawn);
    UFUNCTION(BlueprintCallable, Category="GTT|RuralWork") bool TryMowingPass(APawn* PlayerPawn, int32 PassIndex);

    UFUNCTION(BlueprintPure, Category="GTT|RuralWork") bool IsWorkActive() const { return Stage != EGTTRuralWorkStage::Idle; }
    UFUNCTION(BlueprintPure, Category="GTT|RuralWork") EGTTRuralWorkType GetWorkType() const { return WorkType; }
    UFUNCTION(BlueprintPure, Category="GTT|RuralWork") EGTTRuralWorkStage GetStage() const { return Stage; }
    UFUNCTION(BlueprintPure, Category="GTT|RuralWork") FString GetObjectiveText() const;
    UFUNCTION(BlueprintPure, Category="GTT|RuralWork") int32 GetMowingPassesCompleted() const { return MowingPassesCompleted; }

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|RuralWork|Timber") float TimberTimeLimit = 190.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|RuralWork|Timber") int32 TimberBaseReward = 340;
    UPROPERTY(EditDefaultsOnly, Category="GTT|RuralWork|Timber") int32 TimberFastBonus = 110;
    UPROPERTY(EditDefaultsOnly, Category="GTT|RuralWork|Mowing") float MowingTimeLimit = 210.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|RuralWork|Mowing") int32 MowingReward = 390;
    UPROPERTY(EditDefaultsOnly, Category="GTT|RuralWork|Mowing") int32 RequiredMowingPasses = 5;

private:
    bool CanTakeLegalWork(APawn* PlayerPawn) const;
    class AGTTVehicleBase* FindNearbyVehicle(APawn* PlayerPawn, float Radius, bool bRequireTractor) const;
    void FinishWork(APawn* PlayerPawn, int32 Reward, const FString& Message);
    void FailWork(APawn* PlayerPawn, const FString& Reason);
    void PushMessage(APawn* Pawn, const FString& Message, float Duration = 5.0f) const;

    EGTTRuralWorkType WorkType = EGTTRuralWorkType::None;
    EGTTRuralWorkStage Stage = EGTTRuralWorkStage::Idle;
    float TimeRemaining = 0.0f;
    float CargoIntegrity = 1.0f;
    int32 MowingPassesCompleted = 0;
};
