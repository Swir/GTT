#include "Activities/GTTFarmJobDirector.h"

#include "Activities/GTTFarmJobTerminal.h"
#include "Core/GTTGameplayStatics.h"
#include "Core/GTTGameMode.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTLogisticsDispatcherPawn.h"
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTLogisticsReputationSubsystem.h"

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

void AGTTFarmJobDirector::BeginPlay()
{
    Super::BeginPlay();
    if (!GetWorld()) return;

    if (AGTTFarmJobTerminal* FinalStop = GetWorld()->SpawnActor<AGTTFarmJobTerminal>(FVector(7850.0f, 1120.0f, 55.0f), FRotator::ZeroRotator))
    {
        FinalStop->SetTerminalType(EGTTFarmJobTerminalType::FinalFinish);
    }

    SpawnLogisticsDispatchers();
}

void AGTTFarmJobDirector::SpawnLogisticsDispatchers()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TSet<FName> ExistingRoles;
    for (TActorIterator<AGTTLogisticsDispatcherPawn> It(World); It; ++It)
    {
        if (AGTTLogisticsDispatcherPawn* Dispatcher = *It) ExistingRoles.Add(Dispatcher->GetRoleTag());
    }

    for (TActorIterator<AGTTFarmJobTerminal> It(World); It; ++It)
    {
        AGTTFarmJobTerminal* Terminal = *It;
        if (!Terminal) continue;

        FName RoleTag = NAME_None;
        FString DisplayName;
        switch (Terminal->GetTerminalType())
        {
            case EGTTFarmJobTerminalType::Pickup:
                RoleTag = FName(TEXT("FeedDepotDispatcher"));
                DisplayName = TEXT("Feed Dispatcher");
                break;
            case EGTTFarmJobTerminalType::Finish:
                RoleTag = FName(TEXT("HillFarmReceiver"));
                DisplayName = TEXT("Hill Receiver");
                break;
            case EGTTFarmJobTerminalType::FinalFinish:
                RoleTag = FName(TEXT("WoodYardForeman"));
                DisplayName = TEXT("Wood Foreman");
                break;
            default:
                continue;
        }

        if (ExistingRoles.Contains(RoleTag)) continue;
        const FVector WorkLocation = Terminal->GetActorLocation() + FVector(180.0f, 120.0f, 40.0f);
        const FVector HomeLocation = WorkLocation + FVector(-850.0f, -1850.0f, 0.0f);
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        if (AGTTLogisticsDispatcherPawn* Dispatcher = World->SpawnActor<AGTTLogisticsDispatcherPawn>(WorkLocation, Terminal->GetActorRotation(), SpawnParams))
        {
            Dispatcher->ConfigureDispatcher(RoleTag, DisplayName, WorkLocation, HomeLocation);
            ExistingRoles.Add(RoleTag);
        }
    }
}

void AGTTFarmJobDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage != EGTTFarmJobStage::DeliverCargo && Stage != EGTTFarmJobStage::DeliverFinalStop) return;

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    APawn* PlayerPawn = ResolvePlayerPawn();
    if (!ControlledPawn || !PlayerPawn) return;

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0) bPoliceIncidentDuringRun = true;
    }

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    if (TimeRemaining <= 0.0f)
    {
        FailJob(PlayerPawn, TEXT("Delivery window expired. The rural buyers cancelled the run."));
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

    MarketMultiplierAtStart = 1.0f;
    RouteTierAtStart = 1;
    CargoUnitsReserved = 0;
    CargoCommodityAtStart = TEXT("ANIMAL FEED");
    CargoPriorityAtStart = TEXT("HILL FARM DIRECT");
    bPoliceIncidentDuringRun = false;
    if (GetWorld())
    {
        if (const UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
        {
            if (!Logistics->IsCargoDepotWindowOpen())
            {
                PushMessage(PlayerPawn, FString::Printf(TEXT("FEED DEPOT %s. Cargo staff follow the village work shift; prepare the Mulebox now and return at 07:00."), *Logistics->GetCargoScheduleLabel()), 6.0f);
                return false;
            }
            if (!Logistics->CanAcceptCargoContract())
            {
                PushMessage(PlayerPawn, FString::Printf(TEXT("CARGO MARKET HAS NO OPEN LOAD: %s. Backlog survives into the next work day and new orders/restock are applied there."), *Logistics->GetCargoStockSummary()), 7.0f);
                return false;
            }
            MarketMultiplierAtStart = Logistics->GetCargoMarketMultiplier();
            const int32 CapabilityTier = Logistics->GetCargoRouteTier();
            RouteTierAtStart = Logistics->GetActiveCargoOrderTier();
            CargoCommodityAtStart = Logistics->GetCargoCommodityLabel();
            CargoPriorityAtStart = Logistics->GetCargoOrderPriorityLabel();
            PushMessage(PlayerPawn, FString::Printf(TEXT("CARGO ORDER: %s | %s | CAP T%d / ORDER T%d | REP %s %d | payout x%.2f locked."),
                *CargoPriorityAtStart, *Logistics->GetCargoStockSummary(), CapabilityTier, RouteTierAtStart,
                *Logistics->GetTierLabel(), Logistics->GetReputation(), MarketMultiplierAtStart), 7.5f);
        }

        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            const FGTTFleetMissionAssessment Assessment = Fleet->AssessJobReadiness(FName(TEXT("FarmCargo")));
            PushMessage(PlayerPawn, Fleet->BuildJobDispatchHint(FName(TEXT("FarmCargo"))), 6.0f);
            if (Assessment.Readiness == EGTTFleetMissionReadiness::ServiceRequired)
            {
                FleetPayoutMultiplier = 0.75f;
                PushMessage(PlayerPawn, TEXT("CARGO LOADOUT NEEDS SERVICE: bypassing prep puts 25% of this contract payout at risk. Use the unified contract board to prep first."), 6.0f);
            }
            else if (Assessment.Readiness == EGTTFleetMissionReadiness::Advisory)
            {
                FleetPayoutMultiplier = 0.90f;
                PushMessage(PlayerPawn, TEXT("CARGO LOADOUT CAUTION: bypassing recommended prep reduces this contract payout by 10%."), 5.5f);
            }
            else
            {
                FleetPayoutMultiplier = 1.0f;
            }
        }

        if (UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
        {
            FString ReserveReason;
            if (!Logistics->ReserveCargoContract(RouteTierAtStart, CargoUnitsReserved, ReserveReason))
            {
                PushMessage(PlayerPawn, FString::Printf(TEXT("CARGO RESERVATION FAILED: %s"), *ReserveReason), 7.0f);
                return false;
            }
            PushMessage(PlayerPawn, ReserveReason, 5.5f);
            // Inventory is authoritative as soon as the player accepts the load, so save/reload cannot duplicate depot stock.
            if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
        }
    }

    ClearLoadedVehicleCargoState();
    Stage = EGTTFarmJobStage::ReachPickup;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    const FString RouteText = RouteTierAtStart >= 2
        ? TEXT("FEED DEPOT -> HILL FARM relay -> NORTH WOOD YARD")
        : TEXT("FEED DEPOT -> HILL FARM");
    PushMessage(PlayerPawn, FString::Printf(TEXT("FARM CONTRACT T%d: %s | %s x%d reserved | %s. Mulebox 1200 gets a role bonus."),
        RouteTierAtStart, *RouteText, *CargoCommodityAtStart, CargoUnitsReserved, *CargoPriorityAtStart), 7.0f);
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
    const float CargoLoadFactor = RouteTierAtStart >= 3 ? 1.20f : 1.0f;
    if (LoadedNativeMulebox.IsValid())
    {
        if (RouteTierAtStart >= 3) LoadedNativeMulebox->SetCargoLoadFactor(CargoLoadFactor);
        else LoadedNativeMulebox->SetCargoLoadFactor(1.0f);
        PushMessage(PlayerPawn, RouteTierAtStart >= 3
            ? TEXT("NATIVE MULEBOX BULK-LOADED: 4-unit order adds extra cargo mass to Chaos throttle and high-speed steering.")
            : TEXT("NATIVE MULEBOX LOADED: cargo weight now affects Chaos throttle and high-speed steering."), 5.5f);
    }
    else if (LoadedMulebox.IsValid())
    {
        if (RouteTierAtStart >= 3) LoadedMulebox->SetCargoLoadFactor(CargoLoadFactor);
        else LoadedMulebox->SetCargoLoadFactor(1.0f);
        PushMessage(PlayerPawn, RouteTierAtStart >= 3
            ? TEXT("MULEBOX BULK-LOADED: 4-unit order carries extra handling load.")
            : TEXT("MULEBOX LOADED: cargo weight now affects throttle and high-speed steering."), 5.0f);
    }

    Stage = EGTTFarmJobStage::DeliverCargo;
    TimeRemaining = DeliveryTimeLimit + (RouteTierAtStart >= 2 ? ExtendedRouteExtraTime : 0.0f) + (RouteTierAtStart >= 3 ? BulkRouteExtraTime : 0.0f);
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, RouteTierAtStart >= 2
        ? FString::Printf(TEXT("%s LOADED: first handoff HILL FARM, then continue the same load to NORTH WOOD YARD. %s | one timer, one cargo condition."), *CargoCommodityAtStart, *CargoPriorityAtStart)
        : FString::Printf(TEXT("%s LOADED: deliver to HILL FARM before time runs out. %s | keep the vehicle intact."), *CargoCommodityAtStart, *CargoPriorityAtStart), 7.0f);
    return true;
}

bool AGTTFarmJobDirector::IsDeliveryVehiclePresent(APawn* PlayerPawn) const
{
    const bool bNativeMuleboxArrived = LoadedNativeMulebox.IsValid() && UGameplayStatics::GetPlayerPawn(this, 0) == LoadedNativeMulebox.Get();
    return bNativeMuleboxArrived || FindNearbyWorkVehicle(this, PlayerPawn, 750.0f) != nullptr;
}

bool AGTTFarmJobDirector::IsPoliceBlockingHandoff(APawn* PlayerPawn, const FString& LocationLabel)
{
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            bPoliceIncidentDuringRun = true;
            PushMessage(PlayerPawn, FString::Printf(TEXT("%s HANDOFF BLOCKED: legal staff will not sign while police are searching for you. The delivery clock keeps running."), *LocationLabel), 6.0f);
            return true;
        }
    }
    return false;
}

bool AGTTFarmJobDirector::TryCompleteJob(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTFarmJobStage::DeliverCargo) return false;
    if (IsPoliceBlockingHandoff(PlayerPawn, TEXT("HILL FARM"))) return false;
    if (!IsDeliveryVehiclePresent(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("Park the cargo vehicle inside the Hill Farm delivery yard before unloading."));
        return false;
    }

    if (RouteTierAtStart >= 2)
    {
        Stage = EGTTFarmJobStage::DeliverFinalStop;
        PushMessage(PlayerPawn, FString::Printf(TEXT("HILL FARM RELAY SIGNED: keep the same load moving to NORTH WOOD YARD. %.0fs remain | cargo %.0f%% | chain bonus $%d | %s."),
            TimeRemaining, CargoIntegrity * 100.0f, RouteTierAtStart >= 3 ? ReliableChainBonus : TrustedChainBonus, *CargoPriorityAtStart), 7.0f);
        return true;
    }

    return CompleteCargoContract(PlayerPawn, false);
}

bool AGTTFarmJobDirector::TryCompleteFinalStop(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTFarmJobStage::DeliverFinalStop) return false;
    if (IsPoliceBlockingHandoff(PlayerPawn, TEXT("NORTH WOOD YARD"))) return false;
    if (!IsDeliveryVehiclePresent(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("Park the loaded cargo vehicle inside North Wood Yard before the final handoff."));
        return false;
    }
    return CompleteCargoContract(PlayerPawn, true);
}

bool AGTTFarmJobDirector::CompleteCargoContract(APawn* PlayerPawn, bool bExtendedRoute)
{
    const float ActiveTimeLimit = DeliveryTimeLimit + (RouteTierAtStart >= 2 ? ExtendedRouteExtraTime : 0.0f) + (RouteTierAtStart >= 3 ? BulkRouteExtraTime : 0.0f);
    const float TimeRatio = ActiveTimeLimit > 0.0f ? TimeRemaining / ActiveTimeLimit : 0.0f;
    const int32 IntegrityReward = FMath::RoundToInt(BaseReward * FMath::Clamp(CargoIntegrity, 0.0f, 1.0f));
    const int32 Bonus = TimeRatio >= FastDeliveryThreshold ? FastDeliveryBonus : 0;
    const int32 RoleBonus = (LoadedMulebox.IsValid() || LoadedNativeMulebox.IsValid()) ? MuleboxRoleBonus : 0;
    const int32 RouteBonus = bExtendedRoute ? (RouteTierAtStart >= 3 ? ReliableChainBonus : TrustedChainBonus) : 0;
    const int32 RawReward = IntegrityReward + Bonus + RoleBonus + RouteBonus;
    const int32 FleetAdjustedReward = FMath::RoundToInt(RawReward * FleetPayoutMultiplier);
    const int32 TotalReward = FMath::Max(25, FMath::RoundToInt(static_cast<float>(FleetAdjustedReward) * MarketMultiplierAtStart));

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(TotalReward, FString::Printf(TEXT("Rural cargo delivery: +$%d"), TotalReward));
        Economy->PushMessage(
            FString::Printf(TEXT("DELIVERY COMPLETE: $%d | cargo %.0f%% | %.0fs left%s%s%s | MARKET x%.2f%s | %s"),
                TotalReward, CargoIntegrity * 100.0f, TimeRemaining,
                Bonus > 0 ? TEXT(" | FAST BONUS") : TEXT(""),
                RoleBonus > 0 ? TEXT(" | MULEBOX ROLE BONUS") : TEXT(""),
                FleetPayoutMultiplier < 0.999f ? TEXT(" | UNPREPARED FLEET PENALTY") : TEXT(""),
                MarketMultiplierAtStart,
                bExtendedRoute ? TEXT(" | CHAIN COMPLETE") : TEXT(""),
                *CargoPriorityAtStart),
            8.0f);
    }

    if (GetWorld())
    {
        if (UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
        {
            Logistics->SettleCargoContract(CargoUnitsReserved, bExtendedRoute, true);
            Logistics->RecordCargoSuccess(TotalReward, CargoIntegrity, Bonus > 0, bPoliceIncidentDuringRun, bExtendedRoute);
            PushMessage(PlayerPawn, FString::Printf(TEXT("LOGISTICS UPDATED: REP %s %d | CARGO %d complete | %s | %s"),
                *Logistics->GetTierLabel(), Logistics->GetReputation(), Logistics->GetCargoCompletedRuns(),
                *Logistics->GetCargoStockSummary(), *Logistics->GetRecentHistorySummary()), 7.0f);
        }
    }

    ClearLoadedVehicleCargoState();
    Stage = EGTTFarmJobStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    FleetPayoutMultiplier = 1.0f;
    MarketMultiplierAtStart = 1.0f;
    RouteTierAtStart = 1;
    CargoUnitsReserved = 0;
    CargoCommodityAtStart = TEXT("ANIMAL FEED");
    CargoPriorityAtStart = TEXT("HILL FARM DIRECT");
    bPoliceIncidentDuringRun = false;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    return true;
}

FString AGTTFarmJobDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTFarmJobStage::ReachPickup:
            return FString::Printf(TEXT("FARM CARGO T%d | %s | FEED DEPOT | %s x%d | market x%.2f"),
                RouteTierAtStart, *CargoPriorityAtStart, *CargoCommodityAtStart, CargoUnitsReserved, MarketMultiplierAtStart);
        case EGTTFarmJobStage::DeliverCargo:
            return FString::Printf(TEXT("FARM CARGO | HILL FARM %s | %.0fs | cargo %.0f%%%s%s | %s"),
                RouteTierAtStart >= 2 ? TEXT("RELAY") : TEXT("DELIVERY"), TimeRemaining, CargoIntegrity * 100.0f,
                (LoadedMulebox.IsValid() || LoadedNativeMulebox.IsValid()) ? TEXT(" | MULEBOX LOADED") : TEXT(""),
                FleetPayoutMultiplier < 0.999f ? TEXT(" | PREP PENALTY") : TEXT(""), *CargoPriorityAtStart);
        case EGTTFarmJobStage::DeliverFinalStop:
            return FString::Printf(TEXT("FARM CARGO | NORTH WOOD YARD FINAL | %.0fs | cargo %.0f%% | market x%.2f%s | %s"),
                TimeRemaining, CargoIntegrity * 100.0f, MarketMultiplierAtStart,
                bPoliceIncidentDuringRun ? TEXT(" | POLICE INCIDENT") : TEXT(""), *CargoPriorityAtStart);
        default:
            return FString();
    }
}

void AGTTFarmJobDirector::FailJob(APawn* PlayerPawn, const FString& Reason)
{
    if (Stage == EGTTFarmJobStage::DeliverCargo || Stage == EGTTFarmJobStage::DeliverFinalStop)
    {
        if (GetWorld())
        {
            if (UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
            {
                Logistics->SettleCargoContract(CargoUnitsReserved, RouteTierAtStart >= 2, false);
                Logistics->RecordCargoFailure(CargoIntegrity, CargoIntegrity <= 0.02f || TimeRemaining <= 0.0f);
            }
        }
    }

    ClearLoadedVehicleCargoState();
    Stage = EGTTFarmJobStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    FleetPayoutMultiplier = 1.0f;
    MarketMultiplierAtStart = 1.0f;
    RouteTierAtStart = 1;
    CargoUnitsReserved = 0;
    CargoCommodityAtStart = TEXT("ANIMAL FEED");
    CargoPriorityAtStart = TEXT("HILL FARM DIRECT");
    bPoliceIncidentDuringRun = false;
    PushMessage(PlayerPawn, FString::Printf(TEXT("FARM JOB FAILED: %s Reputation/streak consequence saved. Reserved cargo is lost; buyer demand remains open and creates backlog pressure for future orders."), *Reason), 7.0f);
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
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
