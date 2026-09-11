#include "Missions/GTTMissionComponent.h"

UGTTMissionComponent::UGTTMissionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UGTTMissionComponent::StartMission(FName MissionId)
{
    if (MissionId.IsNone() || MissionState == EGTTMissionState::Active)
    {
        return false;
    }

    ActiveMissionId = MissionId;
    MissionStage = 0;
    MissionState = EGTTMissionState::Active;
    BroadcastState();
    return true;
}

void UGTTMissionComponent::AdvanceMission()
{
    if (MissionState == EGTTMissionState::Active)
    {
        ++MissionStage;
        BroadcastState();
    }
}

void UGTTMissionComponent::CompleteMission()
{
    if (MissionState == EGTTMissionState::Active)
    {
        MissionState = EGTTMissionState::Completed;
        BroadcastState();
    }
}

void UGTTMissionComponent::FailMission()
{
    if (MissionState == EGTTMissionState::Active)
    {
        MissionState = EGTTMissionState::Failed;
        BroadcastState();
    }
}

void UGTTMissionComponent::BroadcastState()
{
    OnMissionChanged.Broadcast(ActiveMissionId, MissionStage, MissionState);
}
