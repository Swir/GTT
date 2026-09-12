#include "Activities/GTTRecoveryDirector.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Activities/GTTRecoveryTerminal.h"
#include "Activities/GTTRuralWorkDirector.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTOldCarPawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTTerrainZone.h"

AGTTRecoveryDirector::AGTTRecoveryDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTRecoveryDirector::BeginPlay()
{
    Super::BeginPlay();
    if (!GetWorld()) return;

    GetWorld()->SpawnActor<AGTTRecoveryTerminal>(FVector(-1350.0f, 2400.0f, 55.0f), FRotator::ZeroRotator);

    if (AGTTTerrainZone* Dirt = GetWorld()->SpawnActor<AGTTTerrainZone>(FVector(4150.0f, 2950.0f, 40.0f), FRotator::ZeroRotator))
        Dirt->ConfigureZone(EGTTVehicleTerrainType::Dirt, FVector(900.0f, 420.0f, 100.0f), TEXT("DIRT FARM TRACK | REDUCED GRIP"));
    if (AGTTTerrainZone* Mud = GetWorld()->SpawnActor<AGTTTerrainZone>(FVector(6100.0f, 650.0f, 40.0f), FRotator::ZeroRotator))
        Mud->ConfigureZone(EGTTVehicleTerrainType::Mud, FVector(850.0f, 420.0f, 100.0f), TEXT("MUD TRACK | TRACTION PENALTY"));
    if (AGTTTerrainZone* DeepMud = GetWorld()->SpawnActor<AGTTTerrainZone>(FVector(6750.0f, 2600.0f, 40.0f), FRotator::ZeroRotator))
        DeepMud->ConfigureZone(EGTTVehicleTerrainType::DeepMud, FVector(700.0f, 350.0f, 100.0f), TEXT("DEEP MUD | TRACTOR FAVORED"));
}

void AGTTRecoveryDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage == EGTTRecoveryStage::Idle) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* ControlledVehicle = Cast<AGTTVehicleBase>(PlayerPawn))
    {
        if (ControlledVehicle->GetDriverPawn()) PlayerPawn = ControlledVehicle->GetDriverPawn();
    }

    AGTTVehicleBase* Target = RecoveryTarget.Get();
    if (!Target)
    {
        FailRecovery(PlayerPawn, TEXT("The disabled vehicle is no longer available."));
        return;
    }

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    if (TimeRemaining <= 0.0f)
    {
        FailRecovery(PlayerPawn, TEXT("The roadside assistance window expired."));
        return;
    }

    if (Stage == EGTTRecoveryStage::ReachBreakdown && Target->IsBeingTowed())
    {
        Stage = EGTTRecoveryStage::TowToWorkshop;
        PushMessage(PlayerPawn, TEXT("HITCH SECURED: tow the disabled Rattleback to the WORKSHOP recovery bay."), 6.0f);
    }
}

bool AGTTRecoveryDirector::CanStartLegalRecovery(APawn* PlayerPawn) const
{
    if (!PlayerPawn || Stage != EGTTRecoveryStage::Idle) return false;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0) return false;
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0) return false;
    }
    if (const AGTTFarmJobDirector* Farm = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(this, AGTTFarmJobDirector::StaticClass())))
    {
        if (Farm->IsJobActive()) return false;
    }
    if (const AGTTRuralWorkDirector* Rural = Cast<AGTTRuralWorkDirector>(UGameplayStatics::GetActorOfClass(this, AGTTRuralWorkDirector::StaticClass())))
    {
        if (Rural->IsWorkActive()) return false;
    }
    return true;
}

bool AGTTRecoveryDirector::TryStartRecovery(APawn* PlayerPawn)
{
    if (!CanStartLegalRecovery(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("RECOVERY DISPATCH: clear authority attention and finish current legal work first."));
        return false;
    }

    SpawnRecoveryTarget();
    if (!RecoveryTarget.IsValid())
    {
        PushMessage(PlayerPawn, TEXT("RECOVERY DISPATCH: no breakdown could be assigned."));
        return false;
    }

    Stage = EGTTRecoveryStage::ReachBreakdown;
    TimeRemaining = RecoveryTimeLimit;
    PushMessage(PlayerPawn, TEXT("RECOVERY CONTRACT: find the disabled Rattleback on the east/forest roads. Park a vehicle close and press T to attach the tow hitch."), 8.0f);
    return true;
}

void AGTTRecoveryDirector::SpawnRecoveryTarget()
{
    if (!GetWorld()) return;

    static const FVector Locations[] =
    {
        FVector(5050.0f, 1850.0f, 110.0f),
        FVector(6900.0f, 450.0f, 110.0f),
        FVector(4750.0f, 4300.0f, 110.0f)
    };
    static const FRotator Rotations[] =
    {
        FRotator(0.0f, 20.0f, 0.0f),
        FRotator(0.0f, 105.0f, 0.0f),
        FRotator(0.0f, -35.0f, 0.0f)
    };

    const int32 Pick = FMath::RandRange(0, UE_ARRAY_COUNT(Locations) - 1);
    CurrentBreakdownLocation = Locations[Pick];
    AGTTOldCarPawn* Target = GetWorld()->SpawnActor<AGTTOldCarPawn>(Locations[Pick], Rotations[Pick]);
    if (Target)
    {
        Target->ConfigureRecoveryTarget();
        RecoveryTarget = Target;
    }
}

bool AGTTRecoveryDirector::TryFinishRecovery(APawn* PlayerPawn, const FVector& WorkshopLocation)
{
    if (!PlayerPawn || Stage != EGTTRecoveryStage::TowToWorkshop) return false;
    AGTTVehicleBase* Target = RecoveryTarget.Get();
    if (!Target) return false;

    const float Distance = FVector::Dist2D(Target->GetActorLocation(), WorkshopLocation);
    if (Distance > WorkshopDropRadius)
    {
        PushMessage(PlayerPawn, FString::Printf(TEXT("Bring the disabled vehicle into the recovery bay (%.0f m away)."), Distance / 100.0f));
        return false;
    }
    if (Target->GetSpeedKmh() > 12.0f)
    {
        PushMessage(PlayerPawn, TEXT("Stop the recovered vehicle inside the bay before checking it in."));
        return false;
    }

    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (Vehicle && Vehicle->GetTowedVehicle() == Target)
        {
            Vehicle->ReleaseTowHook();
            break;
        }
    }

    const bool bFast = TimeRemaining >= RecoveryTimeLimit * 0.42f;
    const int32 Reward = RecoveryBaseReward + (bFast ? RecoveryFastBonus : 0);
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Reward, FString::Printf(TEXT("Vehicle recovery: +$%d"), Reward));
        Economy->PushMessage(FString::Printf(TEXT("RECOVERY COMPLETE | +$%d%s"), Reward, bFast ? TEXT(" | FAST RESPONSE BONUS") : TEXT("")), 7.0f);
    }

    Target->Destroy();
    RecoveryTarget.Reset();
    Stage = EGTTRecoveryStage::Idle;
    TimeRemaining = 0.0f;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    return true;
}

FString AGTTRecoveryDirector::GetObjectiveText() const
{
    switch (Stage)
    {
    case EGTTRecoveryStage::ReachBreakdown:
        return FString::Printf(TEXT("RECOVERY | FIND DISABLED RATTLEBACK | %.0fs | T attach hitch"), TimeRemaining);
    case EGTTRecoveryStage::TowToWorkshop:
        return FString::Printf(TEXT("RECOVERY | TOW TO WORKSHOP | %.0fs | T releases hitch"), TimeRemaining);
    default:
        return FString();
    }
}

void AGTTRecoveryDirector::FailRecovery(APawn* PlayerPawn, const FString& Reason)
{
    if (AGTTVehicleBase* Target = RecoveryTarget.Get())
    {
        for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
        {
            AGTTVehicleBase* Vehicle = *It;
            if (Vehicle && Vehicle->GetTowedVehicle() == Target) Vehicle->ReleaseTowHook();
        }
        Target->Destroy();
    }
    RecoveryTarget.Reset();
    Stage = EGTTRecoveryStage::Idle;
    TimeRemaining = 0.0f;
    PushMessage(PlayerPawn, FString::Printf(TEXT("RECOVERY FAILED: %s"), *Reason), 6.0f);
}

void AGTTRecoveryDirector::PushMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn)) Economy->PushMessage(Message, Duration);
}
