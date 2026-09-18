#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmCargoRecoveryEvidenceSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTFarmVanPawn;
class UGTTFarmCargoAuthoritySubsystem;
class UGTTLogisticsReputationSubsystem;
class UGTTPlayerEconomyComponent;
class UGTTSaveGame;
class UGTTWantedComponent;
class UGTTVehicleIdentitySubsystem;

/**
 * 0.1.31 packaged evidence route for mid-contract Farm Cargo save/load recovery.
 *
 * Inert during normal gameplay. The route is enabled only by the dedicated packaged smoke
 * flags and uses the real terminals, primary SaveGame slot, FarmJobDirector, exact-vehicle
 * authority, logistics reputation and economy. It intentionally destroys/recreates the loaded
 * Mulebox between save and load to prove stable-ID recovery rather than stale pointer reuse.
 */
UCLASS()
class GTT_API UGTTFarmCargoRecoveryEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class ERecoveryEvidencePhase : uint8
    {
        Waiting,
        Prepare,
        AcceptContract,
        PickupCargo,
        SaveLoaded,
        ReloadLoaded,
        WrongVehicleAfterReload,
        HillHandoff,
        SaveRelay,
        ReloadRelay,
        FinalHandoff,
        CompletionReload,
        VerifyPersistence,
        Complete
    };

    bool ResolveScenarioActors();
    APawn* ResolvePlayerPawn() const;
    AGTTFarmJobTerminal* FindTerminal(uint8 TerminalTypeValue) const;
    AGTTFarmVanPawn* SpawnEvidenceVan(const TCHAR* Label, FName VehicleId, const FVector& Location);
    void StagePawn(AActor* Actor, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator) const;
    void DisturbCargoRuntimeState();
    void MarkFailure(const TCHAR* Reason);
    void FinishScenario(const TCHAR* Reason);
    void RestoreBaselineState();

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bBaselineCaptured = false;
    bool bIdentityUnique = false;
    bool bAccepted = false;
    bool bPickupBound = false;
    bool bLoadedCheckpointSaved = false;
    bool bLoadedReloadRestored = false;
    bool bActorRebound = false;
    bool bWrongVehicleRejectedAfterReload = false;
    bool bHillHandoff = false;
    bool bRelayCheckpointSaved = false;
    bool bRelayReloadRestored = false;
    bool bSameVehicleAfterRelay = false;
    bool bFinalHandoff = false;
    bool bCompletionReloadStable = false;
    bool bSaveVerified = false;
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
    int32 LoadedCheckpointStock = 0;
    int32 RelayCheckpointStock = 0;
    int32 PayoutDelta = 0;
    int32 CargoRunsDelta = 0;
    int32 ReputationDelta = 0;
    float LoadedCheckpointTime = 0.0f;
    float LoadedCheckpointIntegrity = 1.0f;
    float RelayCheckpointTime = 0.0f;
    float RelayCheckpointIntegrity = 1.0f;
    FName LoadedVehicleId = NAME_None;
    ERecoveryEvidencePhase Phase = ERecoveryEvidencePhase::Waiting;

    TWeakObjectPtr<AGTTFarmJobDirector> Director;
    TWeakObjectPtr<AGTTFarmJobTerminal> StartTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> PickupTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> HillTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> FinalTerminal;
    TWeakObjectPtr<APawn> PlayerPawn;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedPickupVan;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedRecoveredVan;
    TWeakObjectPtr<AGTTFarmVanPawn> SpawnedDecoyVan;
    TWeakObjectPtr<AGTTDayNightCycle> DayNight;
    TWeakObjectPtr<UGTTFarmCargoAuthoritySubsystem> Authority;
    TWeakObjectPtr<UGTTLogisticsReputationSubsystem> Logistics;
    TWeakObjectPtr<UGTTPlayerEconomyComponent> Economy;
    TWeakObjectPtr<UGTTWantedComponent> Wanted;
    TWeakObjectPtr<UGTTVehicleIdentitySubsystem> VehicleIdentity;

    UPROPERTY()
    TObjectPtr<UGTTSaveGame> BaselineLogisticsSave;
};
