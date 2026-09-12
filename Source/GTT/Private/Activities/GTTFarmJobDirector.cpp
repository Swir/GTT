#include "Activities/GTTFarmJobDirector.h"

#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "Core/GTTGameMode.h"

AGTTFarmJobDirector::AGTTFarmJobDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTFarmJobDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage != EGTTFarmJobStage::DeliverCargo) return;

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    APawn* PlayerPawn = ResolvePlayerPawn();
    if (!ControlledPawn || !PlayerPawn) return;

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    if (TimeRemaining <= 0.0f)
    {
        FailJob(PlayerPawn, TEXT("Delivery window expired. The farm cancelled the run."));
        return;
    }

    if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        const float DamageSeverity = 1.0f - Vehicle->GetConditionPercent();
        if (DamageSeverity > 0.35f)
        {
            CargoIntegrity = FMath::Max(0.0f, CargoIntegrity - DamageSeverity * DamagedVehicleCargoLossPerSecond * DeltaSeconds);
        }
        if (CargoIntegrity <= 0.02f)
        {
            FailJob(PlayerPawn, TEXT("Cargo destroyed. Job failed."));
        }
    }
}

bool AGTTFarmJobDirector::TryStartJob(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTFarmJobStage::Idle) return false;

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            PushMessage(PlayerPawn, TEXT("Lose the police before taking a legal farm contract."));
            return false;
        }
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            PushMessage(PlayerPawn, TEXT("Clear the game-warden alert before taking a legal contract."));
            return false;
        }
    }

    Stage = EGTTFarmJobStage::ReachPickup;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, TEXT("FARM CONTRACT: drive to FEED DEPOT and collect the cargo."), 6.0f);
    return true;
}

bool AGTTFarmJobDirector::TryPickupCargo(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTFarmJobStage::ReachPickup) return false;

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Cast<AGTTVehicleBase>(ControlledPawn))
    {
        PushMessage(PlayerPawn, TEXT("Bring a vehicle to load the feed pallets."));
        return false;
    }

    Stage = EGTTFarmJobStage::DeliverCargo;
    TimeRemaining = DeliveryTimeLimit;
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, TEXT("CARGO LOADED: deliver to HILL FARM before time runs out. Keep the vehicle intact."), 7.0f);
    return true;
}

bool AGTTFarmJobDirector::TryCompleteJob(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTFarmJobStage::DeliverCargo) return false;

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Cast<AGTTVehicleBase>(ControlledPawn))
    {
        PushMessage(PlayerPawn, TEXT("Delivery requires the cargo vehicle inside the drop zone."));
        return false;
    }

    const float TimeRatio = DeliveryTimeLimit > 0.0f ? TimeRemaining / DeliveryTimeLimit : 0.0f;
    const int32 IntegrityReward = FMath::RoundToInt(BaseReward * FMath::Clamp(CargoIntegrity, 0.0f, 1.0f));
    const int32 Bonus = TimeRatio >= FastDeliveryThreshold ? FastDeliveryBonus : 0;
    const int32 TotalReward = FMath::Max(25, IntegrityReward + Bonus);

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(TotalReward, FString::Printf(TEXT("Farm cargo delivery: +$%d"), TotalReward));
        Economy->PushMessage(
            FString::Printf(TEXT("DELIVERY COMPLETE: $%d | cargo %.0f%% | %.0fs left%s"),
                TotalReward, CargoIntegrity * 100.0f, TimeRemaining, Bonus > 0 ? TEXT(" | FAST BONUS") : TEXT("")),
            7.0f);
    }

    Stage = EGTTFarmJobStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    return true;
}

FString AGTTFarmJobDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTFarmJobStage::ReachPickup:
            return TEXT("FARM JOB | Reach FEED DEPOT and load cargo");
        case EGTTFarmJobStage::DeliverCargo:
            return FString::Printf(TEXT("FARM JOB | HILL FARM delivery | %.0fs | cargo %.0f%%"), TimeRemaining, CargoIntegrity * 100.0f);
        default:
            return FString();
    }
}

void AGTTFarmJobDirector::FailJob(APawn* PlayerPawn, const FString& Reason)
{
    Stage = EGTTFarmJobStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, FString::Printf(TEXT("FARM JOB FAILED: %s"), *Reason), 6.0f);
}

void AGTTFarmJobDirector::PushMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn)) Economy->PushMessage(Message, Duration);
}

APawn* AGTTFarmJobDirector::ResolvePlayerPawn() const
{
    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        if (Vehicle->GetDriverPawn()) return Vehicle->GetDriverPawn();
    }
    return ControlledPawn;
}
