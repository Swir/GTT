#include "Missions/GTTNightFavorDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"

AGTTNightFavorDirector::AGTTNightFavorDirector()
{
    PrimaryActorTick.bCanEverTick = false;
}

bool AGTTNightFavorDirector::NightlifeWindowOpen() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GameMode ? GameMode->GetDayNightCycle() : nullptr;
    if (!Cycle) return false;
    const float Hour = Cycle->GetTimeOfDayHours();
    return Hour >= 18.5f || Hour < 2.5f;
}

bool AGTTNightFavorDirector::TryStart(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage == EGTTNightFavorStage::Completed) return false;
    if (Stage != EGTTNightFavorStage::Idle)
    {
        PushMessage(PlayerPawn, GetObjectiveText());
        return false;
    }
    if (!NightlifeWindowOpen())
    {
        PushMessage(PlayerPawn, TEXT("THE BENT AXLE: the favor starts during nightlife hours, 18:30-02:30."));
        return false;
    }
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            PushMessage(PlayerPawn, TEXT("Lose the police first. Nobody wants patrol cars following the spare-parts run."));
            return false;
        }
    }
    Stage = EGTTNightFavorStage::CollectParts;
    PushMessage(PlayerPawn, TEXT("SIDE JOB: NIGHT SHIFT FAVOR | pick up the emergency alternator crate at the WORKSHOP."), 7.0f);
    return true;
}

bool AGTTNightFavorDirector::TryCollectParts(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTNightFavorStage::CollectParts) return false;
    Stage = EGTTNightFavorStage::ReachNeighbor;
    PushMessage(PlayerPawn, TEXT("PARTS COLLECTED | take the crate to the stranded neighbor on EAST ROAD."), 6.0f);
    return true;
}

bool AGTTNightFavorDirector::TryHelpNeighbor(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTNightFavorStage::ReachNeighbor) return false;
    Stage = EGTTNightFavorStage::ReturnToTavern;
    PushMessage(PlayerPawn, TEXT("NEIGHBOR RUNNING AGAIN | return to THE BENT AXLE for payment."), 6.0f);
    return true;
}

bool AGTTNightFavorDirector::TryFinish(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTNightFavorStage::ReturnToTavern) return false;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(CompletionReward, FString::Printf(TEXT("Night Shift Favor: +$%d"), CompletionReward));
        Economy->PushMessage(FString::Printf(TEXT("SIDE JOB COMPLETE: NIGHT SHIFT FAVOR | +$%d"), CompletionReward), 8.0f);
    }
    Stage = EGTTNightFavorStage::Completed;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    return true;
}

bool AGTTNightFavorDirector::IsActive() const
{
    return Stage != EGTTNightFavorStage::Idle && Stage != EGTTNightFavorStage::Completed;
}

FString AGTTNightFavorDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTNightFavorStage::CollectParts: return TEXT("SIDE JOB | NIGHT SHIFT FAVOR | collect alternator crate at WORKSHOP");
        case EGTTNightFavorStage::ReachNeighbor: return TEXT("SIDE JOB | NIGHT SHIFT FAVOR | help stranded neighbor on EAST ROAD");
        case EGTTNightFavorStage::ReturnToTavern: return TEXT("SIDE JOB | NIGHT SHIFT FAVOR | return to THE BENT AXLE");
        case EGTTNightFavorStage::Completed: return TEXT("SIDE JOB COMPLETE | NIGHT SHIFT FAVOR");
        default: return FString();
    }
}

void AGTTNightFavorDirector::PushMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn)) Economy->PushMessage(Message, Duration);
}
