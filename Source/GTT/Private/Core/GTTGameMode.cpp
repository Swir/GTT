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
#include "Save/GTTSaveGame.h"
#include "UI/GTTGameHUD.h"
#include "Vehicles/GTTTractorPawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
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

    if (!UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()) && GetWorld())
    {
        DayNightCycle = GetWorld()->SpawnActor<AGTTDayNightCycle>();
    }
    else
    {
        DayNightCycle = Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()));
    }

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        LoadProgress();
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
        PushPlayerMessage(Offender, TEXT("Theft went unnoticed... for now."), 3.0f);
    }

    if (MissionComponent &&
        MissionComponent->GetMissionState() == EGTTMissionState::Active &&
        MissionComponent->GetActiveMissionId() == FName(TEXT("BorrowedTractor")) &&
        MissionComponent->GetMissionStage() == 0 &&
        Vehicle->GetPersistentVehicleId() == FName(TEXT("RustyFieldmaster60")))
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

    if (Vehicle->GetPersistentVehicleId() != FName(TEXT("RustyFieldmaster60")) ||
        MissionComponent->GetMissionState() != EGTTMissionState::Active ||
        MissionComponent->GetActiveMissionId() != FName(TEXT("BorrowedTractor")) ||
        MissionComponent->GetMissionStage() < 1)
    {
        return false;
    }

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Vehicle))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            return false;
        }
    }

    MissionComponent->CompleteMission();
    Vehicle->RepairVehicle(25.0f);
    Vehicle->MarkOwnedByPlayer();

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Vehicle))
    {
        Economy->AddCash(
            BorrowedTractorCashReward,
            FString::Printf(TEXT("Borrowed Tractor reward: $%d"), BorrowedTractorCashReward));
        Economy->PushMessage(TEXT("The Rusty Fieldmaster is now yours. Garage ownership unlocked."), 5.0f);
    }

    SaveProgress();
    return true;
}

bool AGTTGameMode::SaveProgress()
{
    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!ControlledPawn)
    {
        return false;
    }

    APawn* PlayerPawn = ControlledPawn;
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        if (Vehicle->GetDriverPawn())
        {
            PlayerPawn = Vehicle->GetDriverPawn();
        }
    }

    UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::CreateSaveGameObject(UGTTSaveGame::StaticClass()));
    if (!Save)
    {
        return false;
    }

    Save->SaveVersion = 2;

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(ControlledPawn))
    {
        Save->Cash = Economy->GetCash();
        Save->FishCount = Economy->GetFishCount();
        Save->FishWeightKg = Economy->GetFishWeightKg();
    }

    Save->PlayerTransform = PlayerPawn->GetActorTransform();
    Save->bBorrowedTractorCompleted = MissionComponent &&
        MissionComponent->GetActiveMissionId() == FName(TEXT("BorrowedTractor")) &&
        MissionComponent->GetMissionState() == EGTTMissionState::Completed;

    if (AGTTDayNightCycle* Cycle = DayNightCycle.Get())
    {
        Save->DayNumber = Cycle->GetDayNumber();
        Save->TimeOfDayHours = Cycle->GetTimeOfDayHours();
    }

    Save->OwnedVehicles.Reset();
    if (GetWorld())
    {
        for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
        {
            AGTTVehicleBase* Vehicle = *It;
            if (!Vehicle || !Vehicle->IsOwnedByPlayer() || Vehicle->GetPersistentVehicleId().IsNone())
            {
                continue;
            }

            FGTTStoredVehicleData Stored;
            Stored.VehicleId = Vehicle->GetPersistentVehicleId();
            Stored.Transform = Vehicle->GetActorTransform();
            Stored.ConditionPercent = Vehicle->GetConditionPercent();
            Stored.FuelLiters = Vehicle->GetFuelLiters();
            Save->OwnedVehicles.Add(Stored);

            if (Stored.VehicleId == FName(TEXT("RustyFieldmaster60")))
            {
                Save->bTractorOwned = true;
                Save->TractorTransform = Stored.Transform;
                Save->TractorConditionPercent = Stored.ConditionPercent;
                Save->TractorFuelLiters = Stored.FuelLiters;
            }
        }
    }

    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
    if (bSaved)
    {
        PushPlayerMessage(
            ControlledPawn,
            FString::Printf(TEXT("Progress saved. Garage: %d/%d vehicles."), Save->OwnedVehicles.Num(), GarageCapacity),
            2.5f);
    }
    return bSaved;
}

bool AGTTGameMode::LoadProgress()
{
    UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Save)
    {
        return false;
    }

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        Vehicle->ExitVehicle();
        ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    }
    if (!ControlledPawn)
    {
        return false;
    }

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(ControlledPawn))
    {
        Economy->RestoreState(Save->Cash, Save->FishCount, Save->FishWeightKg);
    }
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(ControlledPawn))
    {
        Wanted->ClearWanted();
    }

    ControlledPawn->SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);

    if (Save->bBorrowedTractorCompleted && MissionComponent && MissionComponent->GetMissionState() == EGTTMissionState::Active)
    {
        MissionComponent->CompleteMission();
    }

    if (GetWorld())
    {
        if (Save->SaveVersion >= 2 && Save->OwnedVehicles.Num() > 0)
        {
            for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
            {
                AGTTVehicleBase* Vehicle = *It;
                if (!Vehicle)
                {
                    continue;
                }

                const FGTTStoredVehicleData* Stored = Save->OwnedVehicles.FindByPredicate(
                    [Vehicle](const FGTTStoredVehicleData& Data)
                    {
                        return Data.VehicleId == Vehicle->GetPersistentVehicleId();
                    });

                if (Stored)
                {
                    Vehicle->RestorePersistentState(
                        Stored->Transform,
                        Stored->ConditionPercent,
                        Stored->FuelLiters,
                        true);
                }
            }
        }
        else if (Save->bTractorOwned)
        {
            // Migration path for 0.0.5 saves.
            if (AGTTTractorPawn* Tractor = Cast<AGTTTractorPawn>(UGameplayStatics::GetActorOfClass(this, AGTTTractorPawn::StaticClass())))
            {
                Tractor->RestorePersistentState(
                    Save->TractorTransform,
                    Save->TractorConditionPercent,
                    Save->TractorFuelLiters,
                    true);
            }
        }
    }

    if (AGTTDayNightCycle* Cycle = DayNightCycle.Get())
    {
        Cycle->RestoreTime(Save->DayNumber, Save->TimeOfDayHours);
    }

    PushPlayerMessage(
        ControlledPawn,
        FString::Printf(TEXT("Progress loaded. Garage: %d/%d vehicles."), GetOwnedVehicleCount(), GarageCapacity),
        3.0f);
    return true;
}

bool AGTTGameMode::TryArrestPlayer(APawn* PursuedPawn)
{
    if (!PursuedPawn)
    {
        return false;
    }

    const int32 WantedLevel = UGTTGameplayStatics::GetPlayerWantedLevel(this, 0);
    if (WantedLevel <= 0)
    {
        return false;
    }

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        Vehicle->ExitVehicle();
        ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    }
    if (!ControlledPawn)
    {
        return false;
    }

    const int32 Fine = 65 + WantedLevel * 55;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(ControlledPawn))
    {
        Economy->ChargeFine(Fine, FString::Printf(TEXT("ARRESTED - fine $%d."), Fine));
    }
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(ControlledPawn))
    {
        Wanted->ClearWanted();
    }

    ControlledPawn->SetActorLocation(FVector(2700.0f, -2050.0f, 120.0f), false, nullptr, ETeleportType::TeleportPhysics);
    PushPlayerMessage(ControlledPawn, TEXT("Released outside the police station. Try being less obvious."), 5.0f);
    SaveProgress();
    return true;
}

bool AGTTGameMode::StartFarmJob(APawn* PlayerPawn)
{
    if (!PlayerPawn || bFarmJobActive)
    {
        return false;
    }

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            PushPlayerMessage(PlayerPawn, TEXT("Lose the police before taking a legal job."));
            return false;
        }
    }

    bFarmJobActive = true;
    PushPlayerMessage(PlayerPawn, TEXT("LEGAL JOB STARTED: Take a vehicle to FIELD DELIVERY and finish the run."), 6.0f);
    return true;
}

bool AGTTGameMode::CompleteFarmJob(APawn* PlayerPawn)
{
    if (!PlayerPawn || !bFarmJobActive)
    {
        return false;
    }

    bFarmJobActive = false;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(FarmJobReward, FString::Printf(TEXT("Farm job completed: +$%d"), FarmJobReward));
    }
    SaveProgress();
    return true;
}

bool AGTTGameMode::TryRegisterVehicle(AGTTVehicleBase* Vehicle, APawn* PlayerPawn, int32 RegistrationCost)
{
    if (!Vehicle || !PlayerPawn)
    {
        return false;
    }

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn);
    if (!Economy)
    {
        return false;
    }

    if (Wanted && Wanted->GetWantedLevel() > 0)
    {
        Economy->PushMessage(TEXT("Garage refuses registration while police are looking for you."), 4.0f);
        return false;
    }

    if (Vehicle->IsOwnedByPlayer())
    {
        Economy->PushMessage(TEXT("Owned vehicle parked. Saving all garage slots..."), 3.0f);
        return SaveProgress();
    }

    if (Vehicle->GetPersistentVehicleId().IsNone())
    {
        Economy->PushMessage(TEXT("This vehicle has no persistent garage ID yet."), 4.0f);
        return false;
    }

    const int32 CurrentCount = GetOwnedVehicleCount();
    if (CurrentCount >= GarageCapacity)
    {
        Economy->PushMessage(FString::Printf(TEXT("Garage full: %d/%d slots."), CurrentCount, GarageCapacity), 4.0f);
        return false;
    }

    const int32 Cost = FMath::Max(0, RegistrationCost);
    if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Vehicle registration: -$%d"), Cost)))
    {
        return false;
    }

    Vehicle->MarkOwnedByPlayer();
    Economy->PushMessage(
        FString::Printf(
            TEXT("%s registered. Garage: %d/%d."),
            *Vehicle->GetVehicleDisplayName().ToString(),
            CurrentCount + 1,
            GarageCapacity),
        5.0f);

    SaveProgress();
    return true;
}

int32 AGTTGameMode::GetOwnedVehicleCount() const
{
    int32 Count = 0;
    if (!GetWorld())
    {
        return Count;
    }

    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        const AGTTVehicleBase* Vehicle = *It;
        if (Vehicle && Vehicle->IsOwnedByPlayer())
        {
            ++Count;
        }
    }
    return Count;
}

void AGTTGameMode::PushPlayerMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn))
    {
        Economy->PushMessage(Message, Duration);
    }
}
