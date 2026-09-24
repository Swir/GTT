#include "Activities/GTTFarmCargoBreakdownRecoverySubsystem.h"

#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "Activities/GTTFarmJobDirector.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadsideRecoverySubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
constexpr float CargoRecoveryEvaluationIntervalSeconds = 0.25f;
constexpr float ImpoundCheckpointSeverity = 0.72f;
}

TStatId UGTTFarmCargoBreakdownRecoverySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoBreakdownRecoverySubsystem, STATGROUP_Tickables);
}

void UGTTFarmCargoBreakdownRecoverySubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;
    EvaluationAccumulator += DeltaTime;
    if (EvaluationAccumulator < CargoRecoveryEvaluationIntervalSeconds) return;
    EvaluationAccumulator = 0.0f;
    EvaluateCargoRecovery();
}

void UGTTFarmCargoBreakdownRecoverySubsystem::EvaluateCargoRecovery()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AGTTFarmJobDirector* Director = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(World, AGTTFarmJobDirector::StaticClass()));
    UGTTFarmCargoAuthoritySubsystem* CargoAuthority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>();
    if (!Director || !CargoAuthority || (Director->GetStage() != EGTTFarmJobStage::DeliverCargo && Director->GetStage() != EGTTFarmJobStage::DeliverFinalStop))
    {
        ResetRecoveryState();
        return;
    }

    const FName BoundId = CargoAuthority->GetBoundCargoVehicleId();
    if (BoundId.IsNone())
    {
        SetRecoveryState(EGTTFarmCargoRecoveryState::AwaitingExactVehicle, nullptr, TEXT("CARGO RECOVERY BLOCKED: this active load has no exact vehicle identity. Delivery remains locked."));
        return;
    }

    APawn* BoundPawn = CargoAuthority->GetBoundCargoVehicle();
    if (!BoundPawn)
    {
        CargoAuthority->TryRebindBoundVehicle();
        BoundPawn = CargoAuthority->GetBoundCargoVehicle();
    }

    if (!BoundPawn)
    {
        RecoveryVehicleId = BoundId;
        SetRecoveryState(EGTTFarmCargoRecoveryState::AwaitingExactVehicle, nullptr, FString::Printf(TEXT("CARGO RECOVERY: recover or recall exact vehicle %s. Another vehicle cannot inherit this load."), *BoundId.ToString()));
        UE_LOG(LogGTT, Warning, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=WAIT_EXACT_VEHICLE vehicle=%s handoff_locked=YES transfer_allowed=NO"), *BoundId.ToString());
        return;
    }

    RecoveryVehicleId = BoundId;
    AGTTRoadVehicleNativePawn* NativeVehicle = Cast<AGTTRoadVehicleNativePawn>(BoundPawn);
    if (!NativeVehicle)
    {
        SetRecoveryState(EGTTFarmCargoRecoveryState::Healthy, nullptr, FString());
        return;
    }

    APawn* Driver = NativeVehicle->GetDriverPawn();
    const UGTTBreakdownDecisionSubsystem* Breakdown = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    UGTTRoadsideRecoverySubsystem* Roadside = World->GetSubsystem<UGTTRoadsideRecoverySubsystem>();
    const FGTTBreakdownAssessment Assessment = Breakdown ? Breakdown->AssessVehicle(NativeVehicle) : FGTTBreakdownAssessment();
    const bool bPatchPending = Roadside && Roadside->IsRoadsidePatchPending(NativeVehicle);
    const bool bTowPending = Roadside && Roadside->IsRoadsideTowPending(NativeVehicle);

    int32 WantedLevel = 0;
    if (Driver)
    {
        if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Driver)) WantedLevel = Wanted->GetWantedLevel();
    }

    const bool bWasPatchPending = RecoveryState == EGTTFarmCargoRecoveryState::PatchPending;
    const bool bWasTowPending = RecoveryState == EGTTFarmCargoRecoveryState::TowPending;
    const bool bWasImpoundPending = RecoveryState == EGTTFarmCargoRecoveryState::PoliceImpoundPending;

    if (bPatchPending)
    {
        if (!bWasPatchPending)
        {
            RecoveryVehicleId = BoundId;
            bPreRecoveryCheckpointWritten = CheckpointPrimarySave(TEXT("cargo-roadside-patch-pre-service"));
            SetRecoveryState(EGTTFarmCargoRecoveryState::PatchPending, Driver, FString::Printf(TEXT("CARGO PATCH: %s stays bound to this load. Temporary roadside service does not pause the delivery timer."), *BoundId.ToString()));
            UE_LOG(LogGTT, Display, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=PATCH_CHECKPOINT vehicle=%s saved=%s timer_paused=NO transfer_allowed=NO"), *BoundId.ToString(), bPreRecoveryCheckpointWritten ? TEXT("YES") : TEXT("NO"));
        }
        return;
    }

    if (bTowPending)
    {
        if (!bWasTowPending)
        {
            RecoveryStartLocation = NativeVehicle->GetActorLocation();
            RecoveryVehicleId = BoundId;
            bPreRecoveryCheckpointWritten = CheckpointPrimarySave(TEXT("cargo-roadside-tow-pre-move"));
            SetRecoveryState(EGTTFarmCargoRecoveryState::TowPending, Driver, FString::Printf(TEXT("CARGO TOW: %s stays bound to this load. The delivery timer keeps running and damage is preserved."), *BoundId.ToString()));
            UE_LOG(LogGTT, Display, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=TOW_CHECKPOINT vehicle=%s saved=%s timer_paused=NO transfer_allowed=NO"), *BoundId.ToString(), bPreRecoveryCheckpointWritten ? TEXT("YES") : TEXT("NO"));
        }
        return;
    }

    if (WantedLevel >= 2 && Assessment.Severity >= ImpoundCheckpointSeverity)
    {
        if (!bWasImpoundPending)
        {
            RecoveryStartLocation = NativeVehicle->GetActorLocation();
            RecoveryVehicleId = BoundId;
            bPreRecoveryCheckpointWritten = CheckpointPrimarySave(TEXT("cargo-police-impound-pre-move"));
            SetRecoveryState(EGTTFarmCargoRecoveryState::PoliceImpoundPending, Driver, FString::Printf(TEXT("CARGO + POLICE: %s is still the only valid load vehicle. Impound will not transfer the cargo identity."), *BoundId.ToString()));
            UE_LOG(LogGTT, Warning, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=IMPOUND_CHECKPOINT vehicle=%s saved=%s transfer_allowed=NO"), *BoundId.ToString(), bPreRecoveryCheckpointWritten ? TEXT("YES") : TEXT("NO"));
        }
        return;
    }

    if (bWasPatchPending)
    {
        const FName ExpectedId = RecoveryVehicleId;
        const bool bIdentityPreserved = VerifyExactCargoVehicle(CargoAuthority, NativeVehicle, ExpectedId);
        if (!bIdentityPreserved)
        {
            SetRecoveryState(EGTTFarmCargoRecoveryState::AwaitingExactVehicle, Driver, FString::Printf(TEXT("CARGO PATCH HOLD: exact vehicle %s was not restored. Delivery remains blocked until it returns."), *ExpectedId.ToString()));
            UE_LOG(LogGTT, Error, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=POST_PATCH_VERIFY result=FAIL expected=%s actual=%s handoff_locked=YES transfer_allowed=NO"), *ExpectedId.ToString(), *CargoAuthority->GetBoundCargoVehicleId().ToString());
            return;
        }

        const bool bPostSave = CheckpointPrimarySave(TEXT("cargo-roadside-patch-post-service"));
        SetRecoveryState(EGTTFarmCargoRecoveryState::Patched, Driver, FString::Printf(TEXT("CARGO PATCHED: %s kept the load identity. Limp carefully; the route clock continues and workshop repair remains due."), *ExpectedId.ToString()));
        UE_LOG(LogGTT, Display, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=POST_PATCH_VERIFY result=PASS vehicle=%s identity_preserved=YES saved=%s timer_paused=NO transfer_allowed=NO"), *ExpectedId.ToString(), bPostSave ? TEXT("YES") : TEXT("NO"));
        bPreRecoveryCheckpointWritten = false;
        return;
    }

    if (bWasTowPending || bWasImpoundPending)
    {
        const FName ExpectedId = RecoveryVehicleId;
        const bool bIdentityPreserved = VerifyExactCargoVehicle(CargoAuthority, NativeVehicle, ExpectedId);
        if (!bIdentityPreserved)
        {
            SetRecoveryState(EGTTFarmCargoRecoveryState::AwaitingExactVehicle, Driver, FString::Printf(TEXT("CARGO RECOVERY HOLD: exact vehicle %s was not restored. Delivery remains blocked until it returns."), *ExpectedId.ToString()));
            UE_LOG(LogGTT, Error, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=POST_RECOVERY_VERIFY result=FAIL expected=%s actual=%s handoff_locked=YES"), *ExpectedId.ToString(), *CargoAuthority->GetBoundCargoVehicleId().ToString());
            return;
        }

        const bool bPostSave = CheckpointPrimarySave(TEXT("cargo-recovery-post-move"));
        const float MovedDistanceCm = FVector::Dist2D(RecoveryStartLocation, NativeVehicle->GetActorLocation());
        SetRecoveryState(EGTTFarmCargoRecoveryState::Recovered, Driver, FString::Printf(TEXT("CARGO RECOVERED: %s kept the load identity. Route continues; no stock, payout or reputation was duplicated."), *ExpectedId.ToString()));
        UE_LOG(LogGTT, Display, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=POST_RECOVERY_VERIFY result=PASS vehicle=%s moved_cm=%.1f identity_preserved=YES saved=%s timer_paused=NO"), *ExpectedId.ToString(), MovedDistanceCm, bPostSave ? TEXT("YES") : TEXT("NO"));
        bPreRecoveryCheckpointWritten = false;
        return;
    }

    EGTTFarmCargoRecoveryState DesiredState = EGTTFarmCargoRecoveryState::Healthy;
    FString Message;
    switch (Assessment.Recommendation)
    {
        case EGTTBreakdownRecommendation::Immobilized:
        case EGTTBreakdownRecommendation::TowRecommended:
            DesiredState = EGTTFarmCargoRecoveryState::TowRecommended;
            if (Assessment.bEmergencyPatchPossible)
                Message = FString::Printf(TEXT("CARGO BREAKDOWN: %s can use Y patch $%d for limp-home or T tow $%d. The contract clock keeps running either way."), *BoundId.ToString(), Assessment.EmergencyPatchEstimate, Assessment.TowEstimate);
            else
                Message = FString::Printf(TEXT("CARGO BREAKDOWN: tow is required for %s; structural/body damage is too severe for a roadside patch. The contract clock keeps running."), *BoundId.ToString());
            break;
        case EGTTBreakdownRecommendation::LimpToWorkshop:
            DesiredState = RecoveryState == EGTTFarmCargoRecoveryState::Patched ? EGTTFarmCargoRecoveryState::Patched : EGTTFarmCargoRecoveryState::Degraded;
            if (DesiredState == EGTTFarmCargoRecoveryState::Degraded)
                Message = FString::Printf(TEXT("CARGO VEHICLE DAMAGED: %s can limp on, but cargo integrity and the delivery clock remain at risk."), *BoundId.ToString());
            break;
        default:
            DesiredState = RecoveryState == EGTTFarmCargoRecoveryState::Patched ? EGTTFarmCargoRecoveryState::Patched : RecoveryState == EGTTFarmCargoRecoveryState::Recovered ? EGTTFarmCargoRecoveryState::Recovered : EGTTFarmCargoRecoveryState::Healthy;
            break;
    }
    SetRecoveryState(DesiredState, Driver, Message);
}

bool UGTTFarmCargoBreakdownRecoverySubsystem::VerifyExactCargoVehicle(UGTTFarmCargoAuthoritySubsystem* CargoAuthority, AGTTRoadVehicleNativePawn* ExpectedVehicle, FName ExpectedId) const
{
    if (!CargoAuthority || !ExpectedVehicle || ExpectedId.IsNone()) return false;
    AGTTRoadVehicleNativePawn* NativeVehicle = ExpectedVehicle; // Preserve the established 0.1.32 exact-ID contract vocabulary.
    const bool bRebound = CargoAuthority->TryRebindBoundVehicle();
    APawn* ReboundPawn = CargoAuthority->GetBoundCargoVehicle();
    return bRebound && ReboundPawn == NativeVehicle && CargoAuthority->GetBoundCargoVehicleId() == ExpectedId && NativeVehicle->GetPersistentVehicleId() == ExpectedId;
}

void UGTTFarmCargoBreakdownRecoverySubsystem::SetRecoveryState(EGTTFarmCargoRecoveryState NewState, APawn* Driver, const FString& PlayerMessage)
{
    if (RecoveryState == NewState) return;
    RecoveryState = NewState;
    if (Driver && !PlayerMessage.IsEmpty())
    {
        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Driver)) Economy->PushMessage(PlayerMessage, 7.0f);
    }
}

bool UGTTFarmCargoBreakdownRecoverySubsystem::CheckpointPrimarySave(const TCHAR* Reason) const
{
    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const bool bSaved = GameMode && GameMode->SaveProgress();
    if (bSaved)
    {
        UE_LOG(LogGTT, Display, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=CHECKPOINT reason=%s result=PASS vehicle=%s"), Reason ? Reason : TEXT("unknown"), *RecoveryVehicleId.ToString());
    }
    else
    {
        UE_LOG(LogGTT, Warning, TEXT("FARM_CARGO_BREAKDOWN_RECOVERY event=CHECKPOINT reason=%s result=FAIL vehicle=%s"), Reason ? Reason : TEXT("unknown"), *RecoveryVehicleId.ToString());
    }
    return bSaved;
}

void UGTTFarmCargoBreakdownRecoverySubsystem::ResetRecoveryState()
{
    RecoveryState = EGTTFarmCargoRecoveryState::None;
    RecoveryVehicleId = NAME_None;
    RecoveryStartLocation = FVector::ZeroVector;
    bPreRecoveryCheckpointWritten = false;
}

FString UGTTFarmCargoBreakdownRecoverySubsystem::GetRecoveryStateLabel() const
{
    switch (RecoveryState)
    {
        case EGTTFarmCargoRecoveryState::Healthy: return TEXT("CARGO VEHICLE OK");
        case EGTTFarmCargoRecoveryState::Degraded: return TEXT("CARGO VEHICLE DEGRADED");
        case EGTTFarmCargoRecoveryState::TowRecommended: return TEXT("CARGO RECOVERY CHOICE");
        case EGTTFarmCargoRecoveryState::PatchPending: return TEXT("CARGO PATCH INBOUND");
        case EGTTFarmCargoRecoveryState::Patched: return TEXT("CARGO VEHICLE PATCHED");
        case EGTTFarmCargoRecoveryState::TowPending: return TEXT("CARGO TOW INBOUND");
        case EGTTFarmCargoRecoveryState::PoliceImpoundPending: return TEXT("CARGO POLICE IMPOUND");
        case EGTTFarmCargoRecoveryState::AwaitingExactVehicle: return TEXT("CARGO VEHICLE RECOVERY REQUIRED");
        case EGTTFarmCargoRecoveryState::Recovered: return TEXT("CARGO VEHICLE RECOVERED");
        default: return TEXT("NO CARGO RECOVERY");
    }
}
