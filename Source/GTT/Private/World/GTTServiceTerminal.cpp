#include "World/GTTServiceTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));

    AGTTFieldmasterNativePawn* FindActiveNativeFieldmaster(UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World)
        {
            return nullptr;
        }

        AGTTFieldmasterNativePawn* Best = nullptr;
        float BestDistanceSquared = FMath::Square(Radius);
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            AGTTFieldmasterNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive())
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(Origin, Native->GetActorLocation());
            if (DistanceSquared <= BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Best = Native;
            }
        }
        return Best;
    }

    AGTTRoadVehicleNativePawn* FindActiveNativeRoadVehicle(UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World)
        {
            return nullptr;
        }

        AGTTRoadVehicleNativePawn* Best = nullptr;
        float BestDistanceSquared = FMath::Square(Radius);
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            AGTTRoadVehicleNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive())
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(Origin, Native->GetActorLocation());
            if (DistanceSquared <= BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Best = Native;
            }
        }
        return Best;
    }

    bool HasActiveNativeRoadTakeover(UWorld* World, FName VehicleId)
    {
        if (!World || VehicleId.IsNone())
        {
            return false;
        }
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            const AGTTRoadVehicleNativePawn* Native = *It;
            if (IsValid(Native) && Native->IsLegacyTakeoverActive() && Native->GetPersistentVehicleId() == VehicleId)
            {
                return true;
            }
        }
        return false;
    }

    AGTTVehicleBase* FindFieldmasterMirror(UWorld* World, const AGTTFieldmasterNativePawn* Native)
    {
        if (!World || !Native)
        {
            return nullptr;
        }

        AGTTVehicleBase* Best = nullptr;
        float BestDistanceSquared = TNumericLimits<float>::Max();
        for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
        {
            AGTTVehicleBase* Vehicle = *It;
            if (!IsValid(Vehicle) || Vehicle->GetPersistentVehicleId() != FieldmasterVehicleId)
            {
                continue;
            }

            const float DistanceSquared = FVector::DistSquared(Native->GetActorLocation(), Vehicle->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Best = Vehicle;
            }
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
    if (CubeFinder.Succeeded())
    {
        TerminalMesh->SetStaticMesh(CubeFinder.Object);
    }
}

void AGTTServiceTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Economy)
    {
        return;
    }

    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        Economy->SellAllFish(FishPricePerKg);
        return;
    }

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        if (!NativeRoad->NeedsNativeWorkshopService())
        {
            Economy->PushMessage(TEXT("Workshop: that Native road vehicle is already ready to go."));
            return;
        }

        const int32 DamageSurcharge = NativeRoad->GetBodyDamageRepairSurcharge();
        const int32 TotalCost = WorkshopServiceCost + DamageSurcharge;
        if (!Economy->SpendCash(TotalCost, FString::Printf(TEXT("Native road workshop service - $%d"), TotalCost)))
        {
            return;
        }

        if (!NativeRoad->ApplyNativeWorkshopService())
        {
            Economy->AddCash(TotalCost, TEXT("Native road workshop rollback"));
            Economy->PushMessage(TEXT("Workshop: Native road state refresh failed; payment returned."));
            return;
        }

        Economy->PushMessage(
            FString::Printf(TEXT("%s repaired, panels restored, tires serviced and refuelled ($%d; body surcharge $%d)."),
                *NativeRoad->GetVehicleDisplayName().ToString(), TotalCost, DamageSurcharge),
            6.0f);
        return;
    }

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        AGTTVehicleBase* Mirror = FindFieldmasterMirror(GetWorld(), Native);
        if (!Mirror)
        {
            Economy->PushMessage(TEXT("Workshop: Native Fieldmaster compatibility mirror is unavailable."));
            return;
        }

        const FGTTVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
        const bool bNeedsRepair = State.ConditionPercent < 0.999f;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < Mirror->GetFuelCapacity();
        if (!bNeedsRepair && !bNeedsFuel)
        {
            Economy->PushMessage(TEXT("Workshop: that machine is already ready to go."));
            return;
        }

        if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost)))
        {
            return;
        }

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
        return;
    }

    AGTTVehicleBase* Vehicle = FindNearestVehicle();
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("Workshop: park a vehicle nearby first."));
        return;
    }

    const bool bNeedsRepair = Vehicle->GetConditionPercent() < 0.999f;
    const bool bNeedsFuel = Vehicle->GetFuelPercent() < 0.999f;
    if (!bNeedsRepair && !bNeedsFuel)
    {
        Economy->PushMessage(TEXT("Workshop: that machine is already ready to go."));
        return;
    }

    if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost)))
    {
        return;
    }

    Vehicle->RepairVehicle(100000.0f);
    Vehicle->RefuelVehicle(100000.0f);
    Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled."), *Vehicle->GetVehicleDisplayName().ToString()), 5.0f);
}

FText AGTTServiceTerminal::GetInteractionText_Implementation() const
{
    return ServiceType == EGTTServiceType::FishBuyer
        ? NSLOCTEXT("GTT", "SellFish", "Sell all fish")
        : NSLOCTEXT("GTT", "WorkshopService", "Repair + refuel nearby vehicle ($75 + damage parts)");
}

AGTTVehicleBase* AGTTServiceTerminal::FindNearestVehicle() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AGTTVehicleBase* BestVehicle = nullptr;
    float BestDistanceSquared = FMath::Square(VehicleSearchRadius);

    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!IsValid(Vehicle))
        {
            continue;
        }

        if (Vehicle->GetPersistentVehicleId() == FieldmasterVehicleId)
        {
            bool bNativeTakeoverActive = false;
            for (TActorIterator<AGTTFieldmasterNativePawn> NativeIt(GetWorld()); NativeIt; ++NativeIt)
            {
                if (IsValid(*NativeIt) && NativeIt->IsLegacyTakeoverActive())
                {
                    bNativeTakeoverActive = true;
                    break;
                }
            }
            if (bNativeTakeoverActive)
            {
                continue;
            }
        }

        if (HasActiveNativeRoadTakeover(GetWorld(), Vehicle->GetPersistentVehicleId()))
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestVehicle = Vehicle;
        }
    }

    return BestVehicle;
}
