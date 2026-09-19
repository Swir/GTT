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
    UPROPERTY(BlueprintReadOnly) int32 QueuePosition = 0;
    UPROPERTY(BlueprintReadOnly) bool bUrgent = false;
    UPROPERTY(BlueprintReadOnly) FString Priority = TEXT("STANDARD");
    UPROPERTY(BlueprintReadOnly) bool bCheckedIn = false;
    UPROPERTY(BlueprintReadOnly) int32 ServiceStartDay = 0;
    UPROPERTY(BlueprintReadOnly) float ServiceStartHour = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 ServiceCompleteDay = 0;
    UPROPERTY(BlueprintReadOnly) float ServiceCompleteHour = 0.0f;
    UPROPERTY(BlueprintReadOnly) float HoursUntilServiceComplete = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bReadyForPickup = false;
    UPROPERTY(BlueprintReadOnly) int32 PaidAmount = 0;
    UPROPERTY(BlueprintReadOnly) int32 PaidDay = 0;
    UPROPERTY(BlueprintReadOnly) float PaidHour = 0.0f;
    UPROPERTY(BlueprintReadOnly) FString State = TEXT("EMPTY");
};

/**
 * Persistent deferred-repair appointments for ordinary damaged/mobile native road vehicles.
 *
 * Up to four exact vehicles can reserve separate request-time locked quotes while the workshop
 * is closed. Each booking gets a deterministic service slot. At/after that slot the exact vehicle
 * must physically check in at a real workshop terminal. Service then consumes real world time
 * derived from its condition, tires, body damage and fuel deficit. STANDARD appointments retain
 * their original quote and timing; an explicit URGENT promotion adds a deterministic checkout
 * premium, advances ahead of waiting STANDARD work and shortens the service timer without any
 * pre-charge. Successful checkout persists READY_FOR_PICKUP until the exact repaired vehicle is
 * collected from the workshop, which keeps fleet return and queue capacity visible to the player.
 * Hard TOW/IMMOBILE WORKSHOP HOLD stays on the separate immediate emergency lane.
 */
UCLASS()
class GTT_API UGTTWorkshopRepairQueueSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    static constexpr int32 MaxQueuedRepairs = 4;
    static constexpr float AppointmentSpacingHours = 0.75f;
    static constexpr float MinimumServiceDurationHours = 0.50f;
    static constexpr float MaximumServiceDurationHours = 1.50f;
    static constexpr int32 UrgentQuoteSurchargePercent = 20;
    static constexpr float UrgentServiceDurationMultiplier = 0.80f;

    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Workshop|Queue")
    bool TryQueueNearestEligibleNativeRoadVehicle(const FVector& Origin, float SearchRadius, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Workshop|Queue")
    bool CancelQueuedRepair(FName VehicleId, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Workshop|Queue")
    bool PromoteQueuedRepairToUrgent(FName VehicleId, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Workshop|Queue")
    bool ReleaseCompletedRepairForPickup(FName VehicleId, FString& OutSummary);

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    bool HasQueuedRepair() const { return QueueEntries.Num() > 0; }

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    bool HasQueuedRepairForVehicle(FName VehicleId) const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    bool IsVehicleInWorkshopService(FName VehicleId) const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    bool IsVehicleAwaitingPickup(FName VehicleId) const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    int32 GetQueuedRepairCount() const { return QueueEntries.Num(); }

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    int32 GetQueueCapacity() const { return MaxQueuedRepairs; }

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    int32 GetUrgentQuoteForVehicle(FName VehicleId) const;

    // Compatibility accessors expose the first/next appointment used by 0.1.45/0.1.46 UI/evidence.
    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    FName GetQueuedVehicleId() const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    int32 GetLockedQuote() const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    FGTTWorkshopRepairQueueSnapshot GetQueueSnapshot() const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    TArray<FGTTWorkshopRepairQueueSnapshot> GetQueueSnapshots() const;

    UFUNCTION(BlueprintPure, Category="GTT|Workshop|Queue")
    FText GetQueueStatusText() const;

private:
    void LoadCheckpointOnce();
    bool WriteCheckpoint();
    void ClearCheckpoint(const TCHAR* Reason);
    void RemoveEntryAt(int32 Index, const TCHAR* Reason);
    bool IsCargoVehicleCompatible(FName VehicleId) const;
    bool IsVehicleImpounded(FName VehicleId) const;
    bool RequiresHardWorkshopHold(FName VehicleId) const;
    AGTTRoadVehicleNativePawn* FindExactQueuedVehicle(FName VehicleId, bool& bAmbiguous) const;
    bool IsVehicleAtWorkshop(const AGTTRoadVehicleNativePawn* Vehicle) const;
    bool ResolveClock(int32& OutDay, float& OutHour) const;
    void ResolveNextAppointment(int32 RequestDay, float RequestHour, int32& OutReadyDay, float& OutReadyHour) const;
    void ResolveServiceCompletion(int32 StartDay, float StartHour, float DurationHours, int32& OutDay, float& OutHour) const;
    float CalculateServiceDurationHours(const AGTTRoadVehicleNativePawn* Vehicle, bool bUrgent) const;
    int32 CalculateUrgentQuote(int32 StandardQuote) const;
    void AdvanceUrgentAppointment(int32 UrgentIndex);
    void SortQueueForServiceOrder();
    void TryExecuteReadyReservations();
    FGTTWorkshopRepairQueueSnapshot BuildSnapshot(const FGTTWorkshopRepairQueueSnapshot& Entry, int32 Position) const;

    bool bLoaded = false;
    TArray<FGTTWorkshopRepairQueueSnapshot> QueueEntries;
    float TickAccumulator = 0.0f;
    float NoticeCooldown = 0.0f;
};
