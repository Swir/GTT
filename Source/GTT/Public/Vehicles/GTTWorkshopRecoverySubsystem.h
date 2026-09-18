#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTWorkshopRecoverySubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class APawn;

USTRUCT(BlueprintType)
struct GTT_API FGTTWorkshopServiceQuote
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName PersistentVehicleId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 TotalCost = 0;
    UPROPERTY(BlueprintReadOnly) int32 BodyDamageSurcharge = 0;
    UPROPERTY(BlueprintReadOnly) float ConditionPercent = 1.0f;
    UPROPERTY(BlueprintReadOnly) float TireIntegrity = 1.0f;
    UPROPERTY(BlueprintReadOnly) float FuelLiters = 0.0f;
    UPROPERTY(BlueprintReadOnly) float FuelCapacityLiters = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bAtWorkshop = false;
    UPROPERTY(BlueprintReadOnly) bool bServiceNeeded = false;
    UPROPERTY(BlueprintReadOnly) bool bOwnedByPlayer = false;
};

/**
 * Authoritative paid workshop service for native road vehicles.
 * Roadside tow only moves the exact vehicle to the workshop; this subsystem owns the separate
 * player-authorized repair/refuel/body-restoration transaction once the vehicle is physically there.
 */
UCLASS()
class GTT_API UGTTWorkshopRecoverySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Workshop")
    bool IsVehicleAtWorkshop(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Workshop")
    FGTTWorkshopServiceQuote GetWorkshopServiceQuote(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Workshop")
    AGTTRoadVehicleNativePawn* FindNearbyWorkshopVehicle(const APawn* CustomerPawn) const;

    /**
     * Purchase a complete workshop service. Cash is charged exactly once only after all eligibility
     * checks pass. Failed post-service verification rolls vehicle state back and refunds the charge.
     */
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Workshop")
    bool PurchaseFullWorkshopService(AGTTRoadVehicleNativePawn* Vehicle, APawn* CustomerPawn);

private:
    bool ValidateWorkshopCustomer(const AGTTRoadVehicleNativePawn* Vehicle, const APawn* CustomerPawn, FString& OutReason) const;
    void MaybeShowWorkshopPrompt(APawn* CustomerPawn, AGTTRoadVehicleNativePawn* Vehicle);

    float ScanAccumulator = 0.0f;
    float PromptCooldown = 0.0f;
};
