#include "NPC/GTTRuralFactionDirector.h"

#include "Components/TextRenderComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTCitizenPawn.h"
#include "Save/GTTFactionSaveGame.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
APawn* ResolveCombatPawn(const UObject* Context)
{
    APawn* Controlled=UGameplayStatics::GetPlayerPawn(Context,0);
    if(AGTTVehicleBase* Vehicle=Cast<AGTTVehicleBase>(Controlled))
    {
        if(Vehicle->GetDriverPawn()) return Vehicle->GetDriverPawn();
    }
    return Controlled;
}
}

AGTTRuralFactionDirector::AGTTRuralFactionDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTRuralFactionDirector::BeginPlay()
{
    Super::BeginPlay();
    BuildZones();
    BuildOutpostMarkers();
    LoadFactionProgress();
}

void AGTTRuralFactionDirector::BuildZones()
{
    Zones = {
        {EGTTRuralFaction::RustDogs, TEXT("RUST DOGS SCRAP YARD"), FVector(-5050.0f,-450.0f,110.0f), 3, 240, 820.0f, 0.0f},
        {EGTTRuralFaction::StoneCrows, TEXT("STONE CROWS OLD QUARRY"), FVector(8600.0f,2850.0f,110.0f), 4, 360, 880.0f, 0.0f},
        {EGTTRuralFaction::MudJackals, TEXT("MUD JACKALS MARSH CAMP"), FVector(6900.0f,-3300.0f,110.0f), 5, 480, 920.0f, 0.0f}
    };
}

void AGTTRuralFactionDirector::BuildOutpostMarkers()
{
    if(!GetWorld()) return;
    for(const FFactionZone& Zone : Zones)
    {
        ATextRenderActor* Marker=GetWorld()->SpawnActor<ATextRenderActor>(Zone.Center+FVector(0,0,240),FRotator(0,180,0));
        if(!Marker||!Marker->GetTextRender()) continue;
        Marker->GetTextRender()->SetText(FText::FromString(Zone.Label+TEXT("\nHOSTILE TERRITORY")));
        Marker->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
        Marker->GetTextRender()->SetWorldSize(38.0f);
        Marker->GetTextRender()->SetTextRenderColor(FColor(220,60,35));
    }
}

void AGTTRuralFactionDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for(FFactionZone& Zone:Zones) Zone.CooldownRemaining=FMath::Max(0.0f,Zone.CooldownRemaining-DeltaSeconds);
    EvaluationClock-=DeltaSeconds;
    if(EvaluationClock>0.0f) return;
    EvaluationClock=0.4f;

    APawn* ControlledPawn=UGameplayStatics::GetPlayerPawn(this,0);
    APawn* CombatPawn=ResolveCombatPawn(this);
    if(!ControlledPawn||!CombatPawn) return;

    if(IsEncounterActive())
    {
        EncounterElapsed+=0.4f;
        if(GetActiveHostileCount()<=0) CompleteEncounter(CombatPawn);
        else if(EncounterElapsed>150.0f || FVector::DistSquared2D(ControlledPawn->GetActorLocation(),Zones[ActiveZoneIndex].Center)>FMath::Square(5200.0f))
        {
            if(UGTTPlayerEconomyComponent* Economy=UGTTGameplayStatics::FindEconomyComponentForPawn(CombatPawn)) Economy->PushMessage(TEXT("Faction pursuit broken. The gang regroups."),4.0f);
            CleanupEncounter();
        }
        return;
    }
    TryTriggerEncounter(ControlledPawn);
}

void AGTTRuralFactionDirector::TryTriggerEncounter(APawn* PlayerPawn)
{
    if(!PlayerPawn) return;
    for(int32 Index=0;Index<Zones.Num();++Index)
    {
        const FFactionZone& Zone=Zones[Index];
        if(Zone.CooldownRemaining>0.0f) continue;
        if(FVector::DistSquared2D(PlayerPawn->GetActorLocation(),Zone.Center)<=FMath::Square(Zone.TriggerRadius))
        {
            StartEncounter(Index,ResolveCombatPawn(this));
            return;
        }
    }
}

void AGTTRuralFactionDirector::StartEncounter(int32 ZoneIndex, APawn* PlayerPawn)
{
    if(!Zones.IsValidIndex(ZoneIndex)||!PlayerPawn||!GetWorld()) return;
    CleanupEncounter();
    ActiveZoneIndex=ZoneIndex;
    ActiveFaction=Zones[ZoneIndex].Faction;
    EncounterElapsed=0.0f;
    const int32 PressureBonus=FMath::Clamp(FactionVictories/2,0,2);
    const int32 HostileCount=Zones[ZoneIndex].BaseHostiles+PressureBonus;
    const EGTTHostileArchetype Pattern[] = {EGTTHostileArchetype::Scrapper,EGTTHostileArchetype::Runner,EGTTHostileArchetype::Bruiser,EGTTHostileArchetype::Enforcer};

    for(int32 I=0;I<HostileCount;++I)
    {
        const float Angle=(2.0f*PI*I)/FMath::Max(1,HostileCount);
        const FVector Offset(FMath::Cos(Angle)*FMath::FRandRange(240.0f,430.0f),FMath::Sin(Angle)*FMath::FRandRange(240.0f,430.0f),30.0f);
        AGTTCitizenPawn* Hostile=GetWorld()->SpawnActor<AGTTCitizenPawn>(Zones[ZoneIndex].Center+Offset,(-Offset).Rotation());
        if(!Hostile) continue;
        EGTTHostileArchetype Archetype=Pattern[(I+FactionVictories)%4];
        if(ActiveFaction==EGTTRuralFaction::MudJackals && I==HostileCount-1) Archetype=EGTTHostileArchetype::Bruiser;
        if(ActiveFaction==EGTTRuralFaction::StoneCrows && I==0) Archetype=EGTTHostileArchetype::Enforcer;
        Hostile->ConfigureHostileArchetype(Archetype,PlayerPawn);
        ActiveHostiles.Add(Hostile);
    }

    if(UGTTPlayerEconomyComponent* Economy=UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->PushMessage(FString::Printf(TEXT("AMBUSH: %s | %d hostiles incoming."),*GetFactionName(ActiveFaction),ActiveHostiles.Num()),6.0f);
}

int32 AGTTRuralFactionDirector::GetActiveHostileCount() const
{
    int32 Count=0;
    for(const AGTTCitizenPawn* Hostile:ActiveHostiles) if(IsValid(Hostile)&&!Hostile->IsKnockedOut()) ++Count;
    return Count;
}

void AGTTRuralFactionDirector::CompleteEncounter(APawn* PlayerPawn)
{
    if(!Zones.IsValidIndex(ActiveZoneIndex)||!PlayerPawn) { CleanupEncounter(); return; }
    FFactionZone& Zone=Zones[ActiveZoneIndex];
    const int32 Reward=Zone.BaseReward+FMath::Clamp(FactionVictories,0,8)*35;
    ++FactionVictories;
    if(ActiveFaction==EGTTRuralFaction::RustDogs) ++RustDogsDefeated;
    else if(ActiveFaction==EGTTRuralFaction::StoneCrows) ++StoneCrowsDefeated;
    else if(ActiveFaction==EGTTRuralFaction::MudJackals) ++MudJackalsDefeated;
    Zone.CooldownRemaining=150.0f;

    if(UGTTPlayerEconomyComponent* Economy=UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Reward,FString::Printf(TEXT("Faction territory cleared: +$%d"),Reward));
        Economy->PushMessage(FString::Printf(TEXT("%s ROUTED | +$%d | countryside notoriety %d"),*GetFactionName(ActiveFaction),Reward,FactionVictories),7.0f);
    }
    SaveFactionProgress();
    CleanupEncounter();
}

void AGTTRuralFactionDirector::CleanupEncounter()
{
    for(AGTTCitizenPawn* Hostile:ActiveHostiles) if(IsValid(Hostile)) Hostile->Destroy();
    ActiveHostiles.Reset();
    ActiveFaction=EGTTRuralFaction::None;
    ActiveZoneIndex=INDEX_NONE;
    EncounterElapsed=0.0f;
}

FString AGTTRuralFactionDirector::GetFactionName(EGTTRuralFaction Faction) const
{
    switch(Faction)
    {
        case EGTTRuralFaction::RustDogs: return TEXT("RUST DOGS");
        case EGTTRuralFaction::StoneCrows: return TEXT("STONE CROWS");
        case EGTTRuralFaction::MudJackals: return TEXT("MUD JACKALS");
        default: return TEXT("NONE");
    }
}

FString AGTTRuralFactionDirector::GetObjectiveText() const
{
    if(!IsEncounterActive()) return FString();
    return FString::Printf(TEXT("HOSTILE TERRITORY | %s | %d remaining | NOTORIETY %d"),*GetFactionName(ActiveFaction),GetActiveHostileCount(),FactionVictories);
}

FString AGTTRuralFactionDirector::GetThreatText() const
{
    return FString::Printf(TEXT("RURAL FACTIONS | NOTORIETY %d | Rust Dogs %d | Stone Crows %d | Mud Jackals %d"),FactionVictories,RustDogsDefeated,StoneCrowsDefeated,MudJackalsDefeated);
}

void AGTTRuralFactionDirector::LoadFactionProgress()
{
    if(!UGameplayStatics::DoesSaveGameExist(SaveSlotName,0)) return;
    if(const UGTTFactionSaveGame* Save=Cast<UGTTFactionSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName,0)))
    {
        FactionVictories=FMath::Max(0,Save->FactionVictories);
        RustDogsDefeated=FMath::Max(0,Save->RustDogsDefeated);
        StoneCrowsDefeated=FMath::Max(0,Save->StoneCrowsDefeated);
        MudJackalsDefeated=FMath::Max(0,Save->MudJackalsDefeated);
    }
}

void AGTTRuralFactionDirector::SaveFactionProgress()
{
    UGTTFactionSaveGame* Save=Cast<UGTTFactionSaveGame>(UGameplayStatics::CreateSaveGameObject(UGTTFactionSaveGame::StaticClass()));
    if(!Save) return;
    Save->FactionVictories=FactionVictories;
    Save->RustDogsDefeated=RustDogsDefeated;
    Save->StoneCrowsDefeated=StoneCrowsDefeated;
    Save->MudJackalsDefeated=MudJackalsDefeated;
    UGameplayStatics::SaveGameToSlot(Save,SaveSlotName,0);
}
