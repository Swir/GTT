#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadsideRecoverySubsystem.h"
#include "GTTRoadsideDispatchPersistenceSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;

/**
 * Persists only an in-flight voluntary roadside transaction.
 *
 * The checkpoint follows the exact PersistentVehicleId, locked quote and remaining ETA.
 * It never stores/charges cash and never persists PoliceImpound. A restored dispatch waits
 * for the exact actor + driver to exist, so actor recreation cannot transfer service to a
 * substitute vehicle. Authorization-time primary cash/revision are retained only as a
 * conservative replay guard if a crash leaves a stale sidecar after a committed charge.
 */
UCLASS()
class GTT_API UGTTRoadsideDispatchPersistenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    /**
     * Packaged-evidence hook for 0.1.40 only. Re-arms the normal sidecar loader after an
     * in-process SaveGame reload so the deterministic Win64 route can prove the same
     * production checkpoint/restore path without manufacturing service or economy state.
     * The hook is rejected unless -GTTFarmCargoDispatchPersistenceScenario is present.
     */
    bool ReloadCheckpointForRuntimeEvidence();

private:
    void LoadCheckpointOnce();
    bool TryRestoreLoadedCheckpoint();
    void CaptureLiveCheckpoint();
    void ClearCheckpoint(const TCHAR* Reason);
    void ResetInMemoryCheckpointState();
    int32 ReadPrimaryWorldRevision() const;
    int32 ReadPrimaryCash() const;
    bool IsCargoVehicleCompatible(FName VehicleId) const;
    bool LooksLikeAlreadyCommittedCharge() const;

    bool bCheckpointLoaded = false;
    bool bRestorePending = false;
    bool bCheckpointOnDisk = false;
    float CheckpointAccumulator = 0.0f;

    EGTTRoadsideRecoveryMode RestoreMode = EGTTRoadsideRecoveryMode::None;
    FName RestoreVehicleId = NAME_None;
    int32 RestoreLockedQuote = 0;
    float RestoreSecondsRemaining = 0.0f;
    int32 RestoreAuthorizedCash = INDEX_NONE;
    int32 RestoreAuthorizedPrimaryRevision = 0;

    EGTTRoadsideRecoveryMode LastSavedMode = EGTTRoadsideRecoveryMode::None;
    FName LastSavedVehicleId = NAME_None;
    int32 LastSavedLockedQuote = 0;
    float LastSavedSecondsRemaining = -1.0f;
    int32 LastSavedPrimaryRevision = INDEX_NONE;
    int32 AuthorizedCash = INDEX_NONE;
    int32 AuthorizedPrimaryRevision = 0;
};
