#include "Activities/GTTBrawlDirector.h"

#include "Combat/GTTCombatComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTCitizenPawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"

AGTTBrawlDirector::AGTTBrawlDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

bool AGTTBrawlDirector::TryStartBrawl(APawn* PlayerPawn)
{
    if (!PlayerPawn || bActive || !GetWorld()) return false;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(TEXT("BENT AXLE BRAWL: lose the police before entering."), 4.0f);
            return false;
        }
    }
    const AGTTDayNightCycle* Cycle = Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()));
    const float Hour = Cycle ? Cycle->GetTimeOfDayHours() : 20.0f;
    const bool bNightWindow = Hour >= 18.5f || Hour < 2.5f;
    if (!bNightWindow)
    {
        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(TEXT("BENT AXLE BRAWL opens 18:30-02:30."), 4.0f);
        return false;
    }

    const FVector Base(1450.0f, 2650.0f, 120.0f);
    const FVector Offsets[] = { FVector(-260,120,0), FVector(230,160,0), FVector(30,-250,0) };
    for (const FVector& Offset : Offsets)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (AGTTCitizenPawn* Brawler = GetWorld()->SpawnActor<AGTTCitizenPawn>(Base + Offset, FRotator::ZeroRotator, Params))
        {
            Brawler->StartBrawlWith(PlayerPawn);
            Brawlers.Add(Brawler);
        }
    }
    if (Brawlers.Num() == 0) return false;
    bActive = true;
    TimeRemaining = 120.0f;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->PushMessage(TEXT("BENT AXLE BRAWL: three locals step outside. Stay standing and drop all three."), 6.0f);
    return true;
}

void AGTTBrawlDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bActive) return;
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) { ResetBrawl(true); return; }
    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    int32 KnockedOut = 0;
    for (const AGTTCitizenPawn* Brawler : Brawlers) if (!IsValid(Brawler) || Brawler->IsKnockedOut()) ++KnockedOut;
    if (KnockedOut >= Brawlers.Num()) { FinishBrawl(PlayerPawn); return; }
    if (TimeRemaining <= 0.0f)
    {
        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(TEXT("BENT AXLE BRAWL FAILED: the crowd got bored."), 5.0f);
        ResetBrawl(true);
    }
}

void AGTTBrawlDirector::FinishBrawl(APawn* PlayerPawn)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Reward, FString::Printf(TEXT("Bent Axle brawl purse: +$%d"), Reward));
        Economy->PushMessage(FString::Printf(TEXT("BENT AXLE BRAWL WON: +$%d"), Reward), 6.0f);
    }
    ResetBrawl(true);
}

void AGTTBrawlDirector::ResetBrawl(bool bDestroyRemaining)
{
    if (bDestroyRemaining)
    {
        for (AGTTCitizenPawn* Brawler : Brawlers) if (IsValid(Brawler)) Brawler->Destroy();
    }
    Brawlers.Reset();
    bActive = false;
    TimeRemaining = 0.0f;
}

FString AGTTBrawlDirector::GetObjectiveText() const
{
    if (!bActive) return FString();
    int32 Standing = 0;
    for (const AGTTCitizenPawn* Brawler : Brawlers) if (IsValid(Brawler) && !Brawler->IsKnockedOut()) ++Standing;
    return FString::Printf(TEXT("BENT AXLE BRAWL | opponents %d | %.0fs"), Standing, TimeRemaining);
}
