#include "World/GTTServiceTerminal.h"

#include "GTT.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageServicePolicy.h"
#include "World/GTTWorkshopHoursPolicy.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));

    const AGTTDayNightCycle* FindDayNightCycle(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It)
        {
            if (IsValid(*It)) return *It;
        }
        return nullptr;
    }

    AGTTFieldmasterNativePawn* FindActiveNativeFieldmaster(UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World) return nullptr;
        AGTTFieldmasterNativePawn* Best = nullptr;
        float BestDistanceSquared = FMath::Square(Radius);
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            AGTTFieldmasterNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive()) continue;
            const float DistanceSquared = FVector::DistSquared(Origin, Native->GetActorLocation());
            if (DistanceSquared <= BestDistanceSquared) { BestDistanceSquared = DistanceSquared; Best = Native; }
        }
        return Best;
    }

    AGTTRoadVehicleNativePawn* FindActiveNativeRoadVehicle(UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World) return nullptr;
        AGTTRoadVehicleNativePawn* Best = nullptr;
        float BestDistanceSquared = FMath::Square(Radius);
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            AGTTRoadVehicleNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive()) continue;
            const float DistanceSquared = FVector::DistSquared(Origin, Native->GetActorLocation());
            if (DistanceSquared <= BestDistanceSquared) { BestDistanceSquared = DistanceSquared; Best = Native; }
        }
        return Best;
    }

    bool HasActiveNativeRoadTakeover(UWorld* World, FName VehicleId)
    {
        if (!World || VehicleId.IsNone()) return false;
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            const AGTTRoadVehicleNativePawn* Native = *It;
            if (IsValid(Native) && Native->IsLegacyTakeoverActive() && Native->GetPersistentVehicleId() == VehicleId) return true;
        }
        return false;
    }

    bool NativeRoadNeedsMechanicalService(const AGTTRoadVehicleNativePawn* Vehicle)
    {
        if (!Vehicle) return false;
        const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
        const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
        const bool bBodyDamaged = Body.FrontHealth < 0.999f || Body.RearHealth < 0.999f ||
            Body.LeftHealth < 0.999f || Body.RightHealth < 0.999f || Body.CoolingStress > 0.01f || Body.DetachedPanelCount > 0;
        return State.ConditionPercent < 0.999f || State.TireIntegrity < 0.999f || bBodyDamaged;
    }

    bool ResolveFleetSnapshot(UWorld* World, FName VehicleId, FGTTGarageFleetSnapshot& OutSnapshot)
    {
        if (!World || VehicleId.IsNone()) return false;
        const UGTTGarageFleetSubsystem* Fleet = World->GetSubsystem<UGTTGarageFleetSubsystem>();
        if (!Fleet) return false;
        const TArray<FGTTGarageFleetSnapshot> Vehicles = Fleet->BuildFleetSnapshot(8);
        if (const FGTTGarageFleetSnapshot* Found = Vehicles.FindByPredicate([VehicleId](const FGTTGarageFleetSnapshot& Candidate)
            { return Candidate.VehicleId == VehicleId; }))
        {
            OutSnapshot = *Found;
            return true;
        }
        return false;
    }

    bool IsWorkshopHold(UWorld* World, FName VehicleId, FGTTGarageFleetSnapshot* OutSnapshot = nullptr)
    {
        FGTTGarageFleetSnapshot Snapshot;
        if (!ResolveFleetSnapshot(World, VehicleId, Snapshot)) return false;
        if (OutSnapshot) *OutSnapshot = Snapshot;
        return GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot);
    }

    bool ResolveQueuedWorkshopAppointment(UWorld* World, FName VehicleId, FGTTWorkshopRepairQueueSnapshot& OutSnapshot)
    {
        if (!World || VehicleId.IsNone()) return false;
        const UGTTWorkshopRepairQueueSubsystem* Queue = World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>();
        if (!Queue || !Queue->HasQueuedRepairForVehicle(VehicleId)) return false;

        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        if (const FGTTWorkshopRepairQueueSnapshot* Found = Snapshots.FindByPredicate(
            [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Candidate)
            {
                return Candidate.PersistentVehicleId == VehicleId;
            }))
        {
            OutSnapshot = *Found;
            return true;
        }
        return false;
    }

    AGTTVehicleBase* FindFieldmasterMirror(UWorld* World, const AGTTFieldmasterNativePawn* Native)
    {
        if (!World || !Native) return nullptr;
        AGTTVehicleBase* Best = nullptr;
        float BestDistanceSquared = TNumericLimits<float>::Max();
        for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
        {
            AGTTVehicleBase* Vehicle = *It;
            if (!IsValid(Vehicle) || Vehicle->GetPersistentVehicleId() != FieldmasterVehicleId) continue;
            const float DistanceSquared = FVector::DistSquared(Native->GetActorLocation(), Vehicle->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared) { BestDistanceSquared = DistanceSquared; Best = Vehicle; }
        }
        return Best;
    }
}

AGTTServiceTerminal::AGTTServiceTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
    SetRootComponent(TerminalMesh);
    TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TerminalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TerminalMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.9f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) TerminalMesh->SetStaticMesh(CubeFinder.Object);
}

bool AGTTServiceTerminal::IsWorkshopOpenNow() const
{
    const AGTTDayNightCycle* Clock = FindDayNightCycle(GetWorld());
    // Fail open in test/minimal maps that intentionally do not spawn the world clock.
    return !Clock || GTTWorkshopHoursPolicy::IsOpen(Clock->GetTimeOfDayHours());
}

FText AGTTServiceTerminal::GetWorkshopStatusText() const
{
    const AGTTDayNightCycle* Clock = FindDayNightCycle(GetWorld());
    const FString Schedule = GTTWorkshopHoursPolicy::GetScheduleText();
    if (!Clock)
    {
        return FText::FromString(FString::Printf(TEXT("Workshop OPEN | hours %s | world clock unavailable: fail-open"), *Schedule));
    }

    if (GTTWorkshopHoursPolicy::IsOpen(Clock->GetTimeOfDayHours()))
    {
        return FText::FromString(FString::Printf(TEXT("Workshop OPEN | %s | hours %s"), *Clock->GetClockText(), *Schedule));
    }

    return FText::FromString(FString::Printf(
        TEXT("Workshop CLOSED | %s | hours %s | WORKSHOP HOLD emergency service +%d%%"),
        *Clock->GetClockText(), *Schedule, GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent));
}

int32 AGTTServiceTerminal::GetNativeRoadRepairQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !GetWorld()) return WorkshopServiceCost;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    return Decision ? Decision->CalculateRepairEstimate(Vehicle, WorkshopServiceCost) : WorkshopServiceCost + Vehicle->GetBodyDamageRepairSurcharge();
}

int32 AGTTServiceTerminal::GetNativeRoadFuelQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const float MissingLiters = FMath::Max(0.0f, Vehicle->GetFuelCapacityLiters() - State.FuelLiters);
    return MissingLiters <= KINDA_SMALL_NUMBER ? 0 : FMath::Max(1, FMath::CeilToInt(MissingLiters * NativeFuelPricePerLiter));
}

int32 AGTTServiceTerminal::GetNativeRoadCheckoutQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    const int32 BaseQuote = GetNativeRoadRepairQuote(Vehicle);
    if (!Vehicle || IsWorkshopOpenNow()) return BaseQuote;
    return IsWorkshopHold(GetWorld(), Vehicle->GetPersistentVehicleId())
        ? GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(BaseQuote)
        : BaseQuote;
}

void AGTTServiceTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Economy) return;

    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        Economy->SellAllFish(FishPricePerKg);
        return;
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const bool bWorkshopOpen = IsWorkshopOpenNow();
    const FString WorkshopSchedule = GTTWorkshopHoursPolicy::GetScheduleText();

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        const FGTTRoadVehicleMigrationSnapshot State = NativeRoad->GetMigrationSnapshot();
        FGTTGarageFleetSnapshot FleetSnapshot;
        const bool bWorkshopHold = IsWorkshopHold(GetWorld(), NativeRoad->GetPersistentVehicleId(), &FleetSnapshot);
        const bool bNeedsMechanical = NativeRoadNeedsMechanicalService(NativeRoad) || bWorkshopHold;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < NativeRoad->GetFuelCapacityLiters();
        const bool bAfterHoursEmergency = bWorkshopHold && !bWorkshopOpen;

        FGTTWorkshopRepairQueueSnapshot QueueSnapshot;
        if (!bWorkshopHold && ResolveQueuedWorkshopAppointment(GetWorld(), NativeRoad->GetPersistentVehicleId(), QueueSnapshot))
        {
            const FString QueueState = QueueSnapshot.State.IsEmpty() ? TEXT("QUEUED") : QueueSnapshot.State;
            const FString Timing = QueueSnapshot.bCheckedIn
                ? FString::Printf(TEXT("service ETA %.1f h"), QueueSnapshot.HoursUntilServiceComplete)
                : FString::Printf(TEXT("appointment day %d %s | ETA %.1f h"), QueueSnapshot.ReadyDay,
                    *GTTWorkshopHoursPolicy::FormatHour(QueueSnapshot.ReadyHour), QueueSnapshot.HoursUntilReady);
            Economy->PushMessage(FString::Printf(
                TEXT("Workshop appointment %s: %s | locked $%d | %s. Queue lifecycle owns this exact vehicle; direct walk-up repair/refuel is blocked so the locked quote, timer and no-precharge contract cannot be bypassed."),
                *QueueState, *NativeRoad->GetVehicleDisplayName().ToString(), QueueSnapshot.LockedQuote, *Timing), 8.0f);
            UE_LOG(LogGTT, Display,
                TEXT("WORKSHOP_QUEUE_TERMINAL_GUARD vehicle=%s state=%s locked_quote=%d queue_authority=YES direct_service=BLOCKED hard_hold=NO"),
                *QueueSnapshot.PersistentVehicleId.ToString(), *QueueState, QueueSnapshot.LockedQuote);
            return;
        }

        if (!bNeedsMechanical && !bNeedsFuel)
        {
            Economy->PushMessage(TEXT("Workshop: that Native road vehicle is already ready to go."));
            return;
        }

        if (!bWorkshopOpen && !bWorkshopHold)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("Workshop CLOSED (%s). Ordinary repair/refuel waits for opening; hard WORKSHOP HOLD recovery remains available after hours."),
                *WorkshopSchedule), 6.0f);
            return;
        }

        // Fuel-only visits use a dedicated per-litre quote instead of charging a full damage-service fee.
        // A hard workshop hold always goes through mechanical service so garage recall cannot clear it cheaply.
        if (!bNeedsMechanical && bNeedsFuel)
        {
            const int32 FuelCost = GetNativeRoadFuelQuote(NativeRoad);
            const float MissingLiters = FMath::Max(0.0f, NativeRoad->GetFuelCapacityLiters() - State.FuelLiters);
            if (!Economy->SpendCash(FuelCost, FString::Printf(TEXT("%s fuel - $%d"), *NativeRoad->GetVehicleDisplayName().ToString(), FuelCost))) return;
            const float Added = NativeRoad->RefuelNativeVehicle(MissingLiters);
            if (Added <= KINDA_SMALL_NUMBER)
            {
                Economy->AddCash(FuelCost, TEXT("Native road refuel rollback"));
                Economy->PushMessage(TEXT("Workshop: refuel could not be applied; payment returned."));
                return;
            }
            Economy->PushMessage(FString::Printf(TEXT("%s refuelled %.1f L for $%d. Mechanical state was not changed."), *NativeRoad->GetVehicleDisplayName().ToString(), Added, FuelCost), 5.0f);
            if (GameMode) GameMode->SaveProgress();
            return;
        }

        const int32 BodyParts = NativeRoad->GetBodyDamageRepairSurcharge();
        const int32 BaseCost = GetNativeRoadRepairQuote(NativeRoad);
        const int32 TotalCost = bAfterHoursEmergency
            ? GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(BaseCost)
            : BaseCost;
        if (!Economy->SpendCash(TotalCost, FString::Printf(TEXT("Native road workshop estimate - $%d"), TotalCost))) return;
        if (!NativeRoad->ApplyNativeWorkshopService())
        {
            Economy->AddCash(TotalCost, TEXT("Native road workshop rollback"));
            Economy->PushMessage(TEXT("Workshop: Native road state refresh failed; payment returned."));
            return;
        }

        if (bAfterHoursEmergency)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("%s after-hours recovery complete for $%d (daytime base $%d, emergency +%d%%). WORKSHOP HOLD cleared; garage dispatch is available again."),
                *NativeRoad->GetVehicleDisplayName().ToString(), TotalCost, BaseCost,
                GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent), 8.0f);
        }
        else if (bWorkshopHold)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("%s recovery service complete for $%d (structural parts $%d). WORKSHOP HOLD cleared; garage dispatch is available again."),
                *NativeRoad->GetVehicleDisplayName().ToString(), TotalCost, BodyParts), 7.0f);
        }
        else
        {
            Economy->PushMessage(FString::Printf(TEXT("%s repaired + refuelled for $%d (structural parts $%d)."), *NativeRoad->GetVehicleDisplayName().ToString(), TotalCost, BodyParts), 6.0f);
        }
        if (GameMode) GameMode->SaveProgress();
        return;
    }

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        AGTTVehicleBase* Mirror = FindFieldmasterMirror(GetWorld(), Native);
        if (!Mirror) { Economy->PushMessage(TEXT("Workshop: Native Fieldmaster compatibility mirror is unavailable.")); return; }
        const FGTTRoadVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
        const bool bWorkshopHold = IsWorkshopHold(GetWorld(), FieldmasterVehicleId);
        const bool bNeedsRepair = State.ConditionPercent < 0.999f || bWorkshopHold;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < Mirror->GetFuelCapacity();
        if (!bNeedsRepair && !bNeedsFuel) { Economy->PushMessage(TEXT("Workshop: that machine is already ready to go.")); return; }
        if (!bWorkshopOpen && !bWorkshopHold)
        {
            Economy->PushMessage(FString::Printf(TEXT("Workshop CLOSED (%s). Fieldmaster service resumes at opening."), *WorkshopSchedule), 5.0f);
            return;
        }
        const bool bAfterHoursEmergency = bWorkshopHold && !bWorkshopOpen;
        const int32 TotalCost = bAfterHoursEmergency
            ? GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(WorkshopServiceCost)
            : WorkshopServiceCost;
        if (!Economy->SpendCash(TotalCost, FString::Printf(TEXT("Workshop service - $%d"), TotalCost))) return;
        Mirror->RepairVehicle(100000.0f);
        Mirror->RefuelVehicle(100000.0f);
        FString ImportSummary;
        if (!Native->ImportLegacyGameplayState(Mirror, ImportSummary))
        {
            Economy->AddCash(TotalCost, TEXT("Workshop service rollback"));
            Economy->PushMessage(TEXT("Workshop: Native Fieldmaster state refresh failed; payment returned."));
            return;
        }
        if (bAfterHoursEmergency)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("%s emergency recovery service complete for $%d (+%d%% after-hours); Native Chaos state synchronized."),
                *Native->GetVehicleDisplayName().ToString(), TotalCost,
                GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent), 6.0f);
        }
        else
        {
            Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled for $%d; Native Chaos state synchronized."),
                *Native->GetVehicleDisplayName().ToString(), TotalCost), 5.0f);
        }
        if (GameMode) GameMode->SaveProgress();
        return;
    }

    AGTTVehicleBase* Vehicle = FindNearestVehicle();
    if (!Vehicle) { Economy->PushMessage(TEXT("Workshop: park a vehicle nearby first.")); return; }
    const bool bWorkshopHold = IsWorkshopHold(GetWorld(), Vehicle->GetPersistentVehicleId());
    const bool bNeedsRepair = Vehicle->GetConditionPercent() < 0.999f || bWorkshopHold;
    const bool bNeedsFuel = Vehicle->GetFuelPercent() < 0.999f;
    if (!bNeedsRepair && !bNeedsFuel) { Economy->PushMessage(TEXT("Workshop: that machine is already ready to go.")); return; }
    if (!bWorkshopOpen && !bWorkshopHold)
    {
        Economy->PushMessage(FString::Printf(TEXT("Workshop CLOSED (%s). Ordinary service resumes at opening."), *WorkshopSchedule), 5.0f);
        return;
    }
    const bool bAfterHoursEmergency = bWorkshopHold && !bWorkshopOpen;
    const int32 TotalCost = bAfterHoursEmergency
        ? GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(WorkshopServiceCost)
        : WorkshopServiceCost;
    if (!Economy->SpendCash(TotalCost, FString::Printf(TEXT("Workshop service - $%d"), TotalCost))) return;
    Vehicle->RepairVehicle(100000.0f);
    Vehicle->RefuelVehicle(100000.0f);
    if (bAfterHoursEmergency)
    {
        Economy->PushMessage(FString::Printf(TEXT("%s emergency recovery service complete for $%d (+%d%% after-hours)."),
            *Vehicle->GetVehicleDisplayName().ToString(), TotalCost,
            GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent), 6.0f);
    }
    else
    {
        Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled for $%d."),
            *Vehicle->GetVehicleDisplayName().ToString(), TotalCost), 5.0f);
    }
    if (GameMode) GameMode->SaveProgress();
}

FText AGTTServiceTerminal::GetInteractionText_Implementation() const
{
    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        return NSLOCTEXT("GTT", "SellFish", "Sell all fish");
    }

    const bool bWorkshopOpen = IsWorkshopOpenNow();
    const FString WorkshopSchedule = GTTWorkshopHoursPolicy::GetScheduleText();

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        const FGTTRoadVehicleMigrationSnapshot State = NativeRoad->GetMigrationSnapshot();
        FGTTGarageFleetSnapshot FleetSnapshot;
        const bool bWorkshopHold = IsWorkshopHold(GetWorld(), NativeRoad->GetPersistentVehicleId(), &FleetSnapshot);
        const bool bNeedsMechanical = NativeRoadNeedsMechanicalService(NativeRoad) || bWorkshopHold;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < NativeRoad->GetFuelCapacityLiters();

        FGTTWorkshopRepairQueueSnapshot QueueSnapshot;
        if (!bWorkshopHold && ResolveQueuedWorkshopAppointment(GetWorld(), NativeRoad->GetPersistentVehicleId(), QueueSnapshot))
        {
            const FString QueueState = QueueSnapshot.State.IsEmpty() ? TEXT("QUEUED") : QueueSnapshot.State;
            if (QueueSnapshot.bCheckedIn)
            {
                return FText::FromString(FString::Printf(
                    TEXT("Workshop appointment %s: %s | locked $%d | service ETA %.1f h | queue-managed"),
                    *QueueState, *NativeRoad->GetVehicleDisplayName().ToString(), QueueSnapshot.LockedQuote,
                    QueueSnapshot.HoursUntilServiceComplete));
            }
            return FText::FromString(FString::Printf(
                TEXT("Workshop appointment %s: %s | locked $%d | day %d %s | queue-managed"),
                *QueueState, *NativeRoad->GetVehicleDisplayName().ToString(), QueueSnapshot.LockedQuote,
                QueueSnapshot.ReadyDay, *GTTWorkshopHoursPolicy::FormatHour(QueueSnapshot.ReadyHour)));
        }

        if (!bNeedsMechanical && !bNeedsFuel)
            return FText::FromString(FString::Printf(TEXT("Workshop: %s is ready"), *NativeRoad->GetVehicleDisplayName().ToString()));
        if (!bWorkshopOpen && !bWorkshopHold)
            return FText::FromString(FString::Printf(TEXT("Workshop CLOSED (%s): %s waits for daytime service"), *WorkshopSchedule, *NativeRoad->GetVehicleDisplayName().ToString()));
        if (!bNeedsMechanical && bNeedsFuel)
            return FText::FromString(FString::Printf(TEXT("Refuel %s ($%d exact fuel quote)"), *NativeRoad->GetVehicleDisplayName().ToString(), GetNativeRoadFuelQuote(NativeRoad)));
        if (bWorkshopHold && !bWorkshopOpen)
            return FText::FromString(FString::Printf(TEXT("Emergency recovery: %s [%s] ($%d incl. +%d%% after-hours)"),
                *NativeRoad->GetVehicleDisplayName().ToString(), *FleetSnapshot.ServiceStatus,
                GetNativeRoadCheckoutQuote(NativeRoad), GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent));
        if (bWorkshopHold)
            return FText::FromString(FString::Printf(TEXT("Complete recovery service: %s [%s] ($%d estimate)"),
                *NativeRoad->GetVehicleDisplayName().ToString(), *FleetSnapshot.ServiceStatus, GetNativeRoadRepairQuote(NativeRoad)));
        return FText::FromString(FString::Printf(TEXT("Workshop: repair + refuel %s ($%d estimate)"), *NativeRoad->GetVehicleDisplayName().ToString(), GetNativeRoadRepairQuote(NativeRoad)));
    }

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        const bool bWorkshopHold = IsWorkshopHold(GetWorld(), FieldmasterVehicleId);
        if (!bWorkshopOpen && !bWorkshopHold)
            return FText::FromString(FString::Printf(TEXT("Workshop CLOSED (%s): Fieldmaster service waits for opening"), *WorkshopSchedule));
        const int32 Quote = bWorkshopHold && !bWorkshopOpen
            ? GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(WorkshopServiceCost)
            : WorkshopServiceCost;
        return FText::FromString(FString::Printf(TEXT("Workshop: inspect %s ($%d%s)"),
            *Native->GetVehicleDisplayName().ToString(), Quote,
            bWorkshopHold && !bWorkshopOpen ? TEXT(" emergency recovery") : TEXT(" base service")));
    }

    if (AGTTVehicleBase* Vehicle = FindNearestVehicle())
    {
        const bool bWorkshopHold = IsWorkshopHold(GetWorld(), Vehicle->GetPersistentVehicleId());
        if (!bWorkshopOpen && !bWorkshopHold)
            return FText::FromString(FString::Printf(TEXT("Workshop CLOSED (%s): ordinary service waits for opening"), *WorkshopSchedule));
        const int32 Quote = bWorkshopHold && !bWorkshopOpen
            ? GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(WorkshopServiceCost)
            : WorkshopServiceCost;
        return FText::FromString(FString::Printf(TEXT("Inspect + repair %s ($%d%s)"),
            *Vehicle->GetVehicleDisplayName().ToString(), Quote,
            bWorkshopHold && !bWorkshopOpen ? TEXT(" emergency recovery") : TEXT("")));
    }

    if (bWorkshopOpen)
    {
        return FText::FromString(TEXT("Workshop OPEN | inspect + repair nearby vehicle"));
    }
    return FText::FromString(FString::Printf(TEXT("Workshop CLOSED (%s) | hard WORKSHOP HOLD emergency service only"), *WorkshopSchedule));
}

AGTTVehicleBase* AGTTServiceTerminal::FindNearestVehicle() const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* BestVehicle = nullptr;
    float BestDistanceSquared = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!IsValid(Vehicle)) continue;
        if (Vehicle->GetPersistentVehicleId() == FieldmasterVehicleId)
        {
            bool bNativeTakeoverActive = false;
            for (TActorIterator<AGTTFieldmasterNativePawn> NativeIt(GetWorld()); NativeIt; ++NativeIt)
            {
                if (IsValid(*NativeIt) && NativeIt->IsLegacyTakeoverActive()) { bNativeTakeoverActive = true; break; }
            }
            if (bNativeTakeoverActive) continue;
        }
        if (HasActiveNativeRoadTakeover(GetWorld(), Vehicle->GetPersistentVehicleId())) continue;
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared) { BestDistanceSquared = DistanceSquared; BestVehicle = Vehicle; }
    }
    return BestVehicle;
}
