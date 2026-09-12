#include "Missions/GTTArc3Director.h"

#include "Combat/GTTCombatComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMainStoryDirector.h"
#include "NPC/GTTCitizenPawn.h"
#include "Save/GTTArc3Save.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTRoadGraph.h"

namespace
{
const FVector FarmOfficeLocation(-2860.0f, -450.0f, 55.0f);
const FVector RedBarnLocation(-5200.0f, 1800.0f, 90.0f);
const FVector CountyDropLocation(-4700.0f, -2500.0f, 55.0f);

FString BuildRoadHint(APawn* Pawn, const FVector& Destination)
{
    if (!Pawn) return TEXT("route unavailable");
    const int32 StartNode = FGTTRoadGraph::FindClosestNode(Pawn->GetActorLocation());
    const int32 GoalNode = FGTTRoadGraph::FindClosestNode(Destination);
    const TArray<FVector> Route = FGTTRoadGraph::BuildRoute(StartNode, GoalNode);
    if (Route.Num() <= 1) return TEXT("destination nearby");
    const int32 NextNode = Route.Num() > 1 ? FGTTRoadGraph::FindClosestNode(Route[1]) : GoalNode;
    return FString::Printf(TEXT("NEXT ROAD %s | %d legs"), *FGTTRoadGraph::GetNodeLabel(NextNode), FMath::Max(0, Route.Num() - 1));
}
}

AGTTArc3Director::AGTTArc3Director()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTArc3Director::BeginPlay()
{
    Super::BeginPlay();
    LoadProgress();
}

void AGTTArc3Director::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return;

    if (Stage == EGTTArc3Stage::RedBarnFight)
    {
        HostileRefreshTimeRemaining = FMath::Max(0.0f, HostileRefreshTimeRemaining - DeltaSeconds);
        if (Hostiles.Num() == 0 || HostileRefreshTimeRemaining <= 0.0f)
        {
            SpawnOrRefreshHostiles(PlayerPawn);
            HostileRefreshTimeRemaining = 1.25f;
        }

        if (Hostiles.Num() > 0 && GetHostilesRemaining() == 0)
        {
            ClearHostiles();
            SetStage(EGTTArc3Stage::RedBarnEvidence, PlayerPawn,
                TEXT("RED BARN RECKONING: the crew is down. Search the barn ledger and grab the evidence crate."));
        }
        return;
    }

    if (Stage == EGTTArc3Stage::EscapePolice)
    {
        if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) <= 0)
        {
            bEscapeMessageShown = false;
            SetStage(EGTTArc3Stage::CountyDrop, PlayerPawn,
                TEXT("RED BARN RECKONING: police lost you. Bring the evidence to COUNTY DROP using your Mulebox van."));
        }
        else if (!bEscapeMessageShown)
        {
            bEscapeMessageShown = true;
            PushMessage(PlayerPawn, TEXT("RED BARN RECKONING: escape the police before exposing the county ledger."), 7.0f);
        }
    }
}

bool AGTTArc3Director::TryFarmContact(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;

    if (Stage == EGTTArc3Stage::Locked)
    {
        if (!AreEarlierArcsComplete())
        {
            PushMessage(PlayerPawn, TEXT("ARC 3 LOCKED: finish TIMBER GHOSTS / Main Story Arc 2 first."));
            return false;
        }
        if (HasAuthorityAttention(PlayerPawn))
        {
            PushMessage(PlayerPawn, TEXT("FARM OFFICE: clear police/ranger attention before starting the next chapter."));
            return false;
        }
        SetStage(EGTTArc3Stage::RedBarnApproach, PlayerPawn,
            TEXT("MAIN STORY ARC 3 - RED BARN RECKONING: a hostile crew has the county payoff ledger. Go to RED BARN prepared to fight."));
        return true;
    }

    if (Stage == EGTTArc3Stage::FinalFarm)
    {
        AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
        if (!GameMode || GameMode->GetOwnedVehicleCount() < 3)
        {
            PushMessage(PlayerPawn, TEXT("ARC 3 FINAL: build a working three-vehicle farm fleet before closing the county case."));
            return false;
        }
        Pay(PlayerPawn, Arc3CompletionReward, TEXT("Main Story Arc 3 completion"));
        SetStage(EGTTArc3Stage::Completed, PlayerPawn,
            TEXT("ARC 3 COMPLETE: RED BARN RECKONING. The farm broke the payoff ring and now controls a serious rural fleet."));
        return true;
    }

    if (Stage == EGTTArc3Stage::Completed)
    {
        PushMessage(PlayerPawn, TEXT("MAIN STORY: Arcs 1-3 complete. The countryside sandbox remains open."));
        return true;
    }

    PushMessage(PlayerPawn, GetObjectiveText());
    return false;
}

bool AGTTArc3Director::TryRedBarn(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;

    if (Stage == EGTTArc3Stage::RedBarnApproach)
    {
        if (HasAuthorityAttention(PlayerPawn))
        {
            PushMessage(PlayerPawn, TEXT("RED BARN: arrive without police/ranger attention or the crew will scatter."));
            return false;
        }
        if (UGTTCombatComponent* Combat = PlayerPawn->FindComponentByClass<UGTTCombatComponent>())
        {
            if (Combat->GetStoredWeaponCount() <= 0)
            {
                PushMessage(PlayerPawn, TEXT("RED BARN: this is a bad place to arrive empty-handed. Find a rural weapon first."), 6.0f);
                return false;
            }
        }
        SetStage(EGTTArc3Stage::RedBarnFight, PlayerPawn,
            TEXT("RED BARN AMBUSH: four crew enforcers are coming. Knock them all out and hold the yard."));
        SpawnOrRefreshHostiles(PlayerPawn);
        return true;
    }

    if (Stage == EGTTArc3Stage::RedBarnEvidence)
    {
        if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn)) Wanted->AddHeat(EvidenceRaidHeat);
        Stage = EGTTArc3Stage::EscapePolice;
        bEscapeMessageShown = false;
        SaveProgress();
        PushMessage(PlayerPawn, TEXT("PAYOFF LEDGER ACQUIRED: somebody called the sheriff. ESCAPE THE POLICE with the evidence."), 8.0f);
        return true;
    }

    PushMessage(PlayerPawn, GetObjectiveText());
    return false;
}

bool AGTTArc3Director::TryCountyDrop(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTArc3Stage::CountyDrop) return false;
    if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) > 0)
    {
        PushMessage(PlayerPawn, TEXT("COUNTY DROP: lose the police before bringing evidence here."));
        return false;
    }
    if (!HasUsableOwnedVan())
    {
        PushMessage(PlayerPawn, TEXT("COUNTY DROP: park your owned Mulebox 1200 here in usable condition (35%+) to carry the evidence."), 6.0f);
        return false;
    }

    Pay(PlayerPawn, EvidenceDeliveryReward, TEXT("Red Barn county evidence delivery"));
    SetStage(EGTTArc3Stage::FinalFarm, PlayerPawn,
        TEXT("RED BARN RECKONING: the county has the ledger. Return to PLAYER FARM with a three-vehicle fleet to close Arc 3."));
    return true;
}

FString AGTTArc3Director::GetObjectiveText() const
{
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    switch (Stage)
    {
        case EGTTArc3Stage::Locked:
            return TEXT("MAIN STORY ARC 3 | finish Arc 2, then visit PLAYER FARM office");
        case EGTTArc3Stage::RedBarnApproach:
            return TEXT("MAIN STORY ARC 3 | RED BARN RECKONING | reach RED BARN armed | ") + BuildRoadHint(Pawn, RedBarnLocation);
        case EGTTArc3Stage::RedBarnFight:
            return FString::Printf(TEXT("MAIN STORY ARC 3 | RED BARN FIGHT | HOSTILES %d/%d"), GetHostilesRemaining(), HostileCount);
        case EGTTArc3Stage::RedBarnEvidence:
            return TEXT("MAIN STORY ARC 3 | RED BARN | collect payoff ledger evidence");
        case EGTTArc3Stage::EscapePolice:
            return TEXT("MAIN STORY ARC 3 | PAYOFF LEDGER | ESCAPE POLICE");
        case EGTTArc3Stage::CountyDrop:
            return TEXT("MAIN STORY ARC 3 | COUNTY DROP | owned Mulebox 35%+ | ") + BuildRoadHint(Pawn, CountyDropLocation);
        case EGTTArc3Stage::FinalFarm:
            return TEXT("MAIN STORY ARC 3 | FINAL FARM MEET | own 3 vehicles | ") + BuildRoadHint(Pawn, FarmOfficeLocation);
        case EGTTArc3Stage::Completed:
            return TEXT("MAIN STORY | ARCS 1-3 COMPLETE");
        default:
            return FString();
    }
}

int32 AGTTArc3Director::GetHostilesRemaining() const
{
    int32 Remaining = 0;
    for (const TWeakObjectPtr<AGTTCitizenPawn>& Hostile : Hostiles)
    {
        if (Hostile.IsValid() && !Hostile->IsKnockedOut()) ++Remaining;
    }
    return Remaining;
}

bool AGTTArc3Director::AreEarlierArcsComplete() const
{
    AGTTMainStoryDirector* MainStory = Cast<AGTTMainStoryDirector>(UGameplayStatics::GetActorOfClass(this, AGTTMainStoryDirector::StaticClass()));
    return MainStory && MainStory->GetStage() == EGTTMainStoryStage::Completed;
}

bool AGTTArc3Director::HasAuthorityAttention(APawn* PlayerPawn) const
{
    if (!PlayerPawn) return true;
    if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) > 0) return true;
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) return GameMode->GetWildlifeAlertLevel() > 0;
    return false;
}

bool AGTTArc3Director::HasUsableOwnedVan() const
{
    if (!GetWorld()) return false;
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        const AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || !Vehicle->IsOwnedByPlayer()) continue;
        if (Vehicle->GetPersistentVehicleId() != FName(TEXT("Mulebox1200"))) continue;
        if (Vehicle->GetConditionPercent() < 0.35f) continue;
        if (FVector::DistSquared2D(Vehicle->GetActorLocation(), CountyDropLocation) > FMath::Square(950.0f)) continue;
        return true;
    }
    return false;
}

void AGTTArc3Director::SpawnOrRefreshHostiles(APawn* PlayerPawn)
{
    if (!GetWorld() || !PlayerPawn) return;

    Hostiles.RemoveAll([](const TWeakObjectPtr<AGTTCitizenPawn>& Hostile){ return !Hostile.IsValid(); });
    if (Hostiles.Num() == 0)
    {
        const FVector Offsets[] =
        {
            FVector(280, 240, 70), FVector(300, -250, 70), FVector(-260, 300, 70), FVector(-310, -260, 70)
        };
        for (int32 Index = 0; Index < FMath::Min(HostileCount, 4); ++Index)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if (AGTTCitizenPawn* Hostile = GetWorld()->SpawnActor<AGTTCitizenPawn>(RedBarnLocation + Offsets[Index], FRotator::ZeroRotator, Params))
            {
                Hostile->StartBrawlWith(PlayerPawn);
                Hostiles.Add(Hostile);
            }
        }
        return;
    }

    for (const TWeakObjectPtr<AGTTCitizenPawn>& Hostile : Hostiles)
    {
        if (Hostile.IsValid() && !Hostile->IsKnockedOut()) Hostile->StartBrawlWith(PlayerPawn);
    }
}

void AGTTArc3Director::ClearHostiles()
{
    for (const TWeakObjectPtr<AGTTCitizenPawn>& Hostile : Hostiles)
    {
        if (Hostile.IsValid()) Hostile->Destroy();
    }
    Hostiles.Reset();
}

void AGTTArc3Director::SetStage(EGTTArc3Stage NewStage, APawn* PlayerPawn, const FString& Message)
{
    Stage = NewStage;
    bEscapeMessageShown = false;
    SaveProgress();
    PushMessage(PlayerPawn, Message, 7.0f);
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
}

void AGTTArc3Director::SaveProgress()
{
    UGTTArc3Save* Save = Cast<UGTTArc3Save>(UGameplayStatics::CreateSaveGameObject(UGTTArc3Save::StaticClass()));
    if (!Save) return;
    Save->Arc3SaveVersion = 1;
    Save->Arc3Stage = static_cast<int32>(Stage);
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void AGTTArc3Director::LoadProgress()
{
    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0)) return;
    if (UGTTArc3Save* Save = Cast<UGTTArc3Save>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
    {
        Stage = static_cast<EGTTArc3Stage>(FMath::Clamp(Save->Arc3Stage, 0, static_cast<int32>(EGTTArc3Stage::Completed)));
    }
}

void AGTTArc3Director::Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Amount, FString::Printf(TEXT("%s: +$%d"), *Reason, Amount));
    }
}

void AGTTArc3Director::PushMessage(APawn* PlayerPawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(Message, Duration);
}
