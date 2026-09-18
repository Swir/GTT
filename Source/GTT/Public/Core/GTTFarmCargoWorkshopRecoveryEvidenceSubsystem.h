#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTFarmCargoWorkshopRecoveryEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTFarmVanPawn;
class AGTTGarageSlotTerminal;
class AGTTMuleboxNativePawn;
class AGTTServiceTerminal;
class UGTTBreakdownDecisionSubsystem;
class UGTTFarmCargoAuthoritySubsystem;
class UGTTGarageFleetSubsystem;
class UGTTLogisticsReputationSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTRoadsideRecoverySubsystem;
class UGTTSaveGame;
class UGTTWantedComponent;

/**
 * 0.1.42 packaged evidence route for the 0.1.41 garage/workshop recovery contract.
 *
 * Runs only behind the dedicated smoke flag after the earlier Farm Cargo evidence windows.
 * It proves that a real paid tow preserves the exact cargo vehicle and damage, produces an
 * authoritative garage WORKSHOP HOLD, rejects cheap garage recall before movement/payment,
 * then clears that hold only through the existing paid native workshop service. The same
 * physical Mulebox must then finish the Farm Cargo route with one payout and save checkpoint.
 */
UCLASS()
class GTT_API UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EWorkshopEvidencePhase : uint8
    {
        Waiting,
        Prepare,
        AcceptContract,
        EnterAndPickup,
        RequestTow,
        AwaitTowCompletion,
        VerifyWorkshopHold,
        RejectGarageRecall,
        ApplyWorkshopService,
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
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    AGTTGarageSlotTerminal* FindGarageSlotForVehicle(FName VehicleId) const;
    AGTTFarmVanPawn* SpawnDecoyVan(const FVector& Location);
    void StageActor(AActor* Actor, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator) const;
    bool EnsureNativeDriver();
    void StageTowRecommendedState();
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bAccepted = false;
    bool bPickupBound = false;
    bool bTowRequested = false;
    bool bTowCompleted = false;
    bool bTowSingleCharge = false;
    bool bTowDamagePreserved = false;
    bool bTowIdentityPreserved = false;
    bool bWorkshopDestination = false;
    bool bWorkshopHoldDetected = false;
    bool bGarageRecallBlocked = false;
    bool bGarageRecallNoCharge = false;
    bool bGarageRecallNoMove = false;
    bool bWorkshopServiceApplied = false;
    bool bWorkshopSingleCharge = false;
    bool bWorkshopHoldCleared = false;
    bool bMechanicalRepaired = false;
    bool bRefuelled = false;
    bool bExactVehiclePreserved = false;
    bool bTimerContinued = false;
    bool bIntegrityNotImproved = false;
    bool bWrongVehicleRejected = false;
    bool bHillHandoff = false;
    bool bFinalHandoff = false;
    bool bSaveVerified = false;

    float Elapsed = 0.0f;
    float PhaseStartedAt = 0.0f;
    float TimerBeforeRecovery = 0.0f;
    float IntegrityBeforeRecovery = 1.0f;
    float TowConditionBefore = 1.0f;
    float TowTireBefore = 1.0f;

    int32 TowLockedQuote = 0;
    int32 WorkshopQuote = 0;
    int32 CashBeforeTow = 0;
    int32 CashBeforeWorkshop = 0;
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
    EWorkshopEvidencePhase Phase = EWorkshopEvidencePhase::Waiting;
    FGTTRoadVehicleMigrationSnapshot BaselineMigration;
    FGTTRoadBodyDamageSnapshot BaselineBodyDamage;
    int32 BaselineDetachedMask = 0;
    FTransform BaselineNativeTransform;

    TWeakObjectPtr<AGTTFarmJobDirector> Director;
    TWeakObjectPtr<AGTTFarmJobTerminal> StartTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> PickupTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> HillTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> FinalTerminal;
    TWeakObjectPtr<AGTTServiceTerminal> WorkshopTerminal;
    TWeakObjectPtr<APawn> PlayerPawn;
    TWeakObjectPtr<AGTTMuleboxNativePawn> NativeMulebox;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedDecoyVan;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<UGTTFarmCargoAuthoritySubsystem> Authority;
    TWeakObjectPtr<UGTTBreakdownDecisionSubsystem> Breakdown;
    TWeakObjectPtr<UGTTGarageFleetSubsystem> GarageFleet;
    TWeakObjectPtr<UGTTRoadsideRecoverySubsystem> Roadside;
    TWeakObjectPtr<UGTTLogisticsReputationSubsystem> Logistics;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
    TWeakObjectPtr<UGTTWantedComponent> Wanted;

    UPROPERTY()
    TObjectPtr<UGTTSaveGame> BaselineLogisticsSave;
};
