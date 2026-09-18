#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTWorkshopQueueRuntimeEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTServiceTerminal;
class UGTTPlayerEconomyComponent;
class UGTTWorkshopRepairQueueSubsystem;

/**
 * 0.1.46 packaged evidence route for the persistent deferred workshop queue.
 *
 * Runs only during the deterministic Win64 evidence route after the 0.1.44 workshop-hours
 * window. It proves request-time exact-ID/locked-quote persistence on disk, no pre-charge,
 * substitute-vehicle rejection at opening, exact-vehicle single-debit execution, sidecar
 * cleanup and preservation of primary Farm Cargo authority fields.
 */
UCLASS()
class GTT_API UGTTWorkshopQueueRuntimeEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EPhase : uint8
    {
        Waiting,
        PrepareAndBook,
        VerifyCheckpointLoad,
        AwaitSubstituteRejection,
        AwaitExactExecution,
        Complete
    };

    bool ResolveActors();
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    AGTTRoadVehicleNativePawn* FindEvidenceVehicle() const;
    AGTTRoadVehicleNativePawn* FindDecoyVehicle(FName ExcludedId) const;
    void StageVehicle(AGTTRoadVehicleNativePawn* Target, const FVector& Location) const;
    void StageOrdinaryDamage();
    bool CapturePrimaryCargoBaseline();
    bool VerifyPrimaryCargoContinuity();
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bBookingAccepted = false;
    bool bNoPrecharge = false;
    bool bCheckpointLoaded = false;
    bool bLockedQuotePreserved = false;
    bool bSubstituteRejected = false;
    bool bReservationPreserved = false;
    bool bSingleDebit = false;
    bool bExactExecution = false;
    bool bSidecarCleared = false;
    bool bIdentityPreserved = false;
    bool bMechanicalRepaired = false;
    bool bRefuelled = false;
    bool bCargoBaselineCaptured = false;
    bool bCargoContinuity = false;

    float Elapsed = 0.0f;
    float PhaseStartedAt = 0.0f;
    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 CashBeforeBooking = 0;
    int32 LockedQuote = 0;
    int32 ReadyDay = 0;
    float ReadyHour = 6.5f;
    FName VehicleId = NAME_None;

    bool bBaselineCargoActive = false;
    uint8 BaselineCargoStage = 0;
    float BaselineCargoTimeRemaining = 0.0f;
    float BaselineCargoIntegrity = 1.0f;
    FName BaselineCargoVehicleId = NAME_None;

    FGTTRoadVehicleMigrationSnapshot BaselineMigration;
    FGTTRoadBodyDamageSnapshot BaselineBodyDamage;
    int32 BaselineDetachedMask = 0;
    FTransform BaselineTransform;
    FTransform DecoyBaselineTransform;

    EPhase Phase = EPhase::Waiting;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<AGTTServiceTerminal> Workshop;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> Vehicle;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> Decoy;
    TWeakObjectPtr<UGTTWorkshopRepairQueueSubsystem> Queue;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
};
