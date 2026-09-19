#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTWorkshopCapacityRuntimeEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTServiceTerminal;
class UGTTPlayerEconomyComponent;
class UGTTWorkshopRepairQueueSubsystem;

/**
 * 0.1.48 packaged evidence route for multi-vehicle workshop capacity.
 *
 * Runs only in the deterministic Win64 evidence route after the existing single-vehicle
 * queue proof. It books two exact owned vehicles after hours, proves 45-minute deterministic
 * capacity spacing, exact-ID cancellation/rebooking and on-disk additive persistence, then
 * makes the earlier appointment intentionally unaffordable and proves it cannot block the
 * later affordable appointment. No booking/cancellation path is allowed to pre-charge cash.
 */
UCLASS()
class GTT_API UGTTWorkshopCapacityRuntimeEvidenceSubsystem : public UTickableWorldSubsystem
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
        PrepareBookings,
        VerifyDiskAndCancel,
        VerifyRebook,
        AwaitCapacityExecution,
        Complete
    };

    bool ResolveActors();
    bool FindEvidenceVehicles();
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    void StageVehicle(AGTTRoadVehicleNativePawn* Target, const FVector& Location) const;
    void StageDamage(AGTTRoadVehicleNativePawn* Target, bool bSevere) const;
    bool CapturePrimaryCargoBaseline();
    bool VerifyPrimaryCargoContinuity() const;
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bCargoBaselineCaptured = false;
    bool bTwoBookingsAccepted = false;
    bool bNoPrecharge = false;
    bool bCapacitySpacing = false;
    bool bDiskRoundtrip = false;
    bool bIndependentCancel = false;
    bool bRebookPreserved = false;
    bool bUnderfundedNonBlocking = false;
    bool bLaterSingleDebit = false;
    bool bEarlierReservationPreserved = false;
    bool bLaterExactService = false;
    bool bIdentityPreserved = false;
    bool bCargoContinuity = false;

    float Elapsed = 0.0f;
    float PhaseStartedAt = 0.0f;
    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 CashBeforeBookings = 0;
    int32 FirstLockedQuote = 0;
    int32 SecondLockedQuote = 0;
    int32 FirstReadyDay = 0;
    int32 SecondReadyDay = 0;
    float FirstReadyHour = 0.0f;
    float SecondReadyHour = 0.0f;
    FName FirstVehicleId = NAME_None;
    FName SecondVehicleId = NAME_None;

    bool bBaselineCargoActive = false;
    uint8 BaselineCargoStage = 0;
    float BaselineCargoTimeRemaining = 0.0f;
    float BaselineCargoIntegrity = 1.0f;
    FName BaselineCargoVehicleId = NAME_None;

    FGTTRoadVehicleMigrationSnapshot FirstBaselineMigration;
    FGTTRoadVehicleMigrationSnapshot SecondBaselineMigration;
    FGTTRoadBodyDamageSnapshot FirstBaselineBodyDamage;
    FGTTRoadBodyDamageSnapshot SecondBaselineBodyDamage;
    int32 FirstBaselineDetachedMask = 0;
    int32 SecondBaselineDetachedMask = 0;
    FTransform FirstBaselineTransform;
    FTransform SecondBaselineTransform;

    EPhase Phase = EPhase::Waiting;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<AGTTServiceTerminal> Workshop;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> FirstVehicle;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> SecondVehicle;
    TWeakObjectPtr<UGTTWorkshopRepairQueueSubsystem> Queue;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
};
