#include "Missions/GTTArc4Director.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTArc3Director.h"
#include "NPC/GTTRuralFactionDirector.h"
#include "Save/GTTArc4Save.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTRoadGraph.h"
#include "World/GTTRuralEconomySubsystem.h"

namespace
{
const FVector FarmOfficeLocation(-2860.0f, -450.0f, 55.0f);
const FVector NorthPassLocation(10150.0f, 2550.0f, 100.0f);
const FVector RidgeExchangeLocation(11200.0f, 800.0f, 100.0f);

FString BuildRoadHint(APawn* Pawn, const FVector& Destination)
{
    if (!Pawn) return TEXT("route unavailable");
    const int32 StartNode = FGTTRoadGraph::FindClosestNode(Pawn->GetActorLocation());
    const int32 GoalNode = FGTTRoadGraph::FindClosestNode(Destination);
    const TArray<FVector> Route = FGTTRoadGraph::BuildRoute(StartNode, GoalNode);
    if (Route.Num() <= 1) return TEXT("destination nearby");
    const int32 NextNode = FGTTRoadGraph::FindClosestNode(Route[1]);
    return FString::Printf(TEXT("NEXT ROAD %s | %d legs"), *FGTTRoadGraph::GetNodeLabel(NextNode), FMath::Max(0, Route.Num() - 1));
}
}

AGTTArc4Director::AGTTArc4Director()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTArc4Director::BeginPlay()
{
    Super::BeginPlay();
    LoadProgress();
}

void AGTTArc4Director::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return;
    EvaluateProgress(PlayerPawn);
}

bool AGTTArc4Director::TryFarmContact(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;
    if (Stage == EGTTArc4Stage::Locked)
    {
        if (!IsArc3Complete())
        {
            PushMessage(PlayerPawn, TEXT("ARC 4 LOCKED: finish RED BARN RECKONING first."));
            return false;
        }
        if (HasAuthorityAttention(PlayerPawn))
        {
            PushMessage(PlayerPawn, TEXT("FARM OFFICE: clear police/ranger attention before taking the North Pass job."));
            return false;
        }
        StartingFactionVictories = GetFactionVictories();
        SetStage(EGTTArc4Stage::ProveGround, PlayerPawn,
            TEXT("MAIN STORY ARC 4 - NORTH PASS RUN: win one hostile-territory encounter to prove the route is open."));
        return true;
    }
    if (Stage == EGTTArc4Stage::FinalFarm)
    {
        Pay(PlayerPawn, Arc4CompletionReward, TEXT("Main Story Arc 4 completion"));
        SetStage(EGTTArc4Stage::Completed, PlayerPawn,
            TEXT("ARC 4 COMPLETE: NORTH PASS RUN. The farm now controls a route linking the village, factions, fence and ridge country."));
        return true;
    }
    PushMessage(PlayerPawn, GetObjectiveText());
    return Stage == EGTTArc4Stage::Completed;
}

bool AGTTArc4Director::TryNorthPass(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTArc4Stage::NorthPass) return false;
    UGTTRuralEconomySubsystem* Rural = GetWorld()->GetSubsystem<UGTTRuralEconomySubsystem>();
    if (!Rural || Rural->GetContrabandUnits() > 0)
    {
        PushMessage(PlayerPawn, TEXT("NORTH PASS: the fence run is not clean yet. Sell the prepared stash first."));
        return false;
    }
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn)) Wanted->AddHeat(NorthPassHeat);
    Stage = EGTTArc4Stage::EscapePolice;
    SaveProgress();
    bEscapeMessageShown = false;
    PushMessage(PlayerPawn, TEXT("NORTH PASS CHECKPOINT BLOWN: deputies are moving in. Escape the police and reach RIDGE EXCHANGE."), 8.0f);
    return true;
}

bool AGTTArc4Director::TryRidgeExchange(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTArc4Stage::RidgeExchange) return false;
    if (HasAuthorityAttention(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("RIDGE EXCHANGE: arrive clean. Police or ranger attention will burn the deal."));
        return false;
    }
    Pay(PlayerPawn, NorthPassReward, TEXT("North Pass ridge exchange"));
    SetStage(EGTTArc4Stage::FinalFarm, PlayerPawn,
        TEXT("NORTH PASS RUN: exchange complete. Return to PLAYER FARM to close Arc 4."));
    return true;
}

void AGTTArc4Director::EvaluateProgress(APawn* PlayerPawn)
{
    UGTTRuralEconomySubsystem* Rural = GetWorld()->GetSubsystem<UGTTRuralEconomySubsystem>();
    if (Stage == EGTTArc4Stage::ProveGround && GetFactionVictories() > StartingFactionVictories)
    {
        SetStage(EGTTArc4Stage::PrepareContraband, PlayerPawn,
            TEXT("NORTH PASS RUN: territory cleared. Build a contraband stash of at least 2 units through the existing poaching loop."));
    }
    else if (Stage == EGTTArc4Stage::PrepareContraband && Rural && Rural->GetContrabandUnits() >= 2)
    {
        bContrabandPrepared = true;
        SetStage(EGTTArc4Stage::FenceRun, PlayerPawn,
            TEXT("NORTH PASS RUN: stash prepared. Clear authority attention and sell it through BACKLOT FENCE."));
    }
    else if (Stage == EGTTArc4Stage::FenceRun && Rural && bContrabandPrepared && Rural->GetContrabandUnits() == 0)
    {
        SetStage(EGTTArc4Stage::NorthPass, PlayerPawn,
            TEXT("NORTH PASS RUN: fence is clean. Drive the new NORTH PASS road and hit the checkpoint."));
    }
    else if (Stage == EGTTArc4Stage::EscapePolice)
    {
        if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) <= 0)
        {
            bEscapeMessageShown = false;
            SetStage(EGTTArc4Stage::RidgeExchange, PlayerPawn,
                TEXT("NORTH PASS RUN: deputies lost you. Reach RIDGE EXCHANGE clean."));
        }
        else if (!bEscapeMessageShown)
        {
            bEscapeMessageShown = true;
            PushMessage(PlayerPawn, TEXT("NORTH PASS RUN: keep moving until the wanted level clears."));
        }
    }
}

FString AGTTArc4Director::GetObjectiveText() const
{
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    UGTTRuralEconomySubsystem* Rural = GetWorld() ? GetWorld()->GetSubsystem<UGTTRuralEconomySubsystem>() : nullptr;
    switch (Stage)
    {
        case EGTTArc4Stage::Locked: return TEXT("MAIN STORY ARC 4 | finish Arc 3, then visit PLAYER FARM office");
        case EGTTArc4Stage::ProveGround: return FString::Printf(TEXT("ARC 4 | PROVE GROUND | faction wins %d -> need +1"), GetFactionVictories());
        case EGTTArc4Stage::PrepareContraband: return FString::Printf(TEXT("ARC 4 | PREPARE STASH | contraband %d/2"), Rural ? Rural->GetContrabandUnits() : 0);
        case EGTTArc4Stage::FenceRun: return TEXT("ARC 4 | BACKLOT FENCE | sell the prepared stash clean");
        case EGTTArc4Stage::NorthPass: return TEXT("ARC 4 | NORTH PASS CHECKPOINT | ") + BuildRoadHint(Pawn, NorthPassLocation);
        case EGTTArc4Stage::EscapePolice: return TEXT("ARC 4 | NORTH PASS | ESCAPE POLICE");
        case EGTTArc4Stage::RidgeExchange: return TEXT("ARC 4 | RIDGE EXCHANGE | arrive clean | ") + BuildRoadHint(Pawn, RidgeExchangeLocation);
        case EGTTArc4Stage::FinalFarm: return TEXT("ARC 4 | RETURN TO PLAYER FARM | ") + BuildRoadHint(Pawn, FarmOfficeLocation);
        case EGTTArc4Stage::Completed: return TEXT("MAIN STORY | ARCS 1-4 COMPLETE");
        default: return FString();
    }
}

bool AGTTArc4Director::IsArc3Complete() const
{
    AGTTArc3Director* Arc3 = Cast<AGTTArc3Director>(UGameplayStatics::GetActorOfClass(this, AGTTArc3Director::StaticClass()));
    return Arc3 && Arc3->GetStage() == EGTTArc3Stage::Completed;
}

int32 AGTTArc4Director::GetFactionVictories() const
{
    AGTTRuralFactionDirector* Factions = Cast<AGTTRuralFactionDirector>(UGameplayStatics::GetActorOfClass(this, AGTTRuralFactionDirector::StaticClass()));
    return Factions ? Factions->GetFactionVictories() : 0;
}

bool AGTTArc4Director::HasAuthorityAttention(APawn* PlayerPawn) const
{
    if (!PlayerPawn || UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) > 0) return true;
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) return GameMode->GetWildlifeAlertLevel() > 0;
    return false;
}

void AGTTArc4Director::SetStage(EGTTArc4Stage NewStage, APawn* PlayerPawn, const FString& Message)
{
    Stage = NewStage;
    SaveProgress();
    PushMessage(PlayerPawn, Message, 7.0f);
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
}

void AGTTArc4Director::SaveProgress()
{
    UGTTArc4Save* Save = Cast<UGTTArc4Save>(UGameplayStatics::CreateSaveGameObject(UGTTArc4Save::StaticClass()));
    if (!Save) return;
    Save->Arc4SaveVersion = 1;
    Save->Arc4Stage = static_cast<int32>(Stage);
    Save->StartingFactionVictories = StartingFactionVictories;
    Save->bContrabandPrepared = bContrabandPrepared;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void AGTTArc4Director::LoadProgress()
{
    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0)) return;
    if (UGTTArc4Save* Save = Cast<UGTTArc4Save>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
    {
        Stage = static_cast<EGTTArc4Stage>(FMath::Clamp(Save->Arc4Stage, 0, static_cast<int32>(EGTTArc4Stage::Completed)));
        StartingFactionVictories = Save->StartingFactionVictories;
        bContrabandPrepared = Save->bContrabandPrepared;
    }
}

void AGTTArc4Director::Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->AddCash(Amount, FString::Printf(TEXT("%s: +$%d"), *Reason, Amount));
}

void AGTTArc4Director::PushMessage(APawn* PlayerPawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(Message, Duration);
}
