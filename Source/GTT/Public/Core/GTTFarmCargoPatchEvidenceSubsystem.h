#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTFarmCargoPatchEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTFarmVanPawn;
class AGTTMuleboxNativePawn;
class UGTTBreakdownDecisionSubsystem;
class UGTTFarmCargoAuthoritySubsystem;
class UGTTFarmCargoBreakdownRecoverySubsystem;
class UGTTLogisticsReputationSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTRoadsideRecoverySubsystem;
class UGTTSaveGame;
class UGTTWantedComponent;

/**
 * 0.1.35 packaged evidence route for Farm Cargo breakdown -> paid emergency patch -> exact-vehicle continuation.
 *
 * Inert during normal gameplay. The route is enabled only by the packaged smoke flags and drives
 * the real Farm Cargo terminals, native Mulebox damage model, breakdown decision, roadside patch,
 * production cargo recovery checkpoints, exact cargo-vehicle authority, payout/reputation and
 * primary save paths. It never awards a contract directly and restores its evidence baseline.
 */
UCLASS()
class GTT_API UGTTFarmCargoPatchEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EPatchEvidencePhase : uint8
    {
        Waiting,
        Prepare,
        AcceptContract,
        EnterAndPickup,
        DamageAndRequestPatch,
        AwaitPatch,
        WrongVehicleAfterPatch,
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
    bool bBreakdownProven = false;
    bool bPatchAvailable = false;
    bool bPatchRequested = false;
    bool bPatchCompleted = false;
    bool bIdentityPreserved = false;
    bool bTimerContinued = false;
    bool bIntegrityNotImproved = false;
    bool bBodyDamagePreserved = false;
    bool bLimpHomeFloorsApplied = false;
    bool bWrongVehicleRejected = false;
    bool bHillHandoff = false;
    bool bFinalHandoff = false;
    bool bSaveVerified = false;

    float Elapsed = 0.0f;
    float PatchRequestedAt = 0.0f;
    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    float BaselineWantedHeat = 0.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 EvidenceCargoRunsBefore = 0;
    int32 EvidenceReputationBefore = 0;
    int32 CashBeforePatch = 0;
    int32 CashAfterPatch = 0;
    int32 PatchCostDelta = 0;
    int32 PayoutDelta = 0;
    int32 CargoRunsDelta = 0;
    int32 ReputationDelta = 0;
    float TimerBeforePatch = 0.0f;
    float TimerAfterPatch = 0.0f;
    float IntegrityBeforePatch = 1.0f;
    float IntegrityAfterPatch = 1.0f;
    float TireIntegrityBeforePatch = 1.0f;
    float ConditionBeforePatch = 1.0f;
    float FuelBeforePatch = 0.0f;
    FName LoadedVehicleId = NAME_None;
    EPatchEvidencePhase Phase = EPatchEvidencePhase::Waiting;

    FGTTRoadVehicleMigrationSnapshot BaselineMigration;
    FGTTRoadBodyDamageSnapshot BaselineBodyDamage;
    FGTTRoadBodyDamageSnapshot BodyBeforePatch;
    int32 BaselineDetachedMask = 0;
    int32 DetachedMaskBeforePatch = 0;
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
    TWeakObjectPtr<UGTTFarmCargoBreakdownRecoverySubsystem> CargoRecovery;
    TWeakObjectPtr<UGTTBreakdownDecisionSubsystem> Breakdown;
    TWeakObjectPtr<UGTTRoadsideRecoverySubsystem> Roadside;
    TWeakObjectPtr<UGTTLogisticsReputationSubsystem> Logistics;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
    TWeakObjectPtr<UGTTWantedComponent> Wanted;

    UPROPERTY()
    TObjectPtr<UGTTSaveGame> BaselineLogisticsSave;
};