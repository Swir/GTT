#include "Activities/GTTFarmJobDirector.h"

#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "Core/GTTGameMode.h"

namespace
{
AGTTVehicleBase* FindNearbyWorkVehicle(const UObject* WorldContextObject, APawn* PlayerPawn, float Radius)
{
    if (!WorldContextObject || !PlayerPawn) return nullptr;
    if (AGTTVehicleBase* Controlled = Cast<AGTTVehicleBase>(UGameplayStatics::GetPlayerPawn(WorldContextObject, 0)))
    {
        if (Controlled->GetConditionPercent() > 0.0f) return Controlled;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || Vehicle->GetConditionPercent() <= 0.0f || Vehicle->IsHidden()) continue;
        const float DistSq = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Vehicle;
        }
    }
    return Best;
}
}

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

    const float ConditionRatio = ResolveCargoVehicleConditionRatio();
    if (ConditionRatio >= 0.0f)
    {
        const float DamageSeverity = 1.0f - ConditionRatio;
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

    ClearLoadedVehicleCargoState();
    Stage = EGTTFarmJobStage::ReachPickup;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, TEXT("FARM CONTRACT: drive to FEED DEPOT and collect the cargo. Mulebox 1200 gets a role bonus."), 6.0f);
    return true;
}

bool AGTTFarmJobDirector::TryPickupCargo(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTFarmJobStage::ReachPickup) return false;

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    LoadedNativeMulebox = Cast<AGTTMuleboxNativePawn>(ControlledPawn);
    AGTTVehicleBase* Vehicle = LoadedNativeMulebox.IsValid() ? nullptr : FindNearbyWorkVehicle(this, PlayerPawn, 700.0f);
    if (!Vehicle && !LoadedNativeMulebox.IsValid())
    {
        PushMessage(PlayerPawn, TEXT("Park a working vehicle beside the feed depot, then load the pallets."));
        return false;
    }

    LoadedMulebox = Cast<AGTTFarmVanPawn>(Vehicle);
    if (LoadedNativeMulebox.IsValid())
    {
        LoadedNativeMulebox->SetCargoLoadFactor(1.0f);
        PushMessage(PlayerPawn, TEXT("NATIVE MULEBOX LOADED: cargo weight now affects Chaos throttle and high-speed steering."), 5.0f);
    }
    else if (LoadedMulebox.IsValid())
    {
        LoadedMulebox->SetCargoLoadFactor(1.0f);
        PushMessage(PlayerPawn, TEXT("MULEBOX LOADED: cargo weight now affects throttle and high-speed steering."), 5.0f);
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

    const bool bNativeMuleboxArrived = LoadedNativeMulebox.IsValid() && UGameplayStatics::GetPlayerPawn(this, 0) == LoadedNativeMulebox.Get();
    if (!bNativeMuleboxArrived && !FindNearbyWorkVehicle(this, PlayerPawn, 750.0f))
    {
        PushMessage(PlayerPawn, TEXT("Park the cargo vehicle inside the delivery yard before unloading."));
        return false;
    }

    const float TimeRatio = DeliveryTimeLimit > 0.0f ? TimeRemaining / DeliveryTimeLimit : 0.0f;
    const int32 IntegrityReward = FMath::RoundToInt(BaseReward * FMath::Clamp(CargoIntegrity, 0.0f, 1.0f));
    const int32 Bonus = TimeRatio >= FastDeliveryThreshold ? FastDeliveryBonus : 0;
    const int32 RoleBonus = (LoadedMulebox.IsValid() || LoadedNativeMulebox.IsValid()) ? MuleboxRoleBonus : 0;
    const int32 TotalReward = FMath::Max(25, IntegrityReward + Bonus + RoleBonus);

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(TotalReward, FString::Printf(TEXT("Farm cargo delivery: +$%d"), TotalReward));
        Economy->PushMessage(
            FString::Printf(TEXT("DELIVERY COMPLETE: $%d | cargo %.0f%% | %.0fs left%s%s"),
                TotalReward, CargoIntegrity * 100.0f, TimeRemaining,
                Bonus > 0 ? TEXT(" | FAST BONUS") : TEXT(""),
                RoleBonus > 0 ? TEXT(" | MULEBOX ROLE BONUS") : TEXT("")),
            7.0f);
    }

    ClearLoadedVehicleCargoState();
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
            return FString::Printf(TEXT("FARM JOB | HILL FARM delivery | %.0fs | cargo %.0f%%%s"), TimeRemaining, CargoIntegrity * 100.0f,
                (LoadedMulebox.IsValid() || LoadedNativeMulebox.IsValid()) ? TEXT(" | MULEBOX LOADED") : TEXT(""));
        default:
            return FString();
    }
}

void AGTTFarmJobDirector::FailJob(APawn* PlayerPawn, const FString& Reason)
{
    ClearLoadedVehicleCargoState();
    Stage = EGTTFarmJobStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, FString::Printf(TEXT("FARM JOB FAILED: %s"), *Reason), 6.0f);
}

void AGTTFarmJobDirector::ClearLoadedVehicleCargoState()
{
    if (LoadedMulebox.IsValid()) LoadedMulebox->SetCargoLoadFactor(0.0f);
    if (LoadedNativeMulebox.IsValid()) LoadedNativeMulebox->SetCargoLoadFactor(0.0f);
    LoadedMulebox.Reset();
    LoadedNativeMulebox.Reset();
}

float AGTTFarmJobDirector::ResolveCargoVehicleConditionRatio() const
{
    if (LoadedNativeMulebox.IsValid()) return FMath::Clamp(LoadedNativeMulebox->GetMigrationSnapshot().ConditionPercent, 0.0f, 1.0f);
    if (LoadedMulebox.IsValid()) return LoadedMulebox->GetConditionPercent();
    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn)) return Vehicle->GetConditionPercent();
    return -1.0f;
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
    if (AGTTRoadVehicleNativePawn* NativeRoadVehicle = Cast<AGTTRoadVehicleNativePawn>(ControlledPawn))
    {
        if (NativeRoadVehicle->GetDriverPawn()) return NativeRoadVehicle->GetDriverPawn();
    }
    return ControlledPawn;
}
