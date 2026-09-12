#include "Core/GTTGameMode.h"

#include "Characters/GTTCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMissionComponent.h"
#include "Police/GTTPoliceDirector.h"
#include "UI/GTTGameHUD.h"
#include "Vehicles/GTTVehicleBase.h"

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
}

void AGTTGameMode::NotifyVehicleStolen(AGTTVehicleBase* Vehicle)
{
    if (!MissionComponent || !Vehicle)
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
