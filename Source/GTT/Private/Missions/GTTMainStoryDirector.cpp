#include "Missions/GTTMainStoryDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMissionComponent.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"

AGTTMainStoryDirector::AGTTMainStoryDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTMainStoryDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage != EGTTMainStoryStage::EscapePolice) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return;

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
        Pay(PlayerPawn, ArcCompletionReward, TEXT("Main story arc completion"));
        SetStage(EGTTMainStoryStage::Completed, PlayerPawn,
            TEXT("MAIN STORY ARC COMPLETE: the farm is solvent, your garage is growing, and the village knows your name."));
        return true;
    }

    if (Stage == EGTTMainStoryStage::Completed)
    {
        PushMessage(PlayerPawn, TEXT("MAIN STORY: first story arc complete. More chapters will follow."));
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
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        Wanted->AddHeat(BackroadPickupHeat);
    }
    Stage = EGTTMainStoryStage::EscapePolice;
    bEscapeMessageShown = false;
    PushMessage(PlayerPawn, TEXT("BACKROAD CRATE ACQUIRED: a patrol spotted the handoff. ESCAPE THE POLICE."), 8.0f);
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

FString AGTTMainStoryDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTMainStoryStage::Locked: return TEXT("MAIN STORY | Finish BORROWED TRACTOR, then visit PLAYER FARM office");
        case EGTTMainStoryStage::NorthWoodPickup: return TEXT("MAIN STORY | COUNTY LEDGER | collect package at NORTH WOOD YARD");
        case EGTTMainStoryStage::ShopDelivery: return TEXT("MAIN STORY | COUNTY LEDGER | deliver package to VILLAGE SHOP");
        case EGTTMainStoryStage::TavernMeet: return TEXT("MAIN STORY | BACKROAD DEAL | meet contact at BENT AXLE 18:30-02:30");
        case EGTTMainStoryStage::EastRoadPickup: return TEXT("MAIN STORY | BACKROAD DEAL | collect unmarked crate on EAST ROAD");
        case EGTTMainStoryStage::EscapePolice: return TEXT("MAIN STORY | BACKROAD DEAL | ESCAPE POLICE");
        case EGTTMainStoryStage::WorkshopDelivery: return TEXT("MAIN STORY | BACKROAD DEAL | deliver crate to WORKSHOP");
        case EGTTMainStoryStage::FinalFarmMeet: return TEXT("MAIN STORY | FINAL FARM MEET | own 2 vehicles, then return to PLAYER FARM");
        case EGTTMainStoryStage::Completed: return TEXT("MAIN STORY | ARC 1 COMPLETE");
        default: return FString();
    }
}

void AGTTMainStoryDirector::RestoreStoryProgress(int32 SavedStage)
{
    Stage = static_cast<EGTTMainStoryStage>(FMath::Clamp(SavedStage, 0, static_cast<int32>(EGTTMainStoryStage::Completed)));
    bEscapeMessageShown = false;
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

void AGTTMainStoryDirector::SetStage(EGTTMainStoryStage NewStage, APawn* PlayerPawn, const FString& Message)
{
    Stage = NewStage;
    PushMessage(PlayerPawn, Message, 7.0f);
    if (AGTTGameMode* GM = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GM->SaveProgress();
}

void AGTTMainStoryDirector::Pay(APawn* PlayerPawn, int32 Amount, const FString& Reason)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(FMath::Max(0, Amount), FString::Printf(TEXT("%s: +$%d"), *Reason, Amount));
    }
}

void AGTTMainStoryDirector::PushMessage(APawn* PlayerPawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(Message, Duration);
}
