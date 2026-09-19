#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTCivilianIncidentResponderSubsystem.generated.h"

class AGTTRoadsideResponderVehicle;
class AGTTTrafficCarPawn;
class UGTTCivilianIncidentDispatchSubsystem;

UENUM(BlueprintType)
enum class EGTTCivilianResponderPhase : uint8
{
    None,
    EnRoute,
    OnScene
};

/**
 * Subordinate responder layer for the authoritative 0.1.54 civilian dispatch.
 *
 * It never opens/closes incidents, awards/spends cash, changes Wanted, or
 * mutates ranger state. Severe unresolved dispatches get a player-first grace
 * window, then an original physical county road-service vehicle can arrive and
 * perform a no-payout recovery handoff through AGTTTrafficCarPawn.
 */
UCLASS()
class GTT_API UGTTCivilianIncidentResponderSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    bool HasActiveResponder() const { return Phase != EGTTCivilianResponderPhase::None; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    EGTTCivilianResponderPhase GetResponderPhase() const { return Phase; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder")
    FName GetResponderIncidentId() const { return TrackedIncidentId; }

private:
    AGTTTrafficCarPawn* FindDispatchVehicle(const UGTTCivilianIncidentDispatchSubsystem* Dispatch) const;
    void RequestResponder(AGTTTrafficCarPawn* Vehicle, bool bStartAtScene);
    void CancelResponder(const TCHAR* Reason, bool bResetGrace);
    void ClearResponderSceneAuthority();
    void DestroyResponderVehicle();
    void SaveCheckpoint() const;
    void LoadCheckpoint();
    void ClearCheckpoint() const;
    void NotifyPlayer(const FString& Message, float Duration = 5.0f) const;

    TWeakObjectPtr<AGTTRoadsideResponderVehicle> ResponderVehicle;
    TWeakObjectPtr<AGTTTrafficCarPawn> AuthorityVehicle;
    FName TrackedIncidentId = NAME_None;
    EGTTCivilianResponderPhase Phase = EGTTCivilianResponderPhase::None;
    float ScanAccumulator = 0.0f;
    float CheckpointAccumulator = 0.0f;
    float PlayerGraceElapsed = 0.0f;
    float SceneHoldRemaining = 0.0f;
    float MissingDispatchSeconds = 0.0f;
    bool bRestoredCheckpoint = false;
    bool bRecoveryCompletedThisSession = false;

    static constexpr float ScanIntervalSeconds = 0.50f;
    static constexpr float CheckpointIntervalSeconds = 5.0f;
    static constexpr float SevereIncidentThreshold = 0.72f;
    static constexpr float PlayerAssistGraceSeconds = 18.0f;
    static constexpr float ResponderSceneHoldSeconds = 7.0f;
    static constexpr float ResponderSpawnDistanceCm = 1900.0f;
    static constexpr float MissingDispatchExpirySeconds = 8.0f;
};
