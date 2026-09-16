#include "Core/GTTDamageRecoveryEvidenceSubsystem.h"

#include "Core/GTTDemoSmokeScenarioSubsystem.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Save/GTTSaveGame.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "World/GTTServiceTerminal.h"
#include "GTT.h"

namespace
{
    const FString PrimarySaveSlot(TEXT("GTT_Prototype_01"));
    constexpr float SavedStateTolerance = 0.025f;
    constexpr float RecoveryEpsilon = 0.01f;
}

void UGTTDamageRecoveryEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_DAMAGE_RECOVERY_BEGIN version=9 route=save-load-workshop"));
    }
}

AGTTRoadVehicleNativePawn* UGTTDamageRecoveryEvidenceSubsystem::FindDamagedNativeRoadVehicle() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    AGTTRoadVehicleNativePawn* Best = nullptr;
    float LowestTireIntegrity = 1.0f;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->IsNativeReady() || !Vehicle->IsLegacyTakeoverActive()) continue;
        const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
        if (!State.bOwnedByPlayer || State.TireIntegrity >= 0.995f) continue;
        if (State.TireIntegrity < LowestTireIntegrity)
        {
            LowestTireIntegrity = State.TireIntegrity;
            Best = Vehicle;
        }
    }
    return Best;
}

AGTTVehicleBase* UGTTDamageRecoveryEvidenceSubsystem::FindLegacyVehicle(FName VehicleId) const
{
    UWorld* World = GetWorld();
    if (!World || VehicleId.IsNone()) return nullptr;
    for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (IsValid(Vehicle) && Vehicle->GetPersistentVehicleId() == VehicleId) return Vehicle;
    }
    return nullptr;
}

AGTTServiceTerminal* UGTTDamageRecoveryEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It)) return *It;
    }
    return nullptr;
}

void UGTTDamageRecoveryEvidenceSubsystem::Fail(const FString& Reason)
{
    UE_LOG(LogGTT, Error, TEXT("DEMO_SCENARIO_DAMAGE_RECOVERY result=FAIL phase=%d reason=%s elapsed=%.2f"),
        static_cast<int32>(Phase), *Reason, Elapsed);
    bFinished = true;
    Phase = EEvidencePhase::Complete;
}

void UGTTDamageRecoveryEvidenceSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    UWorld* World = GetWorld();
    if (!World) return;
    if (Elapsed > 115.0f)
    {
        Fail(TEXT("evidence timeout"));
        return;
    }

    switch (Phase)
    {
        case EEvidencePhase::WaitForCoreScenario:
        {
            UGTTDemoSmokeScenarioSubsystem* CoreScenario = World->GetSubsystem<UGTTDemoSmokeScenarioSubsystem>();
            if (!CoreScenario || CoreScenario->IsTickable()) return;

            AGTTRoadVehicleNativePawn* Vehicle = FindDamagedNativeRoadVehicle();
            if (!Vehicle)
            {
                if (Elapsed > 90.0f) Fail(TEXT("core scenario finished without a damaged owned Native road vehicle"));
                return;
            }

            TargetVehicle = Vehicle;
            TargetVehicleId = Vehicle->GetPersistentVehicleId();
            const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
            DamagedTireIntegrity = State.TireIntegrity;
            DamagedCondition = State.ConditionPercent;
            DamagedWheelRisk = Vehicle->GetRuntimeWheelRisk();
            DamagedThrottleLimit = Vehicle->GetRuntimeThrottleLimit();
            DamagedSteeringLimit = Vehicle->GetRuntimeSteeringLimit();

            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_DAMAGE_PERSISTENCE vehicle=%s phase=CAPTURE tire=%.3f condition=%.3f wheel_risk=%.3f throttle_limit=%.3f steering_limit=%.3f"),
                *TargetVehicleId.ToString(), DamagedTireIntegrity, DamagedCondition, DamagedWheelRisk,
                DamagedThrottleLimit, DamagedSteeringLimit);

            Vehicle->DeactivateLegacyTakeover();
            Phase = EEvidencePhase::SaveDamagedState;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::SaveDamagedState:
        {
            if (Elapsed - PhaseStartedSeconds < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            AGTTVehicleBase* Legacy = FindLegacyVehicle(TargetVehicleId);
            AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>();
            if (!Vehicle || !Legacy || !GameMode)
            {
                Fail(TEXT("save round-trip actors unavailable"));
                return;
            }
            if (!GameMode->SaveProgress())
            {
                Fail(TEXT("SaveProgress failed for damaged vehicle"));
                return;
            }

            const UGTTSaveGame* Saved = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimarySaveSlot, 0));
            if (!Saved)
            {
                Fail(TEXT("primary SaveGame could not be read back"));
                return;
            }
            const FGTTStoredVehicleData* Stored = Saved->OwnedVehicles.FindByPredicate(
                [this](const FGTTStoredVehicleData& Data){ return Data.VehicleId == TargetVehicleId; });
            if (!Stored)
            {
                Fail(TEXT("damaged Native road vehicle missing from OwnedVehicles"));
                return;
            }
            if (!FMath::IsNearlyEqual(Stored->TireIntegrity, DamagedTireIntegrity, SavedStateTolerance) ||
                !FMath::IsNearlyEqual(Stored->ConditionPercent, DamagedCondition, SavedStateTolerance))
            {
                Fail(TEXT("serialized vehicle damage does not match Native mirror"));
                return;
            }

            ReloadedTireIntegrity = Stored->TireIntegrity;
            ReloadedCondition = Stored->ConditionPercent;

            // Deliberately overwrite the live mirror before loading. A PASS therefore proves
            // that LoadProgress restores serialized damage instead of merely observing stale state.
            Legacy->RepairTires();
            Legacy->RepairVehicle(100000.0f);
            if (Legacy->GetTireIntegrity() < 0.999f || Legacy->GetConditionPercent() < 0.999f)
            {
                Fail(TEXT("pre-load mutation did not clear live vehicle damage"));
                return;
            }
            if (!GameMode->LoadProgress())
            {
                Fail(TEXT("LoadProgress failed during damage round-trip"));
                return;
            }

            Phase = EEvidencePhase::VerifyLoadRoundTrip;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::VerifyLoadRoundTrip:
        {
            if (Elapsed - PhaseStartedSeconds < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            AGTTVehicleBase* Legacy = FindLegacyVehicle(TargetVehicleId);
            if (!Vehicle || !Legacy)
            {
                Fail(TEXT("vehicle missing after LoadProgress"));
                return;
            }
            if (!FMath::IsNearlyEqual(Legacy->GetTireIntegrity(), ReloadedTireIntegrity, SavedStateTolerance) ||
                !FMath::IsNearlyEqual(Legacy->GetConditionPercent(), ReloadedCondition, SavedStateTolerance))
            {
                Fail(TEXT("LoadProgress did not restore serialized tire/condition damage"));
                return;
            }
            if (!Vehicle->TryActivateLegacyTakeover())
            {
                Fail(TEXT("Native takeover could not resume after loading damaged state"));
                return;
            }

            const FGTTRoadVehicleMigrationSnapshot NativeState = Vehicle->GetMigrationSnapshot();
            if (!FMath::IsNearlyEqual(NativeState.TireIntegrity, ReloadedTireIntegrity, SavedStateTolerance) ||
                !FMath::IsNearlyEqual(NativeState.ConditionPercent, ReloadedCondition, SavedStateTolerance))
            {
                Fail(TEXT("Native takeover did not import reloaded damage"));
                return;
            }

            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_DAMAGE_PERSISTENCE vehicle=%s result=PASS tire_saved=%.3f tire_reloaded=%.3f condition_saved=%.3f condition_reloaded=%.3f"),
                *TargetVehicleId.ToString(), DamagedTireIntegrity, NativeState.TireIntegrity,
                DamagedCondition, NativeState.ConditionPercent);

            Phase = EEvidencePhase::SettleReloadedNative;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::SettleReloadedNative:
        {
            if (Elapsed - PhaseStartedSeconds < 1.0f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            if (!Vehicle || !Vehicle->IsLegacyTakeoverActive())
            {
                Fail(TEXT("Native vehicle did not remain active after persistence round-trip"));
                return;
            }

            DamagedWheelRisk = Vehicle->GetRuntimeWheelRisk();
            DamagedThrottleLimit = Vehicle->GetRuntimeThrottleLimit();
            DamagedSteeringLimit = Vehicle->GetRuntimeSteeringLimit();

            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Terminal || !PlayerPawn || !Economy)
            {
                Fail(TEXT("workshop/economy integration unavailable"));
                return;
            }

            // Keep the test deterministic while still exercising the real paid workshop path.
            if (Economy->GetCash() < 250)
            {
                const int32 Reserve = 250 - Economy->GetCash();
                Economy->AddCash(Reserve, TEXT("Demo workshop recovery evidence reserve"));
                UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_ACTION action=WORKSHOP_TEST_RESERVE amount=%d"), Reserve);
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
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Vehicle || !Terminal || !PlayerPawn || !Economy)
            {
                Fail(TEXT("workshop invocation actors unavailable"));
                return;
            }
            if (!Vehicle->NeedsNativeWorkshopService())
            {
                Fail(TEXT("damaged vehicle unexpectedly reports no workshop need"));
                return;
            }

            Terminal->SetServiceType(EGTTServiceType::Workshop);
            Terminal->Interact_Implementation(PlayerPawn);
            CashAfterWorkshop = Economy->GetCash();
            if (CashAfterWorkshop >= CashBeforeWorkshop)
            {
                Fail(TEXT("paid workshop route did not charge the economy"));
                return;
            }

            Phase = EEvidencePhase::VerifyWorkshop;
            PhaseStartedSeconds = Elapsed;
            return;
        }

        case EEvidencePhase::VerifyWorkshop:
        {
            if (Elapsed - PhaseStartedSeconds < 1.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = TargetVehicle.Get();
            if (!Vehicle)
            {
                Fail(TEXT("Native vehicle disappeared after workshop service"));
                return;
            }

            const FGTTRoadVehicleMigrationSnapshot Repaired = Vehicle->GetMigrationSnapshot();
            const float RepairedRisk = Vehicle->GetRuntimeWheelRisk();
            const float RepairedThrottle = Vehicle->GetRuntimeThrottleLimit();
            const float RepairedSteering = Vehicle->GetRuntimeSteeringLimit();
            const bool bStateRecovered = Repaired.TireIntegrity >= 0.999f && Repaired.ConditionPercent >= 0.999f && !Vehicle->NeedsNativeWorkshopService();
            const bool bControlNonRegressed = RepairedRisk <= DamagedWheelRisk + 0.02f &&
                RepairedThrottle + RecoveryEpsilon >= DamagedThrottleLimit &&
                RepairedSteering + RecoveryEpsilon >= DamagedSteeringLimit;
            const bool bMeasuredRecovery = Repaired.TireIntegrity > ReloadedTireIntegrity + RecoveryEpsilon &&
                (RepairedRisk + RecoveryEpsilon < DamagedWheelRisk ||
                 RepairedThrottle > DamagedThrottleLimit + RecoveryEpsilon ||
                 RepairedSteering > DamagedSteeringLimit + RecoveryEpsilon);

            if (!bStateRecovered || !bControlNonRegressed || !bMeasuredRecovery)
            {
                Fail(TEXT("workshop did not produce measurable repaired handling state"));
                return;
            }

            UE_LOG(LogGTT, Display,
                TEXT("DEMO_SCENARIO_WORKSHOP_RECOVERY vehicle=%s result=PASS cash_before=%d cash_after=%d tire_before=%.3f tire_after=%.3f wheel_risk_before=%.3f wheel_risk_after=%.3f throttle_limit_before=%.3f throttle_limit_after=%.3f steering_limit_before=%.3f steering_limit_after=%.3f"),
                *TargetVehicleId.ToString(), CashBeforeWorkshop, CashAfterWorkshop,
                ReloadedTireIntegrity, Repaired.TireIntegrity, DamagedWheelRisk, RepairedRisk,
                DamagedThrottleLimit, RepairedThrottle, DamagedSteeringLimit, RepairedSteering);
            UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_DAMAGE_RECOVERY result=PASS vehicle=%s route=spike-save-load-workshop"), *TargetVehicleId.ToString());

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
