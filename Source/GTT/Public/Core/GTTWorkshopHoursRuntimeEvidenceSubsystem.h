#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTWorkshopHoursRuntimeEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTServiceTerminal;
class UGTTGarageFleetSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTRoadsideRecoverySubsystem;

/**
 * 0.1.44 packaged evidence route for the 0.1.43 workshop-hours contract.
 *
 * Runs only during the deterministic Win64 smoke route, after the 0.1.42 Farm Cargo
 * workshop-recovery evidence window. It proves real clock boundaries, fail-closed ordinary
 * service after closing, production tow -> WORKSHOP HOLD, the exact +35% emergency quote,
 * single-charge emergency service, hold release and stable persistent vehicle identity.
 */
UCLASS()
class GTT_API UGTTWorkshopHoursRuntimeEvidenceSubsystem : public UTickableWorldSubsystem
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
        Prepare,
        Boundaries,
        ClosedOrdinaryService,
        RequestTow,
        AwaitTow,
        VerifyEmergencyQuote,
        ApplyEmergencyService,
        Complete
    };

    bool ResolveActors();
    APawn* ResolvePlayerPawn() const;
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    AGTTRoadVehicleNativePawn* FindEvidenceVehicle() const;
    bool EnsureNativeDriver();
    void StageVehicle(const FVector& Location) const;
    void StageOrdinaryDamage();
    void StageTowDamage();
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bBoundariesVerified = false;
    bool bClosedServiceRejected = false;
    bool bClosedNoCharge = false;
    bool bClosedNoMutation = false;
    bool bTowRequested = false;
    bool bTowCompleted = false;
    bool bTowSingleCharge = false;
    bool bHoldDetected = false;
    bool bEmergencyQuoteVerified = false;
    bool bEmergencySingleCharge = false;
    bool bEmergencyServiceApplied = false;
    bool bHoldCleared = false;
    bool bIdentityPreserved = false;
    bool bMechanicalRepaired = false;
    bool bRefuelled = false;

    float Elapsed = 0.0f;
    float PhaseStartedAt = 0.0f;
    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 CashBeforeTow = 0;
    int32 CashBeforeEmergency = 0;
    int32 TowLockedQuote = 0;
    int32 BaseWorkshopQuote = 0;
    int32 EmergencyWorkshopQuote = 0;
    FName VehicleId = NAME_None;

    FGTTRoadVehicleMigrationSnapshot BaselineMigration;
    FGTTRoadBodyDamageSnapshot BaselineBodyDamage;
    int32 BaselineDetachedMask = 0;
    FTransform BaselineTransform;

    EPhase Phase = EPhase::Waiting;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<AGTTServiceTerminal> Workshop;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> Vehicle;
    TWeakObjectPtr<APawn> PlayerPawn;
    TWeakObjectPtr<UGTTGarageFleetSubsystem> GarageFleet;
    TWeakObjectPtr<UGTTRoadsideRecoverySubsystem> Roadside;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
};
