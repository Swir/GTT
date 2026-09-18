#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTFarmCargoDispatchPersistenceEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTFarmVanPawn;
class AGTTMuleboxNativePawn;
class UGTTBreakdownDecisionSubsystem;
class UGTTFarmCargoAuthoritySubsystem;
class UGTTLogisticsReputationSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTRoadsideDispatchPersistenceSubsystem;
class UGTTRoadsideRecoverySubsystem;
class UGTTSaveGame;
class UGTTWantedComponent;

/**
 * 0.1.40 packaged evidence route for roadside-dispatch persistence inside Farm Cargo.
 *
 * This route runs after 0.1.38 evidence and performs real SaveGame roundtrips through the
 * production roadside sidecar. It proves exact-id + locked-quote/ETA restore for voluntary
 * tow and patch, no-charge cancellation, Wanted fail-closed invalidation, one restored patch
 * charge and continued Farm Cargo authority. It is inert without the dedicated smoke flag.
 */
UCLASS()
class GTT_API UGTTFarmCargoDispatchPersistenceEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EPersistenceEvidencePhase : uint8
    {
        Waiting,
        Prepare,
        AcceptContract,
        EnterAndPickup,
        RequestTow,
        WaitTowCheckpoint,
        ReloadTow,
        ObserveTowRestore,
        CancelRestoredTow,
        RequestWantedTow,
        WaitWantedTowCheckpoint,
        ReloadWantedTow,
        ObserveWantedReject,
        RequestPatch,
        WaitPatchCheckpoint,
        ReloadPatch,
        ObservePatchRestore,
        AwaitPatchCompletion,
        WrongVehicle,
        HillHandoff,
        FinalHandoff,
        VerifyPersistence,
        Complete
    };

    bool ResolveScenarioActors();
    APawn* ResolvePlayerPawn() const;
    AGTTFarmJobTerminal* FindTerminal(uint8 TerminalTypeValue) const;
    AGTTMuleboxNativePawn* FindNativeMulebox() const;
    AGTTFarmVanPawn* SpawnDecoyVan(const FVector& Location);
    void StageActor(AActor* Actor, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator) const;
    bool EnsureNativeDriver();
    void StageTowRecommendedState();
    bool ReadDispatchCheckpoint(uint8 ExpectedMode, int32 ExpectedQuote, float& OutEta) const;
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bAccepted = false;
    bool bPickupBound = false;
    bool bTowCheckpointSaved = false;
    bool bTowPrimarySaved = false;
    bool bTowPrimaryLoaded = false;
    bool bTowReloadRearmed = false;
    bool bTowRestored = false;
    bool bTowQuotePreserved = false;
    bool bTowEtaPreserved = false;
    bool bTowExactVehicle = false;
    bool bTowCancelledNoCharge = false;
    bool bWantedCheckpointSaved = false;
    bool bWantedPrimarySaved = false;
    bool bWantedPrimaryLoaded = false;
    bool bWantedRestoreRearmed = false;
    bool bWantedRestoreRejectedNoCharge = false;
    bool bPatchCheckpointSaved = false;
    bool bPatchPrimarySaved = false;
    bool bPatchPrimaryLoaded = false;
    bool bPatchReloadRearmed = false;
    bool bPatchRestored = false;
    bool bPatchQuotePreserved = false;
    bool bPatchEtaPreserved = false;
    bool bPatchExactVehicle = false;
    bool bPatchNoChargeBeforeArrival = false;
    bool bPatchCompleted = false;
    bool bPatchSingleCharge = false;
    bool bTimerContinued = false;
    bool bIntegrityNotImproved = false;
    bool bWrongVehicleRejected = false;
    bool bHillHandoff = false;
    bool bFinalHandoff = false;
    bool bSaveVerified = false;

    float Elapsed = 0.0f;
    float PhaseStartedAt = 0.0f;
    float TowCheckpointEta = 0.0f;
    float WantedCheckpointEta = 0.0f;
    float PatchCheckpointEta = 0.0f;
    float TimerBeforePersistence = 0.0f;
    float IntegrityBeforePersistence = 1.0f;

    int32 TowLockedQuote = 0;
    int32 WantedTowLockedQuote = 0;
    int32 PatchLockedQuote = 0;
    int32 CashBeforeTow = 0;
    int32 CashBeforeWantedTow = 0;
    int32 CashBeforePatch = 0;
    int32 PayoutDelta = 0;
    int32 CargoRunsDelta = 0;
    int32 ReputationDelta = 0;

    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    float BaselineWantedHeat = 0.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 EvidenceCargoRunsBefore = 0;
    int32 EvidenceReputationBefore = 0;

    FName LoadedVehicleId = NAME_None;
    EPersistenceEvidencePhase Phase = EPersistenceEvidencePhase::Waiting;
    FGTTRoadVehicleMigrationSnapshot BaselineMigration;
    FGTTRoadBodyDamageSnapshot BaselineBodyDamage;
    int32 BaselineDetachedMask = 0;
    FTransform BaselineNativeTransform;

    TWeakObjectPtr<AGTTFarmJobDirector> Director;
    TWeakObjectPtr<AGTTFarmJobTerminal> StartTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> PickupTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> HillTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> FinalTerminal;
    TWeakObjectPtr<APawn> PlayerPawn;
    TWeakObjectPtr<AGTTMuleboxNativePawn> NativeMulebox;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedDecoyVan;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<UGTTFarmCargoAuthoritySubsystem> Authority;
    TWeakObjectPtr<UGTTBreakdownDecisionSubsystem> Breakdown;
    TWeakObjectPtr<UGTTRoadsideRecoverySubsystem> Roadside;
    TWeakObjectPtr<UGTTRoadsideDispatchPersistenceSubsystem> DispatchPersistence;
    TWeakObjectPtr<UGTTLogisticsReputationSubsystem> Logistics;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
    TWeakObjectPtr<UGTTWantedComponent> Wanted;

    UPROPERTY()
    TObjectPtr<UGTTSaveGame> BaselineLogisticsSave;
};
