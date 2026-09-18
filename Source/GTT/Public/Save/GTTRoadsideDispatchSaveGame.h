#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTRoadsideDispatchSaveGame.generated.h"

/**
 * Tiny transactional checkpoint for a voluntary roadside dispatch that is still in flight.
 *
 * This is intentionally separate from the long-lived campaign snapshot: the service lasts
 * only a few seconds and must be cleared immediately on completion/cancel. The checkpoint
 * contains no economy authority; cash is still charged only by the production recovery
 * completion path.
 */
UCLASS()
class GTT_API UGTTRoadsideDispatchSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 SchemaVersion = 1;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") bool bDispatchPending = false;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") uint8 RecoveryMode = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") FName PersistentVehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 LockedQuote = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") float SecondsRemaining = 0.0f;

    // Diagnostic link to the ordinary primary world snapshot. This is not used to
    // manufacture progression and never replaces the primary save revision.
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 PrimaryWorldStateRevision = 0;
};
