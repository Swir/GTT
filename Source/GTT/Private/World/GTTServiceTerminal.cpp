#include "World/GTTServiceTerminal.h"

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
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageServicePolicy.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));

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

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        const FGTTRoadVehicleMigrationSnapshot State = NativeRoad->GetMigrationSnapshot();
        FGTTGarageFleetSnapshot FleetSnapshot;
        const bool bHasFleetSnapshot = ResolveFleetSnapshot(GetWorld(), NativeRoad->GetPersistentVehicleId(), FleetSnapshot);
        const bool bWorkshopHold = bHasFleetSnapshot && GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(FleetSnapshot);
        const bool bNeedsMechanical = NativeRoadNeedsMechanicalService(NativeRoad) || bWorkshopHold;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < NativeRoad->GetFuelCapacityLiters();

        if (!bNeedsMechanical && !bNeedsFuel)
        {
            Economy->PushMessage(TEXT("Workshop: that Native road vehicle is already ready to go."));
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
        const int32 TotalCost = GetNativeRoadRepairQuote(NativeRoad);
        if (!Economy->SpendCash(TotalCost, FString::Printf(TEXT("Native road workshop estimate - $%d"), TotalCost))) return;
        if (!NativeRoad->ApplyNativeWorkshopService())
        {
            Economy->AddCash(TotalCost, TEXT("Native road workshop rollback"));
            Economy->PushMessage(TEXT("Workshop: Native road state refresh failed; payment returned."));
            return;
        }

        if (bWorkshopHold)
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
        const FGTTVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
        const bool bNeedsRepair = State.ConditionPercent < 0.999f;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < Mirror->GetFuelCapacity();
        if (!bNeedsRepair && !bNeedsFuel) { Economy->PushMessage(TEXT("Workshop: that machine is already ready to go.")); return; }
        if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost))) return;
        Mirror->RepairVehicle(100000.0f);
        Mirror->RefuelVehicle(100000.0f);
        FString ImportSummary;
        if (!Native->ImportLegacyGameplayState(Mirror, ImportSummary))
        {
            Economy->AddCash(WorkshopServiceCost, TEXT("Workshop service rollback"));
            Economy->PushMessage(TEXT("Workshop: Native Fieldmaster state refresh failed; payment returned."));
            return;
        }
        Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled; Native Chaos state synchronized."), *Native->GetVehicleDisplayName().ToString()), 5.0f);
        if (GameMode) GameMode->SaveProgress();
        return;
    }

    AGTTVehicleBase* Vehicle = FindNearestVehicle();
    if (!Vehicle) { Economy->PushMessage(TEXT("Workshop: park a vehicle nearby first.")); return; }
    const bool bNeedsRepair = Vehicle->GetConditionPercent() < 0.999f;
    const bool bNeedsFuel = Vehicle->GetFuelPercent() < 0.999f;
    if (!bNeedsRepair && !bNeedsFuel) { Economy->PushMessage(TEXT("Workshop: that machine is already ready to go.")); return; }
    if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost))) return;
    Vehicle->RepairVehicle(100000.0f);
    Vehicle->RefuelVehicle(100000.0f);
    Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled."), *Vehicle->GetVehicleDisplayName().ToString()), 5.0f);
    if (GameMode) GameMode->SaveProgress();
}

FText AGTTServiceTerminal::GetInteractionText_Implementation() const
{
    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        return NSLOCTEXT("GTT", "SellFish", "Sell all fish");
    }

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        const FGTTRoadVehicleMigrationSnapshot State = NativeRoad->GetMigrationSnapshot();
        FGTTGarageFleetSnapshot FleetSnapshot;
        const bool bHasFleetSnapshot = ResolveFleetSnapshot(GetWorld(), NativeRoad->GetPersistentVehicleId(), FleetSnapshot);
        const bool bWorkshopHold = bHasFleetSnapshot && GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(FleetSnapshot);
        const bool bNeedsMechanical = NativeRoadNeedsMechanicalService(NativeRoad) || bWorkshopHold;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < NativeRoad->GetFuelCapacityLiters();
        if (!bNeedsMechanical && !bNeedsFuel)
            return FText::FromString(FString::Printf(TEXT("Workshop: %s is ready"), *NativeRoad->GetVehicleDisplayName().ToString()));
        if (!bNeedsMechanical && bNeedsFuel)
            return FText::FromString(FString::Printf(TEXT("Refuel %s ($%d exact fuel quote)"), *NativeRoad->GetVehicleDisplayName().ToString(), GetNativeRoadFuelQuote(NativeRoad)));
        if (bWorkshopHold)
            return FText::FromString(FString::Printf(TEXT("Complete recovery service: %s [%s] ($%d estimate)"),
                *NativeRoad->GetVehicleDisplayName().ToString(), *FleetSnapshot.ServiceStatus, GetNativeRoadRepairQuote(NativeRoad)));
        return FText::FromString(FString::Printf(TEXT("Workshop: repair + refuel %s ($%d estimate)"), *NativeRoad->GetVehicleDisplayName().ToString(), GetNativeRoadRepairQuote(NativeRoad)));
    }

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        return FText::FromString(FString::Printf(TEXT("Workshop: inspect %s ($%d base service)"), *Native->GetVehicleDisplayName().ToString(), WorkshopServiceCost));
    }

    return NSLOCTEXT("GTT", "WorkshopService", "Inspect + repair nearby vehicle (damage-based quote)");
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
