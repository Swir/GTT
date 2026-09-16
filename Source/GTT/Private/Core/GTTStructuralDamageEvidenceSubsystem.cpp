#include "Core/GTTStructuralDamageEvidenceSubsystem.h"

#include "Core/GTTDamageRecoveryEvidenceSubsystem.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Save/GTTSaveGame.h"
#include "World/GTTServiceTerminal.h"
#include "GTT.h"

namespace
{
    const FString PrimarySaveSlot(TEXT("GTT_Prototype_01"));
    constexpr float StateTolerance = 0.03f;

    float MinBodyHealth(const FGTTRoadBodyDamageSnapshot& Body)
    {
        return FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth));
    }

    bool MatchesStoredBody(const FGTTRoadBodyDamageSnapshot& Body, const FGTTStoredRoadStructuralDamageData& Stored)
    {
        return FMath::IsNearlyEqual(Body.FrontHealth, Stored.FrontHealth, StateTolerance) &&
            FMath::IsNearlyEqual(Body.RearHealth, Stored.RearHealth, StateTolerance) &&
            FMath::IsNearlyEqual(Body.LeftHealth, Stored.LeftHealth, StateTolerance) &&
            FMath::IsNearlyEqual(Body.RightHealth, Stored.RightHealth, StateTolerance) &&
            FMath::IsNearlyEqual(Body.CoolingStress, Stored.CoolingStress, StateTolerance);
    }
}

void UGTTStructuralDamageEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_STRUCTURAL_RECOVERY_BEGIN version=10 route=structural-save-load-workshop"));
    }
}

AGTTRoadVehicleNativePawn* UGTTStructuralDamageEvidenceSubsystem::FindActiveOwnedRoadVehicle() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->IsNativeReady() || !Vehicle->IsLegacyTakeoverActive()) continue;
        if (!Vehicle->GetMigrationSnapshot().bOwnedByPlayer) continue;
        if (Vehicle->GetPersistentVehicleId() == FName(TEXT("Rattleback82")) ||
            Vehicle->GetPersistentVehicleId() == FName(TEXT("Mulebox1200")))
        {
            return Vehicle;
        }
    }
    return nullptr;
}

AGTTServiceTerminal* UGTTStructuralDamageEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It)) return *It;
    }
    return nullptr;
}

void UGTTStructuralDamageEvidenceSubsystem::Fail(const FString& Reason)
{
    UE_LOG(LogGTT, Error, TEXT("DEMO_SCENARIO_STRUCTURAL_RECOVERY result=FAIL phase=%d reason=%s elapsed=%.2f"),
        static_cast<int32>(Phase), *Reason, Elapsed);
    bFinished = true;
    Phase = EEvidencePhase::Complete;
}

void UGTTStructuralDamageEvidenceSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    UWorld* World = GetWorld();
    if (!World) return;
    if (Elapsed > 145.0f)
    {
        Fail(TEXT("structural evidence timeout"));
        return;
    }

    switch (Phase)
    {
        case EEvidencePhase::WaitForDamageRecovery:
        {
            UGTTDamageRecoveryEvidenceSubsystem* Previous = World->GetSubsystem<UGTTDamageRecoveryEvidenceSubsystem>();
            if (!Previous || Previous->IsTickable()) return;
            TargetVehicle = FindActiveOwnedRoadVehicle();
            if (!TargetVehicle.IsValid())
            {
                if (Elapsed > 105.0f) Fail(TEXT("damage recovery completed without an active owned Native road vehicle"));
                return;
            }
            TargetVehicleId = TargetVehicle->GetPersistentVehicleId();
            Phase = EEvidencePhase::StageStructuralDamage;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::StageStructuralDamage:
        {
            if (Elapsed - PhaseStartedSeconds < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            if (!Vehicle)
            {
                Fail(TEXT("target vehicle disappeared before structural staging"));
                return;
            }

            const bool bLeftImpactA = Vehicle->ApplyScriptedImpactDamage(110.0f, EGTTRoadDamageZone::Left);
            const bool bLeftImpactB = Vehicle->ApplyScriptedImpactDamage(110.0f, EGTTRoadDamageZone::Left);
            const bool bFrontImpact = Vehicle->ApplyScriptedImpactDamage(70.0f, EGTTRoadDamageZone::Front);
            SavedBody = Vehicle->GetBodyDamageSnapshot();
            SavedPanelMask = Vehicle->GetDetachedPanelMask();
            SavedRepairSurcharge = Vehicle->GetBodyDamageRepairSurcharge();

            if (!bLeftImpactA || !bLeftImpactB || !bFrontImpact || SavedPanelMask == 0 ||
                SavedBody.LeftHealth >= 0.60f || SavedBody.FrontHealth >= 0.98f ||
                SavedBody.CoolingStress <= 0.05f || SavedRepairSurcharge <= 0)
            {
                Fail(TEXT("scripted crash did not create measurable structural damage, cooling stress and a detached panel"));
                return;
            }

            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_DAMAGE vehicle=%s result=PASS front=%.3f rear=%.3f left=%.3f right=%.3f cooling=%.3f panel_mask=%d surcharge=%d"),
                *TargetVehicleId.ToString(), SavedBody.FrontHealth, SavedBody.RearHealth,
                SavedBody.LeftHealth, SavedBody.RightHealth, SavedBody.CoolingStress,
                SavedPanelMask, SavedRepairSurcharge);
            Phase = EEvidencePhase::SaveStructuralState;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::SaveStructuralState:
        {
            if (Elapsed - PhaseStartedSeconds < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>();
            if (!Vehicle || !GameMode || !GameMode->SaveProgress())
            {
                Fail(TEXT("SaveProgress failed for structural damage state"));
                return;
            }

            const UGTTSaveGame* Saved = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimarySaveSlot, 0));
            if (!Saved || Saved->SaveVersion < 5)
            {
                Fail(TEXT("primary save did not advance to structural schema v5"));
                return;
            }
            const FGTTStoredRoadStructuralDamageData* Stored = Saved->RoadStructuralDamage.FindByPredicate(
                [this](const FGTTStoredRoadStructuralDamageData& Data){ return Data.VehicleId == TargetVehicleId; });
            if (!Stored || !MatchesStoredBody(SavedBody, *Stored) || Stored->DetachedPanelMask != SavedPanelMask)
            {
                Fail(TEXT("primary save did not serialize exact structural zones/cooling/panel mask"));
                return;
            }

            FGTTRoadBodyDamageSnapshot Pristine;
            Vehicle->RestorePersistentBodyDamage(Pristine, 0);
            if (Vehicle->GetDetachedPanelMask() != 0 || Vehicle->GetBodyDamageRepairSurcharge() != 0)
            {
                Fail(TEXT("anti-stale structural mutation did not clear live body state"));
                return;
            }
            if (!GameMode->LoadProgress())
            {
                Fail(TEXT("LoadProgress failed for structural round-trip"));
                return;
            }

            Phase = EEvidencePhase::VerifyStructuralReload;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::VerifyStructuralReload:
        {
            if (Elapsed - PhaseStartedSeconds < 0.75f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            if (!Vehicle || !Vehicle->IsLegacyTakeoverActive())
            {
                Fail(TEXT("Native takeover did not resume after structural LoadProgress"));
                return;
            }

            const FGTTRoadBodyDamageSnapshot Reloaded = Vehicle->GetBodyDamageSnapshot();
            const int32 ReloadedMask = Vehicle->GetDetachedPanelMask();
            if (!FMath::IsNearlyEqual(SavedBody.FrontHealth, Reloaded.FrontHealth, StateTolerance) ||
                !FMath::IsNearlyEqual(SavedBody.RearHealth, Reloaded.RearHealth, StateTolerance) ||
                !FMath::IsNearlyEqual(SavedBody.LeftHealth, Reloaded.LeftHealth, StateTolerance) ||
                !FMath::IsNearlyEqual(SavedBody.RightHealth, Reloaded.RightHealth, StateTolerance) ||
                !FMath::IsNearlyEqual(SavedBody.CoolingStress, Reloaded.CoolingStress, StateTolerance) ||
                ReloadedMask != SavedPanelMask || Vehicle->GetBodyDamageRepairSurcharge() <= 0)
            {
                Fail(TEXT("exact structural body state did not survive SaveProgress/LoadProgress"));
                return;
            }

            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_PERSISTENCE vehicle=%s result=PASS front_saved=%.3f front_reloaded=%.3f left_saved=%.3f left_reloaded=%.3f cooling_saved=%.3f cooling_reloaded=%.3f panel_mask_saved=%d panel_mask_reloaded=%d surcharge=%d"),
                *TargetVehicleId.ToString(), SavedBody.FrontHealth, Reloaded.FrontHealth,
                SavedBody.LeftHealth, Reloaded.LeftHealth, SavedBody.CoolingStress, Reloaded.CoolingStress,
                SavedPanelMask, ReloadedMask, Vehicle->GetBodyDamageRepairSurcharge());

            Phase = EEvidencePhase::PrepareWorkshop;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::PrepareWorkshop:
        {
            if (Elapsed - PhaseStartedSeconds < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Vehicle || !Terminal || !PlayerPawn || !Economy)
            {
                Fail(TEXT("structural workshop integration unavailable"));
                return;
            }

            if (Economy->GetCash() < 450)
            {
                const int32 Reserve = 450 - Economy->GetCash();
                Economy->AddCash(Reserve, TEXT("Demo structural workshop evidence reserve"));
                UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_ACTION action=STRUCTURAL_WORKSHOP_TEST_RESERVE amount=%d"), Reserve);
            }

            Terminal->SetServiceType(EGTTServiceType::Workshop);
            Vehicle->SetActorLocation(Terminal->GetActorLocation() + Terminal->GetActorForwardVector() * 260.0f + FVector(0.0f, 0.0f, 85.0f),
                false, nullptr, ETeleportType::TeleportPhysics);
            CashBeforeWorkshop = Economy->GetCash();
            Phase = EEvidencePhase::InvokeWorkshop;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::InvokeWorkshop:
        {
            if (Elapsed - PhaseStartedSeconds < 0.5f) return;
            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            if (!Terminal || !PlayerPawn)
            {
                Fail(TEXT("workshop terminal/player missing during structural repair"));
                return;
            }
            Terminal->SetServiceType(EGTTServiceType::Workshop);
            Terminal->Interact_Implementation(PlayerPawn);
            Phase = EEvidencePhase::VerifyWorkshop;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::VerifyWorkshop:
        {
            if (Elapsed - PhaseStartedSeconds < 1.0f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>();
            if (!Vehicle || !Economy || !GameMode)
            {
                Fail(TEXT("structural repair verification actors unavailable"));
                return;
            }

            const FGTTRoadBodyDamageSnapshot Repaired = Vehicle->GetBodyDamageSnapshot();
            const int32 RepairedMask = Vehicle->GetDetachedPanelMask();
            const int32 CashAfter = Economy->GetCash();
            const int32 Paid = CashBeforeWorkshop - CashAfter;
            const bool bBodyRestored = MinBodyHealth(Repaired) >= 0.999f && Repaired.CoolingStress <= 0.01f &&
                RepairedMask == 0 && Vehicle->GetBodyDamageRepairSurcharge() == 0;
            const bool bSurchargeCharged = SavedRepairSurcharge > 0 && Paid > SavedRepairSurcharge;
            if (!bBodyRestored || !bSurchargeCharged)
            {
                Fail(TEXT("paid workshop did not restore structural zones/panels or charge the structural surcharge"));
                return;
            }

            if (!GameMode->SaveProgress())
            {
                Fail(TEXT("post-repair structural save failed"));
                return;
            }
            const UGTTSaveGame* Saved = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimarySaveSlot, 0));
            const FGTTStoredRoadStructuralDamageData* Stored = Saved ? Saved->RoadStructuralDamage.FindByPredicate(
                [this](const FGTTStoredRoadStructuralDamageData& Data){ return Data.VehicleId == TargetVehicleId; }) : nullptr;
            if (!Stored || Stored->DetachedPanelMask != 0 || Stored->CoolingStress > 0.01f ||
                FMath::Min(FMath::Min(Stored->FrontHealth, Stored->RearHealth), FMath::Min(Stored->LeftHealth, Stored->RightHealth)) < 0.999f)
            {
                Fail(TEXT("repaired structural state was not persisted as pristine"));
                return;
            }

            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_REPAIR vehicle=%s result=PASS cash_before=%d cash_after=%d paid=%d surcharge_before=%d panel_mask_before=%d panel_mask_after=%d body_min_after=%.3f cooling_after=%.3f"),
                *TargetVehicleId.ToString(), CashBeforeWorkshop, CashAfter, Paid, SavedRepairSurcharge,
                SavedPanelMask, RepairedMask, MinBodyHealth(Repaired), Repaired.CoolingStress);
            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_RECOVERY result=PASS vehicle=%s route=structural-save-load-workshop"),
                *TargetVehicleId.ToString());

            Phase = EEvidencePhase::Complete;
            bFinished = true;
            return;
        }

        case EEvidencePhase::Complete:
        default:
            bFinished = true;
            return;
    }
}
