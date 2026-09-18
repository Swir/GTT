#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTFarmCargoDispatchEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTFarmVanPawn;
class AGTTMuleboxNativePawn;
class UGTTBreakdownDecisionSubsystem;
class UGTTFarmCargoAuthoritySubsystem;
class UGTTLogisticsReputationSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTRoadsideRecoverySubsystem;
class UGTTSaveGame;
class UGTTWantedComponent;

/**
 * 0.1.38 packaged evidence route for the roadside dispatch contract inside Farm Cargo.
 *
 * The scenario runs after the earlier cargo recovery evidence routes and exercises production
 * patch/tow requests, locked request-time quotes, live ETA, same-key cancellation/no-charge,
 * exact PersistentVehicleId pinning, re-requested service, wrong-vehicle rejection, payout,
 * reputation and the primary save path. It is inert unless the packaged smoke flags enable it.
 */
UCLASS()
class GTT_API UGTTFarmCargoDispatchEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EDispatchEvidencePhase : uint8
    {
        Waiting,
        Prepare,
        AcceptContract,
        EnterAndPickup,
        RequestPatchContract,
        ObservePatchContract,
        CancelPatch,
        RequestTowContract,
        ObserveTowContract,
        CancelTow,
        ReRequestPatch,
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
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bAccepted = false;
    bool bPickupBound = false;
    bool bPatchRequestLocked = false;
    bool bPatchEtaAdvanced = false;
    bool bPatchCancelledNoCharge = false;
    bool bTowRequestLocked = false;
    bool bTowEtaAdvanced = false;
    bool bTowCancelledNoCharge = false;
    bool bPatchRerequested = false;
    bool bPatchCompleted = false;
    bool bPatchChargeMatched = false;
    bool bExactVehiclePreserved = false;
    bool bTimerContinued = false;
    bool bIntegrityNotImproved = false;
    bool bWrongVehicleRejected = false;
    bool bHillHandoff = false;
    bool bFinalHandoff = false;
    bool bSaveVerified = false;

    float Elapsed = 0.0f;
    float PatchRequestedAt = 0.0f;
    float TowRequestedAt = 0.0f;
    float PatchRerequestedAt = 0.0f;
    float PatchEtaInitial = 0.0f;
    float PatchEtaObserved = 0.0f;
    float TowEtaInitial = 0.0f;
    float TowEtaObserved = 0.0f;
    float TimerBeforeDispatch = 0.0f;
    float TimerAfterPatch = 0.0f;
    float IntegrityBeforeDispatch = 1.0f;
    float IntegrityAfterPatch = 1.0f;

    int32 InitialPatchQuote = 0;
    int32 TowQuote = 0;
    int32 FinalPatchQuote = 0;
    int32 CashBeforePatchRequest = 0;
    int32 CashAfterPatchCancel = 0;
    int32 CashBeforeTowRequest = 0;
    int32 CashAfterTowCancel = 0;
    int32 CashBeforeFinalPatch = 0;
    int32 CashAfterFinalPatch = 0;
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
    EDispatchEvidencePhase Phase = EDispatchEvidencePhase::Waiting;
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
    TWeakObjectPtr<UGTTLogisticsReputationSubsystem> Logistics;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
    TWeakObjectPtr<UGTTWantedComponent> Wanted;

    UPROPERTY()
    TObjectPtr<UGTTSaveGame> BaselineLogisticsSave;
};
