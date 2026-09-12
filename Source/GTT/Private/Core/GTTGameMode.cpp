#include "Core/GTTGameMode.h"

#include "Characters/GTTCharacter.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMissionComponent.h"
#include "NPC/GTTCitizenPawn.h"
#include "Police/GTTPoliceDirector.h"
#include "UI/GTTGameHUD.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTPrototypeWorld.h"

AGTTGameMode::AGTTGameMode()
{
    DefaultPawnClass = AGTTCharacter::StaticClass();
    HUDClass = AGTTGameHUD::StaticClass();

    MissionComponent = CreateDefaultSubobject<UGTTMissionComponent>(TEXT("MissionComponent"));
}

void AGTTGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (MissionComponent)
    {
        MissionComponent->StartMission(FName(TEXT("BorrowedTractor")));
    }

    if (!UGameplayStatics::GetActorOfClass(this, AGTTPoliceDirector::StaticClass()) && GetWorld())
    {
        GetWorld()->SpawnActor<AGTTPoliceDirector>();
    }

    if (!UGameplayStatics::GetActorOfClass(this, AGTTPrototypeWorld::StaticClass()) && GetWorld())
    {
        GetWorld()->SpawnActor<AGTTPrototypeWorld>();
    }
}

void AGTTGameMode::NotifyVehicleStolen(AGTTVehicleBase* Vehicle, APawn* Offender)
{
    if (!Vehicle || !Offender)
    {
        return;
    }

    int32 WitnessCount = 0;
    if (GetWorld())
    {
        for (TActorIterator<AGTTCitizenPawn> It(GetWorld()); It; ++It)
        {
            if (AGTTCitizenPawn* Citizen = *It)
            {
                WitnessCount += Citizen->TryWitnessVehicleTheft(Vehicle, Offender) ? 1 : 0;
            }
        }
    }

    if (WitnessCount == 0)
    {
        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Offender))
        {
            Economy->PushMessage(TEXT("Theft went unnoticed... for now."), 3.0f);
        }
    }

    if (!MissionComponent)
    {
        return;
    }

    if (MissionComponent->GetMissionState() == EGTTMissionState::Active &&
        MissionComponent->GetActiveMissionId() == FName(TEXT("BorrowedTractor")) &&
        MissionComponent->GetMissionStage() == 0)
    {
        MissionComponent->AdvanceMission();
    }
}

bool AGTTGameMode::TryCompleteBorrowedTractor(AGTTVehicleBase* Vehicle)
{
    if (!MissionComponent || !Vehicle || !Vehicle->WasReportedStolen() || !Vehicle->IsOccupied())
    {
        return false;
    }

    if (MissionComponent->GetMissionState() != EGTTMissionState::Active ||
        MissionComponent->GetActiveMissionId() != FName(TEXT("BorrowedTractor")) ||
        MissionComponent->GetMissionStage() < 1)
    {
        return false;
    }

    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Vehicle);
    if (Wanted && Wanted->GetWantedLevel() > 0)
    {
        return false;
    }

    MissionComponent->CompleteMission();
    Vehicle->RepairVehicle(25.0f);

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Vehicle))
    {
        Economy->AddCash(
            BorrowedTractorCashReward,
            FString::Printf(TEXT("Borrowed Tractor reward: $%d"), BorrowedTractorCashReward));
    }

    return true;
}
