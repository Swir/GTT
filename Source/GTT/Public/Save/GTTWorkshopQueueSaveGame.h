#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTWorkshopQueueSaveGame.generated.h"

USTRUCT(BlueprintType)
struct GTT_API FGTTWorkshopQueueSaveEntry
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) FName PersistentVehicleId = NAME_None;
    UPROPERTY(SaveGame) int32 LockedQuote = 0;
    UPROPERTY(SaveGame) int32 RequestedDay = 1;
    UPROPERTY(SaveGame) float RequestedHour = 0.0f;
    UPROPERTY(SaveGame) int32 ReadyDay = 1;
    UPROPERTY(SaveGame) float ReadyHour = 6.5f;

    // 0.1.51 additive priority contract. Missing fields deserialize as STANDARD.
    UPROPERTY(SaveGame) bool bUrgent = false;

    // 0.1.49 additive lifecycle checkpoint. False/zero values mean the appointment is waiting.
    UPROPERTY(SaveGame) bool bCheckedIn = false;
    UPROPERTY(SaveGame) int32 ServiceStartDay = 0;
    UPROPERTY(SaveGame) float ServiceStartHour = 0.0f;
    UPROPERTY(SaveGame) int32 ServiceCompleteDay = 0;
    UPROPERTY(SaveGame) float ServiceCompleteHour = 0.0f;

    // 0.1.51 additive paid-service pickup checkpoint. Missing fields mean normal pre-checkout work.
    UPROPERTY(SaveGame) bool bReadyForPickup = false;
    UPROPERTY(SaveGame) int32 PaidAmount = 0;
    UPROPERTY(SaveGame) int32 PaidDay = 0;
    UPROPERTY(SaveGame) float PaidHour = 0.0f;
};

/**
 * Transactional sidecar for deferred workshop appointments.
 *
 * SchemaVersion intentionally remains 1 because the original single-reservation fields are
 * retained as a first-entry mirror. Existing 0.1.45/0.1.46 saves therefore load without a
 * destructive migration, while Appointments extends the format additively for multiple exact-
 * vehicle reservations, the 0.1.49 timed lifecycle, and 0.1.51 urgency/pickup state. Missing
 * additive fields deserialize to STANDARD waiting work with no pickup claim. Cash and repair
 * mutation remain authoritative in the production workshop queue.
 */
UCLASS()
class GTT_API UGTTWorkshopQueueSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame) int32 SchemaVersion = 1;

    // Legacy first-entry mirror kept for backward compatibility and packaged 0.1.46 evidence.
    UPROPERTY(SaveGame) bool bQueued = false;
    UPROPERTY(SaveGame) FName PersistentVehicleId = NAME_None;
    UPROPERTY(SaveGame) int32 LockedQuote = 0;
    UPROPERTY(SaveGame) int32 RequestedDay = 1;
    UPROPERTY(SaveGame) float RequestedHour = 0.0f;
    UPROPERTY(SaveGame) int32 ReadyDay = 1;
    UPROPERTY(SaveGame) float ReadyHour = 6.5f;

    // Additive multi-vehicle queue. Empty means "read the legacy mirror".
    UPROPERTY(SaveGame) TArray<FGTTWorkshopQueueSaveEntry> Appointments;
};
