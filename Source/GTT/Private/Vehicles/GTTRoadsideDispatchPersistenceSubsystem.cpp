#include "Vehicles/GTTRoadsideDispatchPersistenceSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
#include "Save/GTTRoadsideDispatchSaveGame.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTT.h"

namespace
{
    const FString RoadsideDispatchSlot(TEXT("GTT_RoadsideDispatch_01"));
    const FString PrimaryWorldSlot(TEXT("GTT_Prototype_01"));
    constexpr int32 SaveUserIndex = 0;
    constexpr float CheckpointIntervalSeconds = 0.25f;
    constexpr float RemainingEtaWriteThresholdSeconds = 0.35f;
}

TStatId UGTTRoadsideDispatchPersistenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTRoadsideDispatchPersistenceSubsystem, STATGROUP_Tickables);
}

void UGTTRoadsideDispatchPersistenceSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    CheckpointAccumulator += DeltaSeconds;
    if (CheckpointAccumulator < CheckpointIntervalSeconds) return;
    CheckpointAccumulator = 0.0f;

    if (!bCheckpointLoaded)
    {
        LoadCheckpointOnce();
    }

    if (bRestorePending)
    {
        if (!TryRestoreLoadedCheckpoint())
        {
            // Exact vehicle/driver is not ready yet. Do not decrement or overwrite the
            // saved ETA while waiting for actor recreation / player re-entry.
            return;
        }
    }

    CaptureLiveCheckpoint();
}

void UGTTRoadsideDispatchPersistenceSubsystem::LoadCheckpointOnce()
{
    bCheckpointLoaded = true;
    bRestorePending = false;
    bCheckpointOnDisk = false;

    UGTTRoadsideDispatchSaveGame* Save = Cast<UGTTRoadsideDispatchSaveGame>(
        UGameplayStatics::LoadGameFromSlot(RoadsideDispatchSlot, SaveUserIndex));
    if (!Save) return;

    bCheckpointOnDisk = true;
    if (Save->SchemaVersion != 1
        || !Save->bDispatchPending
        || Save->PersistentVehicleId.IsNone()
        || Save->LockedQuote <= 0
        || Save->SecondsRemaining <= 0.0f)
    {
        ClearCheckpoint(TEXT("INVALID_OR_EMPTY"));
        return;
    }

    RestoreMode = static_cast<EGTTRoadsideRecoveryMode>(Save->RecoveryMode);
    RestoreVehicleId = Save->PersistentVehicleId;
    RestoreLockedQuote = Save->LockedQuote;
    RestoreSecondsRemaining = Save->SecondsRemaining;

    if (RestoreMode != EGTTRoadsideRecoveryMode::EmergencyPatch
        && RestoreMode != EGTTRoadsideRecoveryMode::RoadsideAssistance)
    {
        ClearCheckpoint(TEXT("NON_VOLUNTARY_MODE"));
        return;
    }

    if (!IsCargoVehicleCompatible(RestoreVehicleId))
    {
        ClearCheckpoint(TEXT("FARM_CARGO_ID_MISMATCH"));
        return;
    }

    bRestorePending = true;
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_LOADED vehicle=%s mode=%d locked_quote=%d eta=%.2f primary_revision=%d charged=NO"),
        *RestoreVehicleId.ToString(), static_cast<int32>(RestoreMode), RestoreLockedQuote,
        RestoreSecondsRemaining, Save->PrimaryWorldStateRevision);
}

bool UGTTRoadsideDispatchPersistenceSubsystem::TryRestoreLoadedCheckpoint()
{
    UWorld* World = GetWorld();
    UGTTRoadsideRecoverySubsystem* Roadside = World ? World->GetSubsystem<UGTTRoadsideRecoverySubsystem>() : nullptr;
    if (!Roadside) return false;

    const EGTTRoadsideRecoveryRestoreResult Result = Roadside->RestorePendingRecoveryCheckpoint(
        RestoreMode, RestoreVehicleId, RestoreLockedQuote, RestoreSecondsRemaining);

    if (Result == EGTTRoadsideRecoveryRestoreResult::WaitingForVehicle)
    {
        return false;
    }

    bRestorePending = false;
    if (Result == EGTTRoadsideRecoveryRestoreResult::Rejected)
    {
        ClearCheckpoint(TEXT("RESTORE_REJECTED"));
        return true;
    }

    LastSavedMode = RestoreMode;
    LastSavedVehicleId = RestoreVehicleId;
    LastSavedLockedQuote = RestoreLockedQuote;
    LastSavedSecondsRemaining = RestoreSecondsRemaining;
    LastSavedPrimaryRevision = ReadPrimaryWorldRevision();
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_REBOUND vehicle=%s locked_quote=%d eta=%.2f exact_id=YES charged=NO"),
        *RestoreVehicleId.ToString(), RestoreLockedQuote, RestoreSecondsRemaining);
    return true;
}

void UGTTRoadsideDispatchPersistenceSubsystem::CaptureLiveCheckpoint()
{
    UWorld* World = GetWorld();
    UGTTRoadsideRecoverySubsystem* Roadside = World ? World->GetSubsystem<UGTTRoadsideRecoverySubsystem>() : nullptr;
    if (!World || !Roadside) return;

    AGTTRoadVehicleNativePawn* PendingVehicle = nullptr;
    int32 PendingCount = 0;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!Candidate || !Roadside->HasPendingRoadsideService(Candidate)) continue;
        PendingVehicle = Candidate;
        ++PendingCount;
    }

    if (PendingCount > 1)
    {
        UE_LOG(LogGTT, Error,
            TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SKIPPED reason=AMBIGUOUS_PENDING_SERVICES count=%d"),
            PendingCount);
        return;
    }

    if (!PendingVehicle)
    {
        if (bCheckpointOnDisk) ClearCheckpoint(TEXT("NO_LIVE_DISPATCH"));
        return;
    }

    const EGTTRoadsideRecoveryMode Mode = Roadside->GetPendingRecoveryMode(PendingVehicle);
    const FName VehicleId = Roadside->GetPendingRecoveryVehicleId(PendingVehicle);
    const int32 LockedQuote = Roadside->GetPendingRecoveryQuote(PendingVehicle);
    const float SecondsRemaining = Roadside->GetPendingRecoverySecondsRemaining(PendingVehicle);
    const int32 PrimaryRevision = ReadPrimaryWorldRevision();

    if ((Mode != EGTTRoadsideRecoveryMode::EmergencyPatch
            && Mode != EGTTRoadsideRecoveryMode::RoadsideAssistance)
        || VehicleId.IsNone()
        || LockedQuote <= 0
        || SecondsRemaining <= 0.0f)
    {
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SKIPPED reason=INVALID_LIVE_CONTRACT vehicle=%s quote=%d eta=%.2f"),
            *VehicleId.ToString(), LockedQuote, SecondsRemaining);
        return;
    }

    if (!IsCargoVehicleCompatible(VehicleId))
    {
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SKIPPED reason=FARM_CARGO_ID_MISMATCH vehicle=%s"),
            *VehicleId.ToString());
        return;
    }

    const bool bContractChanged =
        Mode != LastSavedMode
        || VehicleId != LastSavedVehicleId
        || LockedQuote != LastSavedLockedQuote
        || PrimaryRevision != LastSavedPrimaryRevision;
    const bool bEtaCheckpointDue =
        LastSavedSecondsRemaining < 0.0f
        || FMath::Abs(SecondsRemaining - LastSavedSecondsRemaining) >= RemainingEtaWriteThresholdSeconds;
    if (!bContractChanged && !bEtaCheckpointDue) return;

    UGTTRoadsideDispatchSaveGame* Save = Cast<UGTTRoadsideDispatchSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UGTTRoadsideDispatchSaveGame::StaticClass()));
    if (!Save) return;

    Save->bDispatchPending = true;
    Save->RecoveryMode = static_cast<uint8>(Mode);
    Save->PersistentVehicleId = VehicleId;
    Save->LockedQuote = LockedQuote;
    Save->SecondsRemaining = SecondsRemaining;
    Save->PrimaryWorldStateRevision = PrimaryRevision;

    if (!UGameplayStatics::SaveGameToSlot(Save, RoadsideDispatchSlot, SaveUserIndex))
    {
        UE_LOG(LogGTT, Error,
            TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_WRITE_FAILED vehicle=%s"),
            *VehicleId.ToString());
        return;
    }

    bCheckpointOnDisk = true;
    LastSavedMode = Mode;
    LastSavedVehicleId = VehicleId;
    LastSavedLockedQuote = LockedQuote;
    LastSavedSecondsRemaining = SecondsRemaining;
    LastSavedPrimaryRevision = PrimaryRevision;
    UE_LOG(LogGTT, VeryVerbose,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SAVED vehicle=%s mode=%d locked_quote=%d eta=%.2f primary_revision=%d charged=NO"),
        *VehicleId.ToString(), static_cast<int32>(Mode), LockedQuote, SecondsRemaining, PrimaryRevision);
}

void UGTTRoadsideDispatchPersistenceSubsystem::ClearCheckpoint(const TCHAR* Reason)
{
    if (UGameplayStatics::DoesSaveGameExist(RoadsideDispatchSlot, SaveUserIndex))
    {
        UGameplayStatics::DeleteGameInSlot(RoadsideDispatchSlot, SaveUserIndex);
    }

    bCheckpointOnDisk = false;
    bRestorePending = false;
    RestoreMode = EGTTRoadsideRecoveryMode::None;
    RestoreVehicleId = NAME_None;
    RestoreLockedQuote = 0;
    RestoreSecondsRemaining = 0.0f;
    LastSavedMode = EGTTRoadsideRecoveryMode::None;
    LastSavedVehicleId = NAME_None;
    LastSavedLockedQuote = 0;
    LastSavedSecondsRemaining = -1.0f;
    LastSavedPrimaryRevision = INDEX_NONE;
    UE_LOG(LogGTT, VeryVerbose,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_CLEARED reason=%s"), Reason ? Reason : TEXT("UNKNOWN"));
}

int32 UGTTRoadsideDispatchPersistenceSubsystem::ReadPrimaryWorldRevision() const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    return Primary ? Primary->UnifiedWorldStateRevision : 0;
}

bool UGTTRoadsideDispatchPersistenceSubsystem::IsCargoVehicleCompatible(FName VehicleId) const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    if (!Primary || !Primary->bFarmCargoContractActive || Primary->FarmCargoBoundVehicleId.IsNone())
    {
        return true;
    }

    // When the primary snapshot says a physical load exists, a pending roadside service
    // may only be restored/captured for that same authoritative cargo vehicle.
    return Primary->FarmCargoBoundVehicleId == VehicleId;
}
