#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTWorkshopPriorityPickupRuntimeEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTServiceTerminal;
class UGTTPlayerEconomyComponent;
class UGTTWorkshopRepairQueueSubsystem;

/**
 * 0.1.52 packaged evidence route for workshop priority, paid pickup and fleet return.
 *
 * Enabled only by the deterministic demo-smoke command line. The route uses the production
 * workshop queue APIs to book STANDARD work, promote the same exact vehicle to URGENT, prove
 * +20% locked-quote/no-precharge persistence, execute the x0.80 timed service, prove one debit
 * and persisted READY_FOR_PICKUP, reject a wrong exact-ID pickup, then release the same repaired
 * vehicle without a second charge. Normal gameplay is never accelerated by this subsystem.
 */
UCLASS()
class GTT_API UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem : public UTickableWorldSubsystem
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
        PreparePriority,
        AwaitCheckIn,
        AwaitCheckout,
        VerifyPickup,
        Complete
    };

    bool ResolveActors();
    bool FindEvidenceVehicles();
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    void StageVehicle(AGTTRoadVehicleNativePawn* Target, const FVector& Location) const;
    void StageDamage(AGTTRoadVehicleNativePawn* Target) const;
    float CalculateExpectedStandardDuration(const AGTTRoadVehicleNativePawn* Target) const;
    bool VerifyPriorityCheckpoint(bool bRequirePickup) const;
    bool CapturePrimaryCargoBaseline();
    bool VerifyPrimaryCargoContinuity() const;
    void MarkFailure(const TCHAR* Reason);
    void RestoreBaselineState();
    void FinishScenario(const TCHAR* Reason);

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bCargoBaselineCaptured = false;
    bool bPriorityPromoted = false;
    bool bNoPrecharge = false;
    bool bPriorityPersisted = false;
    bool bUrgentTiming = false;
    bool bSingleDebit = false;
    bool bPickupPersisted = false;
    bool bPickupHoldObserved = false;
    bool bWrongIdRejected = false;
    bool bExactPickupReleased = false;
    bool bNoSecondCharge = false;
    bool bIdentityPreserved = false;
    bool bCargoContinuity = false;

    float Elapsed = 0.0f;
    float PhaseStartedAt = 0.0f;
    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 CashBeforeBooking = 0;
    int32 StandardQuote = 0;
    int32 UrgentQuote = 0;
    int32 ReadyDay = 0;
    float ReadyHour = 0.0f;
    int32 ServiceCompleteDay = 0;
    float ServiceCompleteHour = 0.0f;
    float ExpectedStandardDuration = 0.0f;
    float ObservedUrgentDuration = 0.0f;
    FName VehicleId = NAME_None;
    FName DecoyVehicleId = NAME_None;

    bool bBaselineCargoActive = false;
    uint8 BaselineCargoStage = 0;
    float BaselineCargoTimeRemaining = 0.0f;
    float BaselineCargoIntegrity = 1.0f;
    FName BaselineCargoVehicleId = NAME_None;

    FGTTRoadVehicleMigrationSnapshot BaselineMigration;
    FGTTRoadBodyDamageSnapshot BaselineBodyDamage;
    int32 BaselineDetachedMask = 0;
    FTransform BaselineTransform;

    EPhase Phase = EPhase::Waiting;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<AGTTServiceTerminal> Workshop;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> Vehicle;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> DecoyVehicle;
    TWeakObjectPtr<UGTTWorkshopRepairQueueSubsystem> Queue;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
};
