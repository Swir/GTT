#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRoadRunDirector.generated.h"

UENUM(BlueprintType)
enum class EGTTRoadRunStage : uint8
{
    Idle,
    CollectParts,
    DeliverParts
};

UCLASS()
class GTT_API AGTTRoadRunDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTRoadRunDirector();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|RoadRun")
    bool TryStartContract(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category="GTT|RoadRun")
    bool IsActive() const { return Stage != EGTTRoadRunStage::Idle; }

    UFUNCTION(BlueprintPure, Category="GTT|RoadRun")
    EGTTRoadRunStage GetStage() const { return Stage; }

    UFUNCTION(BlueprintPure, Category="GTT|RoadRun")
    float GetTimeRemaining() const { return TimeRemaining; }

    UFUNCTION(BlueprintPure, Category="GTT|RoadRun")
    float GetParcelIntegrity() const { return ParcelIntegrity; }

    UFUNCTION(BlueprintPure, Category="GTT|RoadRun")
    FString GetObjectiveText() const;

private:
    APawn* ResolvePlayerPawn() const;
    bool IsRattlebackControlled(APawn*& OutControlledVehicle) const;
    void BeginDelivery(APawn* PlayerPawn, APawn* ControlledVehicle);
    void UpdateDeliveryRisk(float DeltaSeconds, APawn* PlayerPawn, APawn* ControlledVehicle);
    void CompleteContract(APawn* PlayerPawn);
    void FailContract(APawn* PlayerPawn, const FString& Reason);
    void PushMessage(APawn* PlayerPawn, const FString& Message, float Duration = 5.0f) const;

    EGTTRoadRunStage Stage = EGTTRoadRunStage::Idle;
    float TimeRemaining = 0.0f;
    float ParcelIntegrity = 1.0f;
    int32 NativeImpactBaseline = 0;
    int32 NativeImpactCountDuringRun = 0;
    float StatusMessageCooldown = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun")
    FVector PartsPickupLocation = FVector(-1250.0f, -470.0f, 80.0f);

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun")
    FVector DeliveryLocation = FVector(7850.0f, 450.0f, 80.0f);

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun", meta=(ClampMin="30.0"))
    float DeliveryTimeLimit = 155.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun", meta=(ClampMin="100.0"))
    float CheckpointRadius = 520.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun", meta=(ClampMin="20.0"))
    float SafeCruiseSpeedKmh = 78.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun", meta=(ClampMin="0"))
    int32 BaseReward = 260;

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun", meta=(ClampMin="0"))
    int32 FastDeliveryBonus = 90;

    UPROPERTY(EditDefaultsOnly, Category="GTT|RoadRun", meta=(ClampMin="0"))
    int32 CleanRunBonus = 40;
};
