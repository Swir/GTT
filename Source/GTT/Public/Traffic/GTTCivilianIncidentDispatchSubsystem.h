#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTCivilianIncidentDispatchSubsystem.generated.h"

class AGTTTrafficCarPawn;
class APawn;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTCivilianIncidentDispatchState : uint8
{
    None,
    Active,
    AssistanceInProgress
};

USTRUCT(BlueprintType)
struct FGTTCivilianIncidentDispatchPresentation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bVisible = false;
    UPROPERTY(BlueprintReadOnly) FName IncidentId = NAME_None;
    UPROPERTY(BlueprintReadOnly) FVector WorldLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float Severity = 0.0f;
    UPROPERTY(BlueprintReadOnly) EGTTCivilianIncidentDispatchState State = EGTTCivilianIncidentDispatchState::None;
    UPROPERTY(BlueprintReadOnly) FString StateLabel;
    UPROPERTY(BlueprintReadOnly) FString Instruction;
    UPROPERTY(BlueprintReadOnly) float DistanceMeters = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bBoundToLiveVehicle = false;
    UPROPERTY(BlueprintReadOnly) bool bWardenTrafficControl = false;
};

/**
 * Single authority for the currently surfaced civilian roadside incident.
 *
 * 0.1.54 deliberately persists only dispatch truth (stable incident id,
 * location, severity and lifecycle), never an actor pointer or helper timer.
 * A reload must rebind to a compatible disabled traffic actor near the saved
 * location before the dispatch can be serviced.
 */
UCLASS()
class GTT_API UGTTCivilianIncidentDispatchSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Dispatch")
    bool HasActiveDispatch() const { return State != EGTTCivilianIncidentDispatchState::None; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Dispatch")
    FName GetActiveIncidentId() const { return ActiveIncidentId; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Dispatch")
    FVector GetActiveIncidentLocation() const { return ActiveLocation; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Dispatch")
    float GetActiveIncidentSeverity() const { return ActiveSeverity; }

    // C++-only observation helpers. Keeping UObject pointer parameters out of
    // reflected UFUNCTION signatures avoids UHT portability risk on UE 5.8.
    bool IsTrackedVehicle(const AGTTTrafficCarPawn* Vehicle) const;
    FGTTCivilianIncidentDispatchPresentation GetPresentationSnapshot(const APawn* Viewer) const;

private:
    void ScanForIncidents();
    void BeginDispatch(AGTTTrafficCarPawn* Vehicle);
    bool TryRebindSavedIncident();
    bool IsWardenTrafficControlBlocking(const FVector& SceneLocation) const;
    void UpdateLifecycleFromVehicle();
    void ResolveDispatch(const TCHAR* Reason, bool bNotifyPlayer);
    void SaveCheckpoint() const;
    void LoadCheckpoint();
    void ClearCheckpoint() const;
    void EnsureWorldMarker();
    void DestroyWorldMarker();
    void NotifyPlayer(const FString& Message, float Duration = 5.0f) const;

    TWeakObjectPtr<AGTTTrafficCarPawn> TrackedVehicle;
    TWeakObjectPtr<UTextRenderComponent> MarkerComponent;

    EGTTCivilianIncidentDispatchState State = EGTTCivilianIncidentDispatchState::None;
    FName ActiveIncidentId = NAME_None;
    FVector ActiveLocation = FVector::ZeroVector;
    float ActiveSeverity = 0.0f;
    float ScanAccumulator = 0.0f;
    float OrphanSeconds = 0.0f;
    bool bRestoredFromCheckpoint = false;

    static constexpr float ScanIntervalSeconds = 0.50f;
    static constexpr float RebindRadiusCm = 950.0f;
    static constexpr float OrphanExpirySeconds = 180.0f;
    static constexpr float RangerAuthorityRadiusCm = 1400.0f;
    static constexpr float SupersedeSeverityDelta = 0.20f;
};
