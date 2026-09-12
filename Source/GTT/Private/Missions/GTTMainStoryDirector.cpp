#include "Missions/GTTMainStoryDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMissionComponent.h"
#include "Save/GTTMainStorySave.h"
#include "Vehicles/GTTTractorPawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTRoadGraph.h"

namespace
{
const FVector NorthWoodLocation(7550,760,55);
const FVector VillageShopLocation(1120,-2200,55);
const FVector TavernLocation(1650,2250,55);
const FVector EastRoadLocation(4700,1650,55);
const FVector WorkshopLocation(100,2250,55);
const FVector WardenLocation(5050,700,55);
const FVector ForestCacheLocation(7050,-950,55);
const FVector HillFarmLocation(5850,2550,55);
const FVector FarmOfficeLocation(-2720,-650,55);
}

AGTTMainStoryDirector::AGTTMainStoryDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTMainStoryDirector::BeginPlay()
{
    Super::BeginPlay();
    LoadStoryProgress();
}

void AGTTMainStoryDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return;

    if (Stage == EGTTMainStoryStage::EscapePolice)
    {
        if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) <= 0)
        {
            bEscapeMessageShown = false;
            SetStage(EGTTMainStoryStage::WorkshopDelivery, PlayerPawn,
                TEXT("THE BACKROAD DEAL: police lost you. Deliver the crate to the WORKSHOP."));
        }
        else if (!bEscapeMessageShown)
        {
            bEscapeMessageShown = true;
            PushMessage(PlayerPawn, TEXT("THE BACKROAD DEAL: lose the police before approaching the workshop."), 7.0f);
        }
        return;
    }

    if (Stage == EGTTMainStoryStage::EscapeRanger)
    {
        const AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
        if (GM && GM->GetWildlifeAlertLevel() <= 0)
        {
            bEscapeMessageShown = false;
            SetStage(EGTTMainStoryStage::HillFarmEvidence, PlayerPawn,
                TEXT("TIMBER GHOSTS: ranger heat is clear. Move the recovered evidence to HILL FARM using your tractor."));
        }
        else if (!bEscapeMessageShown)
        {
            bEscapeMessageShown = true;
            PushMessage(PlayerPawn, TEXT("TIMBER GHOSTS: shake the game warden or take the citation before moving the evidence."), 7.0f);
        }
    }
}

bool AGTTMainStoryDirector::TryFarmContact(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;

    if (Stage == EGTTMainStoryStage::Locked)
    {
        if (!IsBorrowedTractorComplete())
        {
            PushMessage(PlayerPawn, TEXT("MAIN STORY LOCKED: finish BORROWED TRACTOR first."));
            return false;
        }
        if (HasAuthorityAttention(PlayerPawn))
        {
            PushMessage(PlayerPawn, TEXT("FARM OFFICE: lose police/ranger attention before talking business."));
            return false;
        }
        if (AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            if (GM->GetOwnedVehicleCount() < 1)
            {
                PushMessage(PlayerPawn, TEXT("FARM OFFICE: you need at least one registered vehicle."));
                return false;
            }
        }
        SetStage(EGTTMainStoryStage::NorthWoodPickup, PlayerPawn,
            TEXT("MAIN STORY - COUNTY LEDGER: collect the sealed farm ledger from NORTH WOOD YARD."));
        return true;
    }

    if (Stage == EGTTMainStoryStage::FinalFarmMeet)
    {
        AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
        if (!GM || GM->GetOwnedVehicleCount() < 2)
        {
            PushMessage(PlayerPawn, TEXT("FINAL FARM MEET: build the garage to at least 2 owned vehicles first."), 6.0f);
            return false;
        }
        Pay(PlayerPawn, Arc1CompletionReward, TEXT("Main story arc one completion"));
        SetStage(EGTTMainStoryStage::Arc1Completed, PlayerPawn,
            TEXT("ARC 1 COMPLETE: the farm is solvent. Return to this office when ready for the ranger problem."));
        return true;
    }

    if (Stage == EGTTMainStoryStage::Arc1Completed)
    {
        if (HasAuthorityAttention(PlayerPawn))
        {
            PushMessage(PlayerPawn, TEXT("FARM OFFICE: clear police/ranger attention before starting TIMBER GHOSTS."));
            return false;
        }
        SetStage(EGTTMainStoryStage::WardenBriefing, PlayerPawn,
            TEXT("MAIN STORY ARC 2 - TIMBER GHOSTS: meet the game warden at the WARDEN OUTPOST."));
        return true;
    }

    if (Stage == EGTTMainStoryStage::Arc2FinalFarm)
    {
        if (!HasUsableOwnedTractor(PlayerPawn))
        {
            PushMessage(PlayerPawn, TEXT("ARC 2 FINAL: bring your usable owned tractor back into service before closing the case."), 6.0f);
            return false;
        }
        Pay(PlayerPawn, Arc2CompletionReward, TEXT("Main story arc two completion"));
        SetStage(EGTTMainStoryStage::Completed, PlayerPawn,
            TEXT("MAIN STORY ARC 2 COMPLETE: the illegal timber route is broken and the farm earned county trust."));
        return true;
    }

    if (Stage == EGTTMainStoryStage::Completed)
    {
        PushMessage(PlayerPawn, TEXT("MAIN STORY: arcs 1-2 complete. More campaign chapters will follow."));
        return true;
    }

    PushMessage(PlayerPawn, GetObjectiveText());
    return false;
}

bool AGTTMainStoryDirector::TryNorthWoodPickup(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::NorthWoodPickup) return false;
    if (HasAuthorityAttention(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("COUNTY LEDGER: clear authority attention before collecting legal paperwork."));
        return false;
    }
    SetStage(EGTTMainStoryStage::ShopDelivery, PlayerPawn,
        TEXT("COUNTY LEDGER: package collected. Deliver it to the VILLAGE SHOP accountant."));
    return true;
}

bool AGTTMainStoryDirector::TryShopDelivery(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::ShopDelivery) return false;
    Pay(PlayerPawn, LedgerReward, TEXT("County Ledger chapter"));
    SetStage(EGTTMainStoryStage::TavernMeet, PlayerPawn,
        TEXT("CHAPTER COMPLETE: COUNTY LEDGER. Next lead: meet the fixer at THE BENT AXLE after 18:30."));
    return true;
}

bool AGTTMainStoryDirector::TryTavernMeet(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::TavernMeet) return false;
    if (!IsNightWindow())
    {
        PushMessage(PlayerPawn, TEXT("THE BENT AXLE: your contact only appears 18:30-02:30."));
        return false;
    }
    if (HasAuthorityAttention(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("THE BENT AXLE: lose authority attention before the contact will talk."));
        return false;
    }
    SetStage(EGTTMainStoryStage::EastRoadPickup, PlayerPawn,
        TEXT("MAIN STORY - BACKROAD DEAL: collect the unmarked crate on EAST ROAD."));
    return true;
}

bool AGTTMainStoryDirector::TryEastRoadPickup(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::EastRoadPickup) return false;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn)) Wanted->AddHeat(BackroadPickupHeat);
    Stage = EGTTMainStoryStage::EscapePolice;
    bEscapeMessageShown = false;
    PushMessage(PlayerPawn, TEXT("BACKROAD CRATE ACQUIRED: a patrol spotted the handoff. ESCAPE THE POLICE."), 8.0f);
    SaveStoryProgress();
    if (AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GM->SaveProgress();
    return true;
}

bool AGTTMainStoryDirector::TryWorkshopDelivery(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::WorkshopDelivery) return false;
    if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) > 0)
    {
        PushMessage(PlayerPawn, TEXT("WORKSHOP: police are still on you. Do not bring them here."));
        return false;
    }
    Pay(PlayerPawn, BackroadReward, TEXT("Backroad Deal chapter"));
    SetStage(EGTTMainStoryStage::FinalFarmMeet, PlayerPawn,
        TEXT("CHAPTER COMPLETE: BACKROAD DEAL. Return to PLAYER FARM with a 2-vehicle garage to close the deal."));
    return true;
}

bool AGTTMainStoryDirector::TryWardenBriefing(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::WardenBriefing) return false;
    if (HasAuthorityAttention(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("WARDEN BRIEFING: arrive clean. The warden will not discuss the case while anyone is chasing you."));
        return false;
    }
    SetStage(EGTTMainStoryStage::ForestCache, PlayerPawn,
        TEXT("TIMBER GHOSTS: inspect the illegal timber cache deep in the forest. Expect the poachers to report you."));
    return true;
}

bool AGTTMainStoryDirector::TryForestCache(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::ForestCache) return false;
    if (AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GM->ReportWildlifeCrime(PlayerPawn, ForestCacheRangerSeverity);
    Stage = EGTTMainStoryStage::EscapeRanger;
    bEscapeMessageShown = false;
    PushMessage(PlayerPawn, TEXT("EVIDENCE RECOVERED: the poachers framed you for illegal cutting. LOSE THE GAME WARDEN."), 8.0f);
    SaveStoryProgress();
    return true;
}

bool AGTTMainStoryDirector::TryHillFarmEvidence(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTMainStoryStage::HillFarmEvidence) return false;
    if (!HasUsableOwnedTractor(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("HILL FARM: evidence pallet needs your owned tractor in usable condition (40%+)."), 6.0f);
        return false;
    }
    Pay(PlayerPawn, RangerEvidenceReward, TEXT("Timber Ghosts evidence haul"));
    SetStage(EGTTMainStoryStage::Arc2FinalFarm, PlayerPawn,
        TEXT("TIMBER GHOSTS: evidence secured. Return to PLAYER FARM with your tractor to close Arc 2."));
    return true;
}

FString AGTTMainStoryDirector::GetObjectiveText() const
{
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    switch (Stage)
    {
        case EGTTMainStoryStage::Locked: return TEXT("MAIN STORY | Finish BORROWED TRACTOR, then visit PLAYER FARM office");
        case EGTTMainStoryStage::NorthWoodPickup: return TEXT("MAIN STORY | COUNTY LEDGER | NORTH WOOD YARD | ") + BuildRoadHint(Pawn, NorthWoodLocation);
        case EGTTMainStoryStage::ShopDelivery: return TEXT("MAIN STORY | COUNTY LEDGER | VILLAGE SHOP | ") + BuildRoadHint(Pawn, VillageShopLocation);
        case EGTTMainStoryStage::TavernMeet: return TEXT("MAIN STORY | BACKROAD DEAL | BENT AXLE 18:30-02:30 | ") + BuildRoadHint(Pawn, TavernLocation);
        case EGTTMainStoryStage::EastRoadPickup: return TEXT("MAIN STORY | BACKROAD DEAL | EAST ROAD | ") + BuildRoadHint(Pawn, EastRoadLocation);
        case EGTTMainStoryStage::EscapePolice: return TEXT("MAIN STORY | BACKROAD DEAL | ESCAPE POLICE");
        case EGTTMainStoryStage::WorkshopDelivery: return TEXT("MAIN STORY | BACKROAD DEAL | WORKSHOP | ") + BuildRoadHint(Pawn, WorkshopLocation);
        case EGTTMainStoryStage::FinalFarmMeet: return TEXT("MAIN STORY | ARC 1 FINAL | own 2 vehicles | ") + BuildRoadHint(Pawn, FarmOfficeLocation);
        case EGTTMainStoryStage::Arc1Completed: return TEXT("MAIN STORY | ARC 1 COMPLETE | return to PLAYER FARM to begin TIMBER GHOSTS");
        case EGTTMainStoryStage::WardenBriefing: return TEXT("MAIN STORY | TIMBER GHOSTS | WARDEN OUTPOST | ") + BuildRoadHint(Pawn, WardenLocation);
        case EGTTMainStoryStage::ForestCache: return TEXT("MAIN STORY | TIMBER GHOSTS | FOREST CACHE | ") + BuildRoadHint(Pawn, ForestCacheLocation);
        case EGTTMainStoryStage::EscapeRanger: return TEXT("MAIN STORY | TIMBER GHOSTS | CLEAR GAME WARDEN ALERT");
        case EGTTMainStoryStage::HillFarmEvidence: return TEXT("MAIN STORY | TIMBER GHOSTS | TRACTOR TO HILL FARM | ") + BuildRoadHint(Pawn, HillFarmLocation);
        case EGTTMainStoryStage::Arc2FinalFarm: return TEXT("MAIN STORY | TIMBER GHOSTS | RETURN TO PLAYER FARM | ") + BuildRoadHint(Pawn, FarmOfficeLocation);
        case EGTTMainStoryStage::Completed: return TEXT("MAIN STORY | ARCS 1-2 COMPLETE");
        default: return FString();
    }
}

void AGTTMainStoryDirector::RestoreStoryProgress(int32 SavedStage)
{
    Stage = static_cast<EGTTMainStoryStage>(FMath::Clamp(SavedStage, 0, static_cast<int32>(EGTTMainStoryStage::Completed)));
    bEscapeMessageShown = false;
}

void AGTTMainStoryDirector::SaveStoryProgress()
{
    UGTTMainStorySave* Save = Cast<UGTTMainStorySave>(UGameplayStatics::CreateSaveGameObject(UGTTMainStorySave::StaticClass()));
    if (!Save) return;
    Save->StorySaveVersion = 2;
    Save->StoryStage = static_cast<int32>(Stage);
    UGameplayStatics::SaveGameToSlot(Save, StorySaveSlotName, 0);
}

void AGTTMainStoryDirector::LoadStoryProgress()
{
    if (!UGameplayStatics::DoesSaveGameExist(StorySaveSlotName, 0)) return;
    if (UGTTMainStorySave* Save = Cast<UGTTMainStorySave>(UGameplayStatics::LoadGameFromSlot(StorySaveSlotName, 0))) RestoreStoryProgress(Save->StoryStage);
}

bool AGTTMainStoryDirector::IsBorrowedTractorComplete() const
{
    const AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const UGTTMissionComponent* Mission = GM ? GM->GetMissionComponent() : nullptr;
    return Mission && Mission->GetActiveMissionId() == FName(TEXT("BorrowedTractor")) && Mission->GetMissionState() == EGTTMissionState::Completed;
}

bool AGTTMainStoryDirector::HasAuthorityAttention(APawn* PlayerPawn) const
{
    if (UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) > 0) return true;
    if (const AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) return GM->GetWildlifeAlertLevel() > 0;
    return false;
}

bool AGTTMainStoryDirector::IsNightWindow() const
{
    const AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GM ? GM->GetDayNightCycle() : nullptr;
    if (!Cycle) return false;
    const float Hour = Cycle->GetTimeOfDayHours();
    return Hour >= 18.5f || Hour < 2.5f;
}

bool AGTTMainStoryDirector::HasUsableOwnedTractor(APawn* PlayerPawn) const
{
    if (!GetWorld()) return false;
    for (TActorIterator<AGTTTractorPawn> It(GetWorld()); It; ++It)
    {
        const AGTTTractorPawn* Tractor = *It;
        if (Tractor && Tractor->IsOwnedByPlayer() && Tractor->GetConditionPercent() >= 0.40f) return true;
    }
    return false;
}

FString AGTTMainStoryDirector::BuildRoadHint(APawn* PlayerPawn, const FVector& Destination) const
{
    if (!PlayerPawn) return TEXT("route unavailable");
    const int32 Start = FGTTRoadGraph::FindClosestNode(PlayerPawn->GetActorLocation());
    const int32 Goal = FGTTRoadGraph::FindClosestNode(Destination);
    const TArray<FVector> Route = FGTTRoadGraph::BuildRoute(Start, Goal);
    if (Route.Num() <= 1) return FString::Printf(TEXT("ROAD: %s"), *FGTTRoadGraph::GetNodeLabel(Goal));
    const int32 NextNode = FGTTRoadGraph::FindClosestNode(Route[1]);
    return FString::Printf(TEXT("NEXT ROAD: %s | %d nodes"), *FGTTRoadGraph::GetNodeLabel(NextNode), Route.Num() - 1);
}

void AGTTMainStoryDirector::SetStage(EGTTMainStoryStage NewStage, APawn* PlayerPawn, const FString& Message)
{
    Stage = NewStage;
    SaveStoryProgress();
    PushMessage(PlayerPawn, Message, 7.0f);
    if (AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GM->SaveProgress();
}

void AGTTMainStoryDirector::Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->AddCash(FMath::Max(0, Amount), FString::Printf(TEXT("%s: +$%d"), *Reason, Amount));
}

void AGTTMainStoryDirector::PushMessage(APawn* PlayerPawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(Message, Duration);
}
