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
 * completion path. Schema v2 also remembers the authorization-time cash/revision so a stale
 * sidecar left by a crash after a committed charge can fail closed instead of replaying it.
 */
UCLASS()
class GTT_API UGTTRoadsideDispatchSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 SchemaVersion = 2;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") bool bDispatchPending = false;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") uint8 RecoveryMode = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") FName PersistentVehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 LockedQuote = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") float SecondsRemaining = 0.0f;

    // Snapshot at first authorization/checkpoint. A later primary revision whose persisted
    // cash has fallen by at least the locked quote is treated conservatively as evidence that
    // the service may already have committed before a crash; that sidecar is discarded.
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 AuthorizedCash = INDEX_NONE;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 AuthorizedPrimaryWorldStateRevision = 0;

    // Diagnostic link to the latest ordinary primary world snapshot observed while the ETA
    // checkpoint was refreshed. This is never used to manufacture progression.
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Roadside") int32 PrimaryWorldStateRevision = 0;
};
