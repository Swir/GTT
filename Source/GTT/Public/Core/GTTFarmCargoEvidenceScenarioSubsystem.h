#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmCargoEvidenceScenarioSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTFarmVanPawn;
class UGTTFarmCargoAuthoritySubsystem;
class UGTTLogisticsReputationSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTSaveGame;
class UGTTWantedComponent;

/**
 * Deterministic packaged-runtime exercise for the 0.1.27/0.1.28 Farm Cargo vertical slice.
 *
 * This subsystem is inert in normal play. It only runs when the packaged smoke command line
 * explicitly contains both -GTTDemoSmokeScenario and -GTTFarmCargoRuntimeScenario. The route
 * uses the real FarmJobDirector, terminals, cargo authority, economy, logistics and save path.
 */
UCLASS()
class GTT_API UGTTFarmCargoEvidenceScenarioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EFarmCargoEvidencePhase : uint8
    {
        Waiting,
        Prepare,
        AcceptContract,
        PickupCargo,
        WrongVehicleProbe,
        HillHandoff,
        FinalHandoff,
        VerifyPersistence,
        Complete
    };

    bool ResolveScenarioActors();
    APawn* ResolvePlayerPawn() const;
    AGTTFarmJobTerminal* FindTerminal(uint8 TerminalTypeValue) const;
    AGTTFarmVanPawn* SpawnEvidenceVan(const TCHAR* Label, const FVector& Location);
    void StagePawn(AActor* Actor, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator) const;
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bAccepted = false;
    bool bPickupBound = false;
    bool bWrongVehicleRejected = false;
    bool bHillHandoff = false;
    bool bFinalHandoff = false;
    bool bSameVehicleMaintained = false;
    bool bSaveVerified = false;
    bool bBaselineCaptured = false;
    float Elapsed = 0.0f;
    int32 BaselineDay = 1;
    float BaselineHour = 8.0f;
    float BaselineWantedHeat = 0.0f;
    int32 BaselineCash = 0;
    int32 BaselineFishCount = 0;
    float BaselineFishWeightKg = 0.0f;
    int32 EvidenceCashBefore = 0;
    int32 EvidenceCargoRunsBefore = 0;
    int32 EvidenceReputationBefore = 0;
    int32 PayoutDelta = 0;
    int32 CargoRunsDelta = 0;
    int32 ReputationDelta = 0;
    FName LoadedVehicleId = NAME_None;
    EFarmCargoEvidencePhase Phase = EFarmCargoEvidencePhase::Waiting;

    TWeakObjectPtr<AGTTFarmJobDirector> Director;
    TWeakObjectPtr<AGTTFarmJobTerminal> StartTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> PickupTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> HillTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> FinalTerminal;
    TWeakObjectPtr<APawn> PlayerPawn;
    TWeakObjectPtr<APawn> LoadedVehicle;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedPickupVan;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedDecoyVan;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<UGTTFarmCargoAuthoritySubsystem> Authority;
    TWeakObjectPtr<UGTTLogisticsReputationSubsystem> Logistics;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
    TWeakObjectPtr<UGTTWantedComponent> Wanted;

    UPROPERTY()
    TObjectPtr<UGTTSaveGame> BaselineLogisticsSave;
};
