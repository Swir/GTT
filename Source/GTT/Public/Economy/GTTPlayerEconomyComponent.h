#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTTPlayerEconomyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGTTEconomyChanged, int32, Cash, int32, FishCount, float, FishWeightKg);

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTPlayerEconomyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTPlayerEconomyComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Economy")
    void AddCash(int32 Amount, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category="GTT|Economy")
    bool SpendCash(int32 Amount, const FString& Reason);

    UFUNCTION(BlueprintCallable, Category="GTT|Economy")
    void AddFish(float WeightKg, const FString& Species);

    UFUNCTION(BlueprintCallable, Category="GTT|Economy")
    int32 SellAllFish(float PricePerKg);

    UFUNCTION(BlueprintCallable, Category="GTT|Economy")
    void PushMessage(const FString& Message, float Duration = 4.0f);

    UFUNCTION(BlueprintPure, Category="GTT|Economy")
    int32 GetCash() const { return Cash; }

    UFUNCTION(BlueprintPure, Category="GTT|Economy")
    int32 GetFishCount() const { return FishCount; }

    UFUNCTION(BlueprintPure, Category="GTT|Economy")
    float GetFishWeightKg() const { return FishWeightKg; }

    UFUNCTION(BlueprintPure, Category="GTT|Economy")
    FString GetActivityMessage() const { return ActivityMessage; }

    UPROPERTY(BlueprintAssignable, Category="GTT|Economy")
    FGTTEconomyChanged OnEconomyChanged;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Economy", meta=(ClampMin="0"))
    int32 StartingCash = 120;

private:
    void BroadcastEconomy();

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Economy")
    int32 Cash = 0;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Economy")
    int32 FishCount = 0;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Economy")
    float FishWeightKg = 0.0f;

    FString ActivityMessage;
    float ActivityMessageTimeRemaining = 0.0f;
};
