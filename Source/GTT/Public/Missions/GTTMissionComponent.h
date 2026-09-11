#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTTMissionComponent.generated.h"

UENUM(BlueprintType)
enum class EGTTMissionState : uint8
{
    Inactive,
    Active,
    Completed,
    Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGTTMissionChanged, FName, MissionId, int32, Stage, EGTTMissionState, State);

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTMissionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTMissionComponent();

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    bool StartMission(FName MissionId);

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    void AdvanceMission();

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    void CompleteMission();

    UFUNCTION(BlueprintCallable, Category="GTT|Mission")
    void FailMission();

    UFUNCTION(BlueprintPure, Category="GTT|Mission")
    FName GetActiveMissionId() const { return ActiveMissionId; }

    UFUNCTION(BlueprintPure, Category="GTT|Mission")
    int32 GetMissionStage() const { return MissionStage; }

    UFUNCTION(BlueprintPure, Category="GTT|Mission")
    EGTTMissionState GetMissionState() const { return MissionState; }

    UPROPERTY(BlueprintAssignable, Category="GTT|Mission")
    FGTTMissionChanged OnMissionChanged;

private:
    void BroadcastState();

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Mission")
    FName ActiveMissionId = NAME_None;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Mission")
    int32 MissionStage = 0;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Mission")
    EGTTMissionState MissionState = EGTTMissionState::Inactive;
};
