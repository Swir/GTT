#include "Vehicles/GTTRoadsideRecoverySubsystem.h"

#include "Core/GTTGameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    // Keep these synchronized with the production dispatch windows in
    // GTTRoadsideRecoverySubsystem.cpp. The source verifier compares both files.
    constexpr float PersistedTowDispatchSeconds = 2.5f;
    constexpr float PersistedPatchDispatchSeconds = 3.0f;
    constexpr float MinimumRestoredEtaSeconds = 0.05f;
}

EGTTRoadsideRecoveryRestoreResult UGTTRoadsideRecoverySubsystem::RestorePendingRecoveryCheckpoint(
    EGTTRoadsideRecoveryMode Mode,
    FName PersistentVehicleId,
    int32 LockedQuote,
    float SecondsRemaining)
{
    if ((Mode != EGTTRoadsideRecoveryMode::EmergencyPatch
            && Mode != EGTTRoadsideRecoveryMode::RoadsideAssistance)
        || PersistentVehicleId.IsNone()
        || LockedQuote <= 0
        || SecondsRemaining <= 0.0f)
    {
        GTT_LOG( Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s mode=%d quote=%d eta=%.2f reason=INVALID_CHECKPOINT charged=NO"),
            *PersistentVehicleId.ToString(), static_cast<int32>(Mode), LockedQuote, SecondsRemaining);
        return EGTTRoadsideRecoveryRestoreResult::Rejected;
    }

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return EGTTRoadsideRecoveryRestoreResult::WaitingForVehicle;
    }

    AGTTRoadVehicleNativePawn* TargetVehicle = nullptr;
    int32 ExactMatchCount = 0;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!Candidate || Candidate->GetPersistentVehicleId() != PersistentVehicleId) continue;
        TargetVehicle = Candidate;
        ++ExactMatchCount;
    }

    if (ExactMatchCount > 1)
    {
        GTT_LOG( Error,
            TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s reason=DUPLICATE_PERSISTENT_ID matches=%d charged=NO"),
            *PersistentVehicleId.ToString(), ExactMatchCount);
        return EGTTRoadsideRecoveryRestoreResult::Rejected;
    }

    // Actor recreation / garage spawning can lag the primary load. Do not bind to a
    // substitute vehicle and do not let the ETA run while the exact actor is absent.
    if (!TargetVehicle || !TargetVehicle->IsLegacyTakeoverActive() || !TargetVehicle->GetDriverPawn())
    {
        return EGTTRoadsideRecoveryRestoreResult::WaitingForVehicle;
    }

    APawn* Driver = TargetVehicle->GetDriverPawn();
    const UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    if (WantedLevel > 0)
    {
        GTT_LOG( Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s reason=WANTED wanted=%d charged=NO"),
            *PersistentVehicleId.ToString(), WantedLevel);
        return EGTTRoadsideRecoveryRestoreResult::Rejected;
    }

    if (!IsPlayerRecoveryChoiceEligible(TargetVehicle))
    {
        GTT_LOG( Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s reason=NO_LONGER_ELIGIBLE charged=NO"),
            *PersistentVehicleId.ToString());
        return EGTTRoadsideRecoveryRestoreResult::Rejected;
    }

    if (Mode == EGTTRoadsideRecoveryMode::EmergencyPatch)
    {
        const UGTTBreakdownDecisionSubsystem* Decision = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
        if (!Decision || !Decision->CanEmergencyPatch(TargetVehicle))
        {
            GTT_LOG( Warning,
                TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s reason=PATCH_NO_LONGER_VALID charged=NO"),
                *PersistentVehicleId.ToString());
            return EGTTRoadsideRecoveryRestoreResult::Rejected;
        }
    }

    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(TargetVehicle);
    if (Runtime.CooldownSeconds > 0.0f)
    {
        GTT_LOG( Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s reason=RECOVERY_COOLDOWN charged=NO"),
            *PersistentVehicleId.ToString());
        return EGTTRoadsideRecoveryRestoreResult::Rejected;
    }

    // Idempotent retry after a restore is harmless. A different live request is never
    // overwritten by checkpoint data.
    if (Runtime.bTowRequested || Runtime.bPatchRequested)
    {
        const bool bSameMode =
            (Mode == EGTTRoadsideRecoveryMode::EmergencyPatch && Runtime.bPatchRequested)
            || (Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && Runtime.bTowRequested);
        const int32 RuntimeQuote = Runtime.bPatchRequested ? Runtime.PendingPatchQuote : Runtime.PendingTowQuote;
        if (bSameMode
            && Runtime.PendingPersistentVehicleId == PersistentVehicleId
            && RuntimeQuote == LockedQuote)
        {
            return EGTTRoadsideRecoveryRestoreResult::Restored;
        }

        GTT_LOG( Warning,
            TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORE_REJECTED vehicle=%s reason=LIVE_DISPATCH_CONFLICT charged=NO"),
            *PersistentVehicleId.ToString());
        return EGTTRoadsideRecoveryRestoreResult::Rejected;
    }

    const float DispatchDuration = Mode == EGTTRoadsideRecoveryMode::EmergencyPatch
        ? PersistedPatchDispatchSeconds
        : PersistedTowDispatchSeconds;
    const float ClampedRemaining = FMath::Clamp(SecondsRemaining, MinimumRestoredEtaSeconds, DispatchDuration);

    ResetPendingService(Runtime, false);
    Runtime.Mode = Mode;
    Runtime.StrandedSeconds = DispatchDuration - ClampedRemaining;
    Runtime.PendingPersistentVehicleId = PersistentVehicleId;
    Runtime.bAnnounced = true;
    if (Mode == EGTTRoadsideRecoveryMode::EmergencyPatch)
    {
        Runtime.bPatchRequested = true;
        Runtime.PendingPatchQuote = LockedQuote;
    }
    else
    {
        Runtime.bTowRequested = true;
        Runtime.PendingTowQuote = LockedQuote;
    }

    GTT_LOG( Display,
        TEXT("NATIVE_ROADSIDE_DISPATCH_RESTORED vehicle=%s mode=%s locked_quote=%d eta=%.2f exact_id=YES charged=NO"),
        *PersistentVehicleId.ToString(),
        Mode == EGTTRoadsideRecoveryMode::EmergencyPatch ? TEXT("PATCH") : TEXT("TOW"),
        LockedQuote, ClampedRemaining);
    return EGTTRoadsideRecoveryRestoreResult::Restored;
}
