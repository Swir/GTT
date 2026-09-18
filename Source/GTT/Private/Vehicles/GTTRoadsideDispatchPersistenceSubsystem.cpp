#include "Vehicles/GTTRoadsideDispatchPersistenceSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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

    if (!bCheckpointLoaded) LoadCheckpointOnce();

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

bool UGTTRoadsideDispatchPersistenceSubsystem::ReloadCheckpointForRuntimeEvidence()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoDispatchPersistenceScenario")))
    {
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_EVIDENCE_RELOAD result=REJECTED reason=EVIDENCE_FLAG_REQUIRED"));
        return false;
    }

    if (!UGameplayStatics::DoesSaveGameExist(RoadsideDispatchSlot, SaveUserIndex))
    {
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_EVIDENCE_RELOAD result=REJECTED reason=CHECKPOINT_MISSING"));
        return false;
    }

    // Reset only transient reader/capture state. The sidecar stays on disk and remains the
    // sole source for the next LoadCheckpointOnce call; no service/cash state is fabricated.
    ResetInMemoryCheckpointState();
    bCheckpointLoaded = false;
    bCheckpointOnDisk = true;
    CheckpointAccumulator = 0.0f;
    LoadCheckpointOnce();

    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_DISPATCH_EVIDENCE_RELOAD result=%s checkpoint_pending=%s charged=NO"),
        bRestorePending ? TEXT("PASS") : TEXT("REJECTED"), bRestorePending ? TEXT("YES") : TEXT("NO"));
    return bRestorePending;
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
    if (Save->SchemaVersion != 2
        || !Save->bDispatchPending
        || Save->PersistentVehicleId.IsNone()
        || Save->LockedQuote <= 0
        || Save->SecondsRemaining <= 0.0f
        || Save->AuthorizedCash < 0)
    {
        ClearCheckpoint(TEXT("INVALID_OR_EMPTY"));
        return;
    }

    RestoreMode = static_cast<EGTTRoadsideRecoveryMode>(Save->RecoveryMode);
    RestoreVehicleId = Save->PersistentVehicleId;
    RestoreLockedQuote = Save->LockedQuote;
    RestoreSecondsRemaining = Save->SecondsRemaining;
    RestoreAuthorizedCash = Save->AuthorizedCash;
    RestoreAuthorizedPrimaryRevision = Save->AuthorizedPrimaryWorldStateRevision;

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

    // Crash-window replay guard: if a newer/equal primary snapshot already contains a cash
    // drop at least as large as this locked quote, fail closed instead of risking a second
    // charge. If the primary save never recorded the first debit, the service may safely
    // resume and the normal production completion path will charge it exactly once there.
    if (LooksLikeAlreadyCommittedCharge())
    {
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_REPLAY_GUARD vehicle=%s locked_quote=%d authorized_cash=%d current_cash=%d authorized_revision=%d current_revision=%d action=DISCARD charged=NO"),
            *RestoreVehicleId.ToString(), RestoreLockedQuote, RestoreAuthorizedCash, ReadPrimaryCash(),
            RestoreAuthorizedPrimaryRevision, ReadPrimaryWorldRevision());
        ClearCheckpoint(TEXT("POSSIBLE_ALREADY_COMMITTED_CHARGE"));
        return;
    }

    bRestorePending = true;
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_LOADED vehicle=%s mode=%d locked_quote=%d eta=%.2f authorized_cash=%d authorized_revision=%d primary_revision=%d charged=NO"),
        *RestoreVehicleId.ToString(), static_cast<int32>(RestoreMode), RestoreLockedQuote,
        RestoreSecondsRemaining, RestoreAuthorizedCash, RestoreAuthorizedPrimaryRevision,
        Save->PrimaryWorldStateRevision);
}

bool UGTTRoadsideDispatchPersistenceSubsystem::TryRestoreLoadedCheckpoint()
{
    UWorld* World = GetWorld();
    UGTTRoadsideRecoverySubsystem* Roadside = World ? World->GetSubsystem<UGTTRoadsideRecoverySubsystem>() : nullptr;
    if (!Roadside) return false;

    // Re-evaluate the debit guard after actor/world initialization as the primary snapshot
    // may have finished loading since LoadCheckpointOnce().
    if (LooksLikeAlreadyCommittedCharge())
    {
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_REPLAY_GUARD vehicle=%s locked_quote=%d action=DISCARD_ON_RESTORE charged=NO"),
            *RestoreVehicleId.ToString(), RestoreLockedQuote);
        ClearCheckpoint(TEXT("POSSIBLE_ALREADY_COMMITTED_CHARGE"));
        return true;
    }

    const EGTTRoadsideRecoveryRestoreResult Result = Roadside->RestorePendingRecoveryCheckpoint(
        RestoreMode, RestoreVehicleId, RestoreLockedQuote, RestoreSecondsRemaining);

    if (Result == EGTTRoadsideRecoveryRestoreResult::WaitingForVehicle) return false;

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
    AuthorizedCash = RestoreAuthorizedCash;
    AuthorizedPrimaryRevision = RestoreAuthorizedPrimaryRevision;
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
            TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SKIPPED reason=AMBIGUOUS_PENDING_SERVICES count=%d"), PendingCount);
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
        || VehicleId.IsNone() || LockedQuote <= 0 || SecondsRemaining <= 0.0f)
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

    // A new dispatch contract captures its authorization baseline exactly once. ETA refreshes
    // preserve it, so later primary snapshots can identify a likely already-committed debit.
    const bool bNewContract =
        Mode != LastSavedMode || VehicleId != LastSavedVehicleId || LockedQuote != LastSavedLockedQuote;
    if (bNewContract || AuthorizedCash < 0)
    {
        AuthorizedCash = ReadPrimaryCash();
        AuthorizedPrimaryRevision = PrimaryRevision;
    }

    const bool bContractChanged = bNewContract || PrimaryRevision != LastSavedPrimaryRevision;
    const bool bEtaCheckpointDue = LastSavedSecondsRemaining < 0.0f
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
    Save->AuthorizedCash = AuthorizedCash;
    Save->AuthorizedPrimaryWorldStateRevision = AuthorizedPrimaryRevision;
    Save->PrimaryWorldStateRevision = PrimaryRevision;

    if (!UGameplayStatics::SaveGameToSlot(Save, RoadsideDispatchSlot, SaveUserIndex))
    {
        UE_LOG(LogGTT, Error, TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_WRITE_FAILED vehicle=%s"), *VehicleId.ToString());
        return;
    }

    bCheckpointOnDisk = true;
    LastSavedMode = Mode;
    LastSavedVehicleId = VehicleId;
    LastSavedLockedQuote = LockedQuote;
    LastSavedSecondsRemaining = SecondsRemaining;
    LastSavedPrimaryRevision = PrimaryRevision;
    UE_LOG(LogGTT, VeryVerbose,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SAVED vehicle=%s mode=%d locked_quote=%d eta=%.2f authorized_cash=%d authorized_revision=%d primary_revision=%d charged=NO"),
        *VehicleId.ToString(), static_cast<int32>(Mode), LockedQuote, SecondsRemaining,
        AuthorizedCash, AuthorizedPrimaryRevision, PrimaryRevision);
}

void UGTTRoadsideDispatchPersistenceSubsystem::ResetInMemoryCheckpointState()
{
    bRestorePending = false;
    RestoreMode = EGTTRoadsideRecoveryMode::None;
    RestoreVehicleId = NAME_None;
    RestoreLockedQuote = 0;
    RestoreSecondsRemaining = 0.0f;
    RestoreAuthorizedCash = INDEX_NONE;
    RestoreAuthorizedPrimaryRevision = 0;
    LastSavedMode = EGTTRoadsideRecoveryMode::None;
    LastSavedVehicleId = NAME_None;
    LastSavedLockedQuote = 0;
    LastSavedSecondsRemaining = -1.0f;
    LastSavedPrimaryRevision = INDEX_NONE;
    AuthorizedCash = INDEX_NONE;
    AuthorizedPrimaryRevision = 0;
}

void UGTTRoadsideDispatchPersistenceSubsystem::ClearCheckpoint(const TCHAR* Reason)
{
    if (UGameplayStatics::DoesSaveGameExist(RoadsideDispatchSlot, SaveUserIndex))
    {
        UGameplayStatics::DeleteGameInSlot(RoadsideDispatchSlot, SaveUserIndex);
    }

    bCheckpointOnDisk = false;
    ResetInMemoryCheckpointState();
    UE_LOG(LogGTT, VeryVerbose,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_CLEARED reason=%s"), Reason ? Reason : TEXT("UNKNOWN"));
}

int32 UGTTRoadsideDispatchPersistenceSubsystem::ReadPrimaryWorldRevision() const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    return Primary ? Primary->UnifiedWorldStateRevision : 0;
}

int32 UGTTRoadsideDispatchPersistenceSubsystem::ReadPrimaryCash() const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    return Primary ? Primary->Cash : INDEX_NONE;
}

bool UGTTRoadsideDispatchPersistenceSubsystem::LooksLikeAlreadyCommittedCharge() const
{
    if (RestoreAuthorizedCash < 0 || RestoreLockedQuote <= 0) return false;
    const int32 CurrentCash = ReadPrimaryCash();
    const int32 CurrentRevision = ReadPrimaryWorldRevision();
    if (CurrentCash < 0 || CurrentRevision < RestoreAuthorizedPrimaryRevision) return false;
    return CurrentCash <= RestoreAuthorizedCash - RestoreLockedQuote;
}

bool UGTTRoadsideDispatchPersistenceSubsystem::IsCargoVehicleCompatible(FName VehicleId) const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    if (!Primary || !Primary->bFarmCargoContractActive || Primary->FarmCargoBoundVehicleId.IsNone()) return true;

    // When the primary snapshot says a physical load exists, a pending roadside service
    // may only be restored/captured for that same authoritative cargo vehicle.
    return Primary->FarmCargoBoundVehicleId == VehicleId;
}
