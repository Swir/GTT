#include "Vehicles/GTTRoadsideRecoverySubsystem.h"

#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    constexpr float RecoveryScanIntervalSeconds = 0.5f;
    constexpr float PoliceImpoundArmSeconds = 7.0f;
    constexpr float PlayerTowDispatchSeconds = 2.5f;
    constexpr float PlayerPatchDispatchSeconds = 3.0f;
    constexpr float RecoveryCooldownSeconds = 12.0f;
    constexpr float MaxRecoverySpeedKmh = 3.5f;
    const FVector WorkshopBaseLocation(-400.0f, 2650.0f, 105.0f);
}

TStatId UGTTRoadsideRecoverySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTRoadsideRecoverySubsystem, STATGROUP_Tickables);
}

void UGTTRoadsideRecoverySubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;
    for (auto& Pair : RuntimeByVehicle)
    {
        Pair.Value.CooldownSeconds = FMath::Max(0.0f, Pair.Value.CooldownSeconds - DeltaSeconds);
    }

    // The native road pawn owns vehicle input while possessed, so recovery choice input is read at world scope.
    // Repeating the same service key is an explicit cancellation gesture; the opposite key cannot silently switch service.
    if (APlayerController* PlayerController = World->GetFirstPlayerController())
    {
        AGTTRoadVehicleNativePawn* NativeVehicle = Cast<AGTTRoadVehicleNativePawn>(PlayerController->GetPawn());
        if (PlayerController->WasInputKeyJustPressed(EKeys::Y) || PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Left))
        {
            if (IsRoadsidePatchPending(NativeVehicle)) CancelPendingRoadsideService(NativeVehicle);
            else RequestEmergencyRoadsidePatch(NativeVehicle);
        }
        if (PlayerController->WasInputKeyJustPressed(EKeys::T) || PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))
        {
            if (IsRoadsideTowPending(NativeVehicle)) CancelPendingRoadsideService(NativeVehicle);
            else RequestRoadsideTow(NativeVehicle);
        }
    }

    ScanAccumulator += DeltaSeconds;
    if (ScanAccumulator < RecoveryScanIntervalSeconds) return;
    const float Step = ScanAccumulator;
    ScanAccumulator = 0.0f;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It) UpdateVehicle(*It, Step);
}

bool UGTTRoadsideRecoverySubsystem::IsRecoveryEligible(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetDriverPawn()) return false;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    const bool bMechanicalBreakdown = State.ConditionPercent <= 0.05f;
    const bool bOutOfFuel = State.FuelLiters <= 0.05f;
    const bool bTiresDisabled = State.TireIntegrity <= 0.08f;
    const bool bBodyDisabled = Body.DetachedPanelCount >= 3
        && FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth)) <= 0.20f;
    return Vehicle->GetVelocity().Size() * 0.036f <= MaxRecoverySpeedKmh
        && (bMechanicalBreakdown || bOutOfFuel || bTiresDisabled || bBodyDisabled);
}

bool UGTTRoadsideRecoverySubsystem::IsPlayerRecoveryChoiceEligible(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetDriverPawn()) return false;
    if (Vehicle->GetVelocity().Size() * 0.036f > MaxRecoverySpeedKmh) return false;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    if (!Decision) return IsRecoveryEligible(Vehicle);
    const FGTTBreakdownAssessment Assessment = Decision->AssessVehicle(Vehicle);
    return Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
        || Assessment.Recommendation == EGTTBreakdownRecommendation::Immobilized;
}

bool UGTTRoadsideRecoverySubsystem::IsRoadsideTowPending(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return false;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(
        TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && Runtime->Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && Runtime->bTowRequested;
}

bool UGTTRoadsideRecoverySubsystem::IsRoadsidePatchPending(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return false;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(
        TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && Runtime->Mode == EGTTRoadsideRecoveryMode::EmergencyPatch && Runtime->bPatchRequested;
}

bool UGTTRoadsideRecoverySubsystem::HasPendingRoadsideService(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    return IsRoadsideTowPending(Vehicle) || IsRoadsidePatchPending(Vehicle);
}

EGTTRoadsideRecoveryMode UGTTRoadsideRecoverySubsystem::GetPendingRecoveryMode(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return EGTTRoadsideRecoveryMode::None;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(
        TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && (Runtime->bTowRequested || Runtime->bPatchRequested) ? Runtime->Mode : EGTTRoadsideRecoveryMode::None;
}

int32 UGTTRoadsideRecoverySubsystem::GetPendingRecoveryQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(
        TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    if (!Runtime) return 0;
    if (Runtime->Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && Runtime->bTowRequested) return Runtime->PendingTowQuote;
    if (Runtime->Mode == EGTTRoadsideRecoveryMode::EmergencyPatch && Runtime->bPatchRequested) return Runtime->PendingPatchQuote;
    return 0;
}

float UGTTRoadsideRecoverySubsystem::GetPendingRecoverySecondsRemaining(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0.0f;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(
        TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    if (!Runtime) return 0.0f;
    if (Runtime->Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && Runtime->bTowRequested)
        return FMath::Max(0.0f, PlayerTowDispatchSeconds - Runtime->StrandedSeconds);
    if (Runtime->Mode == EGTTRoadsideRecoveryMode::EmergencyPatch && Runtime->bPatchRequested)
        return FMath::Max(0.0f, PlayerPatchDispatchSeconds - Runtime->StrandedSeconds);
    return 0.0f;
}

FName UGTTRoadsideRecoverySubsystem::GetPendingRecoveryVehicleId(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return NAME_None;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(
        TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && (Runtime->bTowRequested || Runtime->bPatchRequested) ? Runtime->PendingPersistentVehicleId : NAME_None;
}

void UGTTRoadsideRecoverySubsystem::ResetPendingService(FGTTRoadsideRecoveryRuntime& Runtime, bool bResetAnnouncement)
{
    Runtime.StrandedSeconds = 0.0f;
    Runtime.Mode = EGTTRoadsideRecoveryMode::None;
    Runtime.bTowRequested = false;
    Runtime.bPatchRequested = false;
    Runtime.PendingTowQuote = 0;
    Runtime.PendingPatchQuote = 0;
    Runtime.PendingPersistentVehicleId = NAME_None;
    if (bResetAnnouncement) Runtime.bAnnounced = false;
}

bool UGTTRoadsideRecoverySubsystem::IsPinnedVehicleValid(
    const AGTTRoadVehicleNativePawn* Vehicle,
    const FGTTRoadsideRecoveryRuntime& Runtime) const
{
    return Vehicle
        && !Runtime.PendingPersistentVehicleId.IsNone()
        && Vehicle->GetPersistentVehicleId() == Runtime.PendingPersistentVehicleId;
}

bool UGTTRoadsideRecoverySubsystem::CancelPendingRoadsideService(AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!Vehicle) return false;
    FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(Vehicle);
    if (!Runtime || (!Runtime->bTowRequested && !Runtime->bPatchRequested)) return false;
    if (Runtime->Mode == EGTTRoadsideRecoveryMode::PoliceImpound) return false;

    const EGTTRoadsideRecoveryMode CancelledMode = Runtime->Mode;
    const int32 CancelledQuote = CancelledMode == EGTTRoadsideRecoveryMode::EmergencyPatch
        ? Runtime->PendingPatchQuote : Runtime->PendingTowQuote;
    const FName ExpectedVehicleId = Runtime->PendingPersistentVehicleId;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;

    ResetPendingService(*Runtime);
    if (Economy)
    {
        Economy->PushMessage(
            CancelledMode == EGTTRoadsideRecoveryMode::EmergencyPatch
                ? TEXT("Emergency patch dispatch cancelled. No charge was taken; press Y again if you still need it.")
                : TEXT("Tow dispatch cancelled. No charge was taken; press T again if you still need it."),
            6.0f);
    }
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_DISPATCH_CANCELLED vehicle=%s mode=%s locked_quote=%d charged=NO"),
        *ExpectedVehicleId.ToString(),
        CancelledMode == EGTTRoadsideRecoveryMode::EmergencyPatch ? TEXT("PATCH") : TEXT("TOW"),
        CancelledQuote);
    return true;
}

bool UGTTRoadsideRecoverySubsystem::RequestRoadsideTow(AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!Vehicle || !IsPlayerRecoveryChoiceEligible(Vehicle)) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    if (!Driver || !Economy) return false;

    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    if (WantedLevel > 0)
    {
        Economy->PushMessage(WantedLevel == 1
            ? TEXT("Roadside tow blocked while police are searching.")
            : TEXT("Police control recovery during an active pursuit."), 5.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_TOW_REQUEST_DENIED vehicle=%s reason=WANTED wanted=%d"),
            *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        return false;
    }

    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f) return false;
    if (Runtime.bTowRequested || Runtime.bPatchRequested)
    {
        Economy->PushMessage(Runtime.bPatchRequested
            ? TEXT("Emergency patch service is already inbound. Cancel it with Y before requesting a tow.")
            : TEXT("Tow service is already inbound. Press T again to cancel it."), 5.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_DISPATCH_CONFLICT vehicle=%s requested=TOW active=%s"),
            *Vehicle->GetPersistentVehicleId().ToString(), Runtime.bPatchRequested ? TEXT("PATCH") : TEXT("TOW"));
        return false;
    }

    const int32 TowQuote = CalculateRoadsideCost(Vehicle);
    if (TowQuote <= 0 || Economy->GetCash() < TowQuote)
    {
        Economy->PushMessage(FString::Printf(TEXT("Tow quote is $%d. You do not have enough cash."), TowQuote), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_ROADSIDE_RECOVERY_DENIED vehicle=%s cost=%d reason=INSUFFICIENT_CASH"),
            *Vehicle->GetPersistentVehicleId().ToString(), TowQuote);
        return false;
    }

    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    const int32 RepairQuote = Decision ? Decision->CalculateRepairEstimate(Vehicle) : 0;
    Runtime.Mode = EGTTRoadsideRecoveryMode::RoadsideAssistance;
    Runtime.StrandedSeconds = 0.0f;
    Runtime.bTowRequested = true;
    Runtime.bPatchRequested = false;
    Runtime.PendingTowQuote = TowQuote;
    Runtime.PendingPatchQuote = 0;
    Runtime.PendingPersistentVehicleId = Vehicle->GetPersistentVehicleId();
    Runtime.bAnnounced = true;
    Economy->PushMessage(FString::Printf(
        TEXT("Tow dispatched: locked quote $%d, charged on arrival. Press T again to cancel. Damage is preserved; workshop estimate $%d remains separate."),
        TowQuote, RepairQuote), 8.0f);
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_TOW_REQUESTED vehicle=%s tow_quote=%d quote_locked=YES target_pinned=YES repair_quote=%d player_authorized=YES"),
        *Runtime.PendingPersistentVehicleId.ToString(), TowQuote, RepairQuote);
    return true;
}

bool UGTTRoadsideRecoverySubsystem::RequestEmergencyRoadsidePatch(AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!Vehicle || !IsPlayerRecoveryChoiceEligible(Vehicle)) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    if (!Driver || !Economy || !Decision || !Decision->CanEmergencyPatch(Vehicle)) return false;

    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    if (WantedLevel > 0)
    {
        Economy->PushMessage(WantedLevel == 1
            ? TEXT("Roadside patch blocked while police are searching.")
            : TEXT("Police control recovery during an active pursuit."), 5.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_PATCH_REQUEST_DENIED vehicle=%s reason=WANTED wanted=%d"),
            *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        return false;
    }

    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f) return false;
    if (Runtime.bTowRequested || Runtime.bPatchRequested)
    {
        Economy->PushMessage(Runtime.bTowRequested
            ? TEXT("Tow service is already inbound. Cancel it with T before requesting an emergency patch.")
            : TEXT("Emergency patch service is already inbound. Press Y again to cancel it."), 5.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_DISPATCH_CONFLICT vehicle=%s requested=PATCH active=%s"),
            *Vehicle->GetPersistentVehicleId().ToString(), Runtime.bTowRequested ? TEXT("TOW") : TEXT("PATCH"));
        return false;
    }

    const int32 PatchQuote = Decision->CalculateRoadsidePatchEstimate(Vehicle);
    if (PatchQuote <= 0 || Economy->GetCash() < PatchQuote)
    {
        Economy->PushMessage(FString::Printf(TEXT("Emergency patch costs $%d. You do not have enough cash."), PatchQuote), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_ROADSIDE_PATCH_DENIED vehicle=%s cost=%d reason=INSUFFICIENT_CASH"),
            *Vehicle->GetPersistentVehicleId().ToString(), PatchQuote);
        return false;
    }

    Runtime.Mode = EGTTRoadsideRecoveryMode::EmergencyPatch;
    Runtime.StrandedSeconds = 0.0f;
    Runtime.bPatchRequested = true;
    Runtime.bTowRequested = false;
    Runtime.PendingTowQuote = 0;
    Runtime.PendingPatchQuote = PatchQuote;
    Runtime.PendingPersistentVehicleId = Vehicle->GetPersistentVehicleId();
    Runtime.bAnnounced = true;
    Economy->PushMessage(FString::Printf(
        TEXT("Emergency patch dispatched: locked quote $%d, charged on arrival. Press Y again to cancel. Limp-home service only; body damage and workshop repairs remain."),
        PatchQuote), 8.0f);
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_ROADSIDE_PATCH_REQUESTED vehicle=%s patch_quote=%d quote_locked=YES target_pinned=YES player_authorized=YES"),
        *Runtime.PendingPersistentVehicleId.ToString(), PatchQuote);
    return true;
}

void UGTTRoadsideRecoverySubsystem::UpdateVehicle(AGTTRoadVehicleNativePawn* Vehicle, float DeltaSeconds)
{
    if (!Vehicle) return;
    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    const bool bHardStranded = IsRecoveryEligible(Vehicle);
    const bool bPlayerChoiceEligible = IsPlayerRecoveryChoiceEligible(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f || (!bHardStranded && !bPlayerChoiceEligible))
    {
        if (Runtime.bTowRequested || Runtime.bPatchRequested)
        {
            UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_DISPATCH_DROPPED vehicle=%s reason=NO_LONGER_ELIGIBLE charged=NO"),
                *Runtime.PendingPersistentVehicleId.ToString());
        }
        ResetPendingService(Runtime);
        return;
    }

    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    if (!Driver || !Economy) return;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;

    if (WantedLevel == 1)
    {
        const bool bHadDispatch = Runtime.bTowRequested || Runtime.bPatchRequested;
        ResetPendingService(Runtime, false);
        if (!Runtime.bAnnounced || bHadDispatch)
        {
            Runtime.bAnnounced = true;
            Economy->PushMessage(TEXT("Roadside assistance unavailable while police are searching. Any voluntary dispatch was cancelled without charge."), 6.0f);
            UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_RECOVERY_BLOCKED vehicle=%s wanted=1 cancelled_dispatch=%s charged=NO"),
                *Vehicle->GetPersistentVehicleId().ToString(), bHadDispatch ? TEXT("YES") : TEXT("NO"));
        }
        return;
    }

    if (WantedLevel >= 2)
    {
        const bool bHadDispatch = Runtime.bTowRequested || Runtime.bPatchRequested;
        ResetPendingService(Runtime, false);

        // Police impound remains an automatic, non-cancellable consequence only for a truly stranded vehicle.
        if (!bHardStranded)
        {
            Runtime.StrandedSeconds = 0.0f;
            Runtime.Mode = EGTTRoadsideRecoveryMode::None;
            if (!Runtime.bAnnounced || bHadDispatch)
            {
                Runtime.bAnnounced = true;
                Economy->PushMessage(TEXT("Roadside service blocked during active pursuit. Keep moving or surrender."), 5.0f);
            }
            return;
        }

        Runtime.Mode = EGTTRoadsideRecoveryMode::PoliceImpound;
        Runtime.StrandedSeconds += DeltaSeconds;
        if (!Runtime.bAnnounced)
        {
            Runtime.bAnnounced = true;
            Economy->PushMessage(TEXT("Vehicle disabled during an active pursuit. Police impound response inbound."), 5.0f);
            UE_LOG(LogGTT, Warning, TEXT("NATIVE_POLICE_IMPOUND_ARMED vehicle=%s wanted=%d"),
                *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        }
        if (Runtime.StrandedSeconds >= PoliceImpoundArmSeconds
            && CompleteRecovery(Vehicle, Runtime.Mode))
        {
            ResetPendingService(Runtime);
            Runtime.CooldownSeconds = RecoveryCooldownSeconds;
        }
        return;
    }

    if (Runtime.Mode == EGTTRoadsideRecoveryMode::None && !Runtime.bTowRequested && !Runtime.bPatchRequested)
    {
        Runtime.bAnnounced = false;
    }

    if (Runtime.bPatchRequested && Runtime.Mode == EGTTRoadsideRecoveryMode::EmergencyPatch)
    {
        if (!IsPinnedVehicleValid(Vehicle, Runtime))
        {
            Economy->PushMessage(TEXT("Emergency patch dispatch cancelled: the target vehicle identity changed. No charge was taken."), 7.0f);
            UE_LOG(LogGTT, Warning,
                TEXT("NATIVE_ROADSIDE_DISPATCH_TARGET_MISMATCH expected=%s actual=%s mode=PATCH charged=NO"),
                *Runtime.PendingPersistentVehicleId.ToString(), *Vehicle->GetPersistentVehicleId().ToString());
            ResetPendingService(Runtime);
            return;
        }

        Runtime.StrandedSeconds += DeltaSeconds;
        if (Runtime.StrandedSeconds >= PlayerPatchDispatchSeconds)
        {
            const int32 LockedQuote = Runtime.PendingPatchQuote;
            const FName ExpectedId = Runtime.PendingPersistentVehicleId;
            const bool bCompleted = CompleteEmergencyPatch(Vehicle, LockedQuote, ExpectedId);
            ResetPendingService(Runtime);
            if (bCompleted) Runtime.CooldownSeconds = RecoveryCooldownSeconds;
        }
        return;
    }

    if (Runtime.bTowRequested && Runtime.Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance)
    {
        if (!IsPinnedVehicleValid(Vehicle, Runtime))
        {
            Economy->PushMessage(TEXT("Tow dispatch cancelled: the target vehicle identity changed. No charge was taken."), 7.0f);
            UE_LOG(LogGTT, Warning,
                TEXT("NATIVE_ROADSIDE_DISPATCH_TARGET_MISMATCH expected=%s actual=%s mode=TOW charged=NO"),
                *Runtime.PendingPersistentVehicleId.ToString(), *Vehicle->GetPersistentVehicleId().ToString());
            ResetPendingService(Runtime);
            return;
        }

        Runtime.StrandedSeconds += DeltaSeconds;
        if (Runtime.StrandedSeconds >= PlayerTowDispatchSeconds)
        {
            const int32 LockedQuote = Runtime.PendingTowQuote;
            const FName ExpectedId = Runtime.PendingPersistentVehicleId;
            const bool bCompleted = CompleteRecovery(Vehicle, Runtime.Mode, LockedQuote, ExpectedId);
            ResetPendingService(Runtime);
            if (bCompleted) Runtime.CooldownSeconds = RecoveryCooldownSeconds;
        }
        return;
    }

    Runtime.Mode = EGTTRoadsideRecoveryMode::RoadsideAssistance;
    Runtime.StrandedSeconds = 0.0f;
    if (!Runtime.bAnnounced)
    {
        Runtime.bAnnounced = true;
        const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
        const FGTTBreakdownAssessment Assessment = Decision ? Decision->AssessVehicle(Vehicle) : FGTTBreakdownAssessment();
        if (Assessment.bEmergencyPatchPossible)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("Vehicle recovery choice: Y / D-Pad Left patch $%d for limp-home, or T / D-Pad Up tow $%d. Quotes lock on dispatch; repeat the same key to cancel before arrival. Full workshop repair ~$%d."),
                Assessment.EmergencyPatchEstimate, Assessment.TowEstimate, Assessment.RepairEstimate), 11.0f);
        }
        else
        {
            Economy->PushMessage(FString::Printf(
                TEXT("Vehicle needs recovery. T / D-Pad Up tow $%d; quote locks on dispatch and T again cancels before arrival. Workshop repair ~$%d. Structural/body damage is too severe for a roadside patch."),
                Assessment.TowEstimate, Assessment.RepairEstimate), 11.0f);
        }
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_ROADSIDE_RECOVERY_ARMED vehicle=%s tow_quote=%d patch_quote=%d patch_possible=%s repair_quote=%d severity=%.3f player_choice=REQUIRED"),
            *Vehicle->GetPersistentVehicleId().ToString(), Assessment.TowEstimate, Assessment.EmergencyPatchEstimate,
            Assessment.bEmergencyPatchPossible ? TEXT("YES") : TEXT("NO"), Assessment.RepairEstimate, Assessment.Severity);
    }
}

int32 UGTTRoadsideRecoverySubsystem::CalculateRoadsideCost(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    if (const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()
        ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr)
    {
        return Decision->CalculateTowEstimate(Vehicle);
    }
    return 140;
}

int32 UGTTRoadsideRecoverySubsystem::CalculateImpoundCost(const AGTTRoadVehicleNativePawn* Vehicle, int32 WantedLevel) const
{
    if (!Vehicle) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const int32 SafetyService = FMath::RoundToInt(
        (1.0f - State.ConditionPercent) * 190.0f + (1.0f - State.TireIntegrity) * 100.0f);
    return FMath::Clamp(360 + WantedLevel * 145 + SafetyService + Vehicle->GetBodyDamageRepairSurcharge(), 505, 1200);
}

FVector UGTTRoadsideRecoverySubsystem::GetWorkshopDropLocation(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    return WorkshopBaseLocation + FVector(
        0.0f,
        Vehicle && Vehicle->GetPersistentVehicleId() == FName(TEXT("Mulebox1200")) ? 250.0f : -250.0f,
        0.0f);
}

bool UGTTRoadsideRecoverySubsystem::CompleteEmergencyPatch(
    AGTTRoadVehicleNativePawn* Vehicle,
    int32 PatchQuote,
    FName ExpectedVehicleId)
{
    if (!Vehicle || !Vehicle->GetDriverPawn() || PatchQuote <= 0 || ExpectedVehicleId.IsNone()) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Driver);
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    if (!Economy || !Decision || !Decision->CanEmergencyPatch(Vehicle)) return false;

    if (Vehicle->GetPersistentVehicleId() != ExpectedVehicleId)
    {
        Economy->PushMessage(TEXT("Emergency patch target changed before arrival. Service cancelled with no charge."), 6.0f);
        return false;
    }

    const FGTTRoadVehicleMigrationSnapshot Before = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyBefore = Vehicle->GetBodyDamageSnapshot();
    const int32 DetachedMaskBefore = Vehicle->GetDetachedPanelMask();

    if (!Economy->SpendCash(PatchQuote, TEXT("Emergency roadside limp-home patch")))
    {
        Economy->PushMessage(FString::Printf(TEXT("Locked patch quote is $%d, but cash is no longer sufficient. Dispatch ended without service."), PatchQuote), 6.0f);
        return false;
    }
    const bool bApplied = Vehicle->ApplyNativeEmergencyRoadsidePatch();

    const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyAfter = Vehicle->GetBodyDamageSnapshot();
    const bool bIdentityPreserved = Vehicle->GetPersistentVehicleId() == ExpectedVehicleId;
    const bool bBodyPreserved =
        FMath::IsNearlyEqual(BodyBefore.FrontHealth, BodyAfter.FrontHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.RearHealth, BodyAfter.RearHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.LeftHealth, BodyAfter.LeftHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.RightHealth, BodyAfter.RightHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.CoolingStress, BodyAfter.CoolingStress, 0.001f)
        && BodyBefore.DetachedPanelCount == BodyAfter.DetachedPanelCount
        && DetachedMaskBefore == Vehicle->GetDetachedPanelMask();
    const bool bLimpFloorsApplied =
        After.ConditionPercent + KINDA_SMALL_NUMBER >= FMath::Max(Before.ConditionPercent, 0.30f)
        && After.TireIntegrity + KINDA_SMALL_NUMBER >= FMath::Max(Before.TireIntegrity, 0.32f)
        && After.FuelLiters + KINDA_SMALL_NUMBER >= FMath::Max(
            Before.FuelLiters, FMath::Min(Vehicle->GetFuelCapacityLiters(), 5.0f));
    const bool bSuccess = bApplied && bIdentityPreserved && bBodyPreserved && bLimpFloorsApplied;

    if (bSuccess)
    {
        Economy->PushMessage(FString::Printf(
            TEXT("Emergency patch complete at locked quote $%d. Limp-home only — body damage remains and a full workshop repair is still recommended."),
            PatchQuote), 8.0f);
        UE_LOG(LogGTT, Display,
            TEXT("NATIVE_ROADSIDE_PATCH_COMPLETE vehicle=%s cost=%d quote_locked=YES result=PASS identity_preserved=YES body_preserved=YES workshop_repair_still_required=YES"),
            *ExpectedVehicleId.ToString(), PatchQuote);
    }
    else
    {
        Vehicle->RestorePersistentMigrationSnapshot(Before);
        Economy->AddCash(PatchQuote, TEXT("Emergency patch verification failed — charge refunded."));
        Economy->PushMessage(TEXT("Emergency patch verification failed. Vehicle state was rolled back; use workshop/tow recovery before continuing."), 8.0f);
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_PATCH_COMPLETE vehicle=%s cost=%d quote_locked=YES result=FAIL identity_preserved=%s body_preserved=%s floors_applied=%s rollback=YES refund=YES"),
            *ExpectedVehicleId.ToString(), PatchQuote, bIdentityPreserved ? TEXT("YES") : TEXT("NO"),
            bBodyPreserved ? TEXT("YES") : TEXT("NO"), bLimpFloorsApplied ? TEXT("YES") : TEXT("NO"));
    }
    return bSuccess;
}

bool UGTTRoadsideRecoverySubsystem::CompleteRecovery(
    AGTTRoadVehicleNativePawn* Vehicle,
    EGTTRoadsideRecoveryMode Mode,
    int32 LockedTowQuote,
    FName ExpectedVehicleId)
{
    if (!Vehicle || !Vehicle->GetDriverPawn()) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Driver);
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Driver);
    if (!Economy) return false;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;

    if (Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance)
    {
        if (ExpectedVehicleId.IsNone() || Vehicle->GetPersistentVehicleId() != ExpectedVehicleId || LockedTowQuote <= 0)
        {
            Economy->PushMessage(TEXT("Tow dispatch contract no longer matches this vehicle. Service cancelled without charge."), 7.0f);
            UE_LOG(LogGTT, Warning,
                TEXT("NATIVE_ROADSIDE_TOW_CONTRACT_REJECTED expected=%s actual=%s locked_quote=%d charged=NO"),
                *ExpectedVehicleId.ToString(), *Vehicle->GetPersistentVehicleId().ToString(), LockedTowQuote);
            return false;
        }
    }

    const int32 Cost = Mode == EGTTRoadsideRecoveryMode::PoliceImpound
        ? CalculateImpoundCost(Vehicle, WantedLevel)
        : LockedTowQuote;
    if (Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance
        && !Economy->SpendCash(Cost, TEXT("Roadside tow to workshop")))
    {
        Economy->PushMessage(FString::Printf(
            TEXT("Locked tow quote is $%d, but cash is no longer sufficient. Dispatch ended without moving the vehicle."), Cost), 7.0f);
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_RECOVERY_DENIED vehicle=%s cost=%d quote_locked=YES reason=INSUFFICIENT_CASH"),
            *ExpectedVehicleId.ToString(), Cost);
        return false;
    }
    if (Mode == EGTTRoadsideRecoveryMode::PoliceImpound)
    {
        Economy->ChargeFine(Cost, TEXT("Police impound + mandatory safety service"));
        if (Wanted) Wanted->ClearWanted();
    }

    const FName BeforeTowId = Vehicle->GetPersistentVehicleId();
    const FGTTRoadVehicleMigrationSnapshot BeforeTow = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyBeforeTow = Vehicle->GetBodyDamageSnapshot();
    const FVector DropLocation = GetWorkshopDropLocation(Vehicle);
    Vehicle->ExitNativeVehicle();
    Vehicle->SetActorTransform(FTransform(FRotator(0.0f, 90.0f, 0.0f), DropLocation), false, nullptr, ETeleportType::ResetPhysics);
    Driver->SetActorLocation(DropLocation + FVector(-180.0f, 0.0f, 20.0f), false, nullptr, ETeleportType::TeleportPhysics);

    if (Mode == EGTTRoadsideRecoveryMode::PoliceImpound)
    {
        const bool bServiced = Vehicle->ApplyNativeWorkshopService();
        Economy->PushMessage(FString::Printf(TEXT("Vehicle impounded and safety-serviced: $%d."), Cost), 6.0f);
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_POLICE_IMPOUND vehicle=%s wanted=%d cost=%d serviced=%s destination=WORKSHOP"),
            *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel, Cost, bServiced ? TEXT("YES") : TEXT("NO"));
        return bServiced;
    }

    const FGTTRoadVehicleMigrationSnapshot AfterTow = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyAfterTow = Vehicle->GetBodyDamageSnapshot();
    const bool bIdentityPreserved = Vehicle->GetPersistentVehicleId() == BeforeTowId
        && Vehicle->GetPersistentVehicleId() == ExpectedVehicleId;
    const bool bDamagePreserved =
        FMath::IsNearlyEqual(BeforeTow.ConditionPercent, AfterTow.ConditionPercent, 0.001f)
        && FMath::IsNearlyEqual(BeforeTow.TireIntegrity, AfterTow.TireIntegrity, 0.001f)
        && FMath::IsNearlyEqual(BodyBeforeTow.FrontHealth, BodyAfterTow.FrontHealth, 0.001f)
        && BodyBeforeTow.DetachedPanelCount == BodyAfterTow.DetachedPanelCount;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    const int32 RepairEstimate = Decision ? Decision->CalculateRepairEstimate(Vehicle) : 0;
    Economy->PushMessage(FString::Printf(
        TEXT("Tow complete at locked quote $%d. Damage preserved; workshop estimate $%d."), Cost, RepairEstimate), 7.0f);
    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_ROADSIDE_TOW_COMPLETE vehicle=%s tow_cost=%d quote_locked=YES target_pinned=YES repair_estimate=%d damage_preserved=%s identity_preserved=%s serviced=NO destination=WORKSHOP"),
        *Vehicle->GetPersistentVehicleId().ToString(), Cost, RepairEstimate,
        bDamagePreserved ? TEXT("YES") : TEXT("NO"), bIdentityPreserved ? TEXT("YES") : TEXT("NO"));
    return bDamagePreserved && bIdentityPreserved;
}
