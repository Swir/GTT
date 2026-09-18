#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTWorkshopRepairQueueSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;

USTRUCT(BlueprintType)
struct GTT_API FGTTWorkshopRepairQueueSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bQueued = false;
    UPROPERTY(BlueprintReadOnly) FName PersistentVehicleId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 LockedQuote = 0;
    UPROPERTY(BlueprintReadOnly) int32 RequestedDay = 0;
    UPROPERTY(BlueprintReadOnly) float RequestedHour = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 ReadyDay = 0;
    UPROPERTY(BlueprintReadOnly) float ReadyHour = 0.0f;
    UPROPERTY(BlueprintReadOnly) float HoursUntilReady = 0.0f;
    UPROPERTY(BlueprintReadOnly) FString State = TEXT("EMPTY");
};

/**
 * Persistent deferred-repair queue for ordinary damaged/mobile native road vehicles.
 *
 * A reservation locks the exact PersistentVehicleId and repair quote while the workshop is
 * closed, but never charges cash. At/after opening the exact owned vehicle must still exist,
 * remain cargo-compatible, stay free of a hard WORKSHOP HOLD, and be physically parked by a
 * workshop terminal. Only then does the subsystem debit the locked quote once and invoke the
 * existing ApplyNativeWorkshopService() mutation. Hard TOW/IMMOBILE holds remain on the
 * immediate emergency lane and are deliberately never converted into queued reservations.
 */
UCLASS()
class GTT_API UGTTWorkshopRepairQueueSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Workshop|Queue")
    bool TryQueueNearestEligibleNativeRoadVehicle(const FVector& Origin, float SearchRadius, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Workshop|Queue")
    bool CancelQueuedRepair(FName VehicleId, FString& OutSummary);

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    bool HasQueuedRepair() const { return bQueued; }

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    FName GetQueuedVehicleId() const { return QueuedVehicleId; }

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    int32 GetLockedQuote() const { return LockedQuote; }

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    FGTTWorkshopRepairQueueSnapshot GetQueueSnapshot() const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    FText GetQueueStatusText() const;

private:
    void LoadCheckpointOnce();
    bool WriteCheckpoint();
    void ClearCheckpoint(const TCHAR* Reason);
    bool IsCargoVehicleCompatible(FName VehicleId) const;
    bool IsVehicleImpounded(FName VehicleId) const;
    bool RequiresHardWorkshopHold(FName VehicleId) const;
    AGTTRoadVehicleNativePawn* FindExactQueuedVehicle(bool& bAmbiguous) const;
    bool IsVehicleAtWorkshop(const AGTTRoadVehicleNativePawn* Vehicle) const;
    bool ResolveClock(int32& OutDay, float& OutHour) const;
    void TryExecuteReadyReservation();

    bool bLoaded = false;
    bool bQueued = false;
    FName QueuedVehicleId = NAME_None;
    int32 LockedQuote = 0;
    int32 RequestedDay = 0;
    float RequestedHour = 0.0f;
    int32 ReadyDay = 0;
    float ReadyHour = 6.5f;
    float TickAccumulator = 0.0f;
    float NoticeCooldown = 0.0f;
};
