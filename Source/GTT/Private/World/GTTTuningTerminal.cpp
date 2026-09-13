#include "World/GTTTuningTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));

    AGTTFieldmasterNativePawn* FindActiveNativeFieldmaster(const UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World)
        {
            return nullptr;
        }

        AGTTFieldmasterNativePawn* Best = nullptr;
        float BestDistSq = FMath::Square(Radius);
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            AGTTFieldmasterNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive() || !Native->IsOwnedByPlayer())
            {
                continue;
            }
            const float DistSq = FVector::DistSquared2D(Origin, Native->GetActorLocation());
            if (DistSq <= BestDistSq)
            {
                BestDistSq = DistSq;
                Best = Native;
            }
        }
        return Best;
    }

    AGTTVehicleBase* FindFieldmasterMirror(const UWorld* World, const AGTTFieldmasterNativePawn* Native)
    {
        if (!World || !Native)
        {
            return nullptr;
        }

        AGTTVehicleBase* Best = nullptr;
        float BestDistSq = TNumericLimits<float>::Max();
        for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
        {
            AGTTVehicleBase* Vehicle = *It;
            if (!IsValid(Vehicle) || Vehicle->GetPersistentVehicleId() != FieldmasterVehicleId)
            {
                continue;
            }
            const float DistSq = FVector::DistSquared2D(Native->GetActorLocation(), Vehicle->GetActorLocation());
            if (DistSq < BestDistSq)
            {
                BestDistSq = DistSq;
                Best = Vehicle;
            }
        }
        return Best;
    }

    bool RefreshNativeFromMirror(AGTTFieldmasterNativePawn* Native, AGTTVehicleBase* Mirror, UGTTPlayerEconomyComponent* Economy)
    {
        FString Summary;
        if (Native && Mirror && Native->ImportLegacyGameplayState(Mirror, Summary))
        {
            return true;
        }
        if (Economy)
        {
            Economy->PushMessage(TEXT("TUNING: Native Fieldmaster state synchronization failed."), 4.0f);
        }
        return false;
    }
}

AGTTTuningTerminal::AGTTTuningTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.05f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
}

void AGTTTuningTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Pawn || !Economy) return;

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode) return;

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        AGTTVehicleBase* Mirror = FindFieldmasterMirror(GetWorld(), Native);
        if (!Mirror)
        {
            Economy->PushMessage(TEXT("TUNING: Native Fieldmaster compatibility mirror is unavailable."), 4.0f);
            return;
        }

        const FGTTVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
        if (State.TireIntegrity < 0.80f)
        {
            if (!Economy->SpendCash(TireRepairCost, FString::Printf(TEXT("Tire service - $%d"), TireRepairCost))) return;
            Mirror->RepairTires();
            if (!RefreshNativeFromMirror(Native, Mirror, Economy))
            {
                Economy->AddCash(TireRepairCost, TEXT("Tire service rollback"));
                return;
            }
            Economy->PushMessage(FString::Printf(TEXT("%s tires restored to 100%% and synchronized to Native Chaos."), *Native->GetVehicleDisplayName().ToString()), 4.0f);
            GameMode->SaveProgress();
            return;
        }

        if (State.EngineUpgradeLevel < 3)
        {
            const int32 NextLevel = State.EngineUpgradeLevel + 1;
            const int32 Cost = EngineBaseCost * NextLevel;
            if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Engine tune L%d - $%d"), NextLevel, Cost))) return;
            if (!Mirror->InstallEngineUpgrade() || !RefreshNativeFromMirror(Native, Mirror, Economy))
            {
                Economy->AddCash(Cost, TEXT("Engine tune rollback"));
                return;
            }
            Economy->PushMessage(FString::Printf(TEXT("ENGINE TUNE installed: L%d/3 on Native Fieldmaster."), NextLevel), 5.0f);
            GameMode->SaveProgress();
            return;
        }

        if (State.TireUpgradeLevel < 3)
        {
            const int32 NextLevel = State.TireUpgradeLevel + 1;
            const int32 Cost = TireBaseCost * NextLevel;
            if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Tire upgrade L%d - $%d"), NextLevel, Cost))) return;
            if (!Mirror->InstallTireUpgrade() || !RefreshNativeFromMirror(Native, Mirror, Economy))
            {
                Economy->AddCash(Cost, TEXT("Tire upgrade rollback"));
                return;
            }
            Economy->PushMessage(FString::Printf(TEXT("HEAVY-DUTY TIRES installed: L%d/3 on Native Fieldmaster."), NextLevel), 5.0f);
            GameMode->SaveProgress();
            return;
        }

        Economy->PushMessage(FString::Printf(TEXT("%s is fully tuned (engine L%d/3, tires L%d/3)."),
            *Native->GetVehicleDisplayName().ToString(), State.EngineUpgradeLevel, State.TireUpgradeLevel), 4.0f);
        return;
    }

    AGTTVehicleBase* Vehicle = FindNearestOwnedVehicle();
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("TUNING: park one of your owned vehicles nearby first."), 4.0f);
        return;
    }

    if (Vehicle->GetTireIntegrity() < 0.80f)
    {
        if (!Economy->SpendCash(TireRepairCost, FString::Printf(TEXT("Tire service - $%d"), TireRepairCost))) return;
        Vehicle->RepairTires();
        Economy->PushMessage(FString::Printf(TEXT("%s tires restored to 100%%."), *Vehicle->GetVehicleDisplayName().ToString()), 4.0f);
        GameMode->SaveProgress();
        return;
    }

    if (Vehicle->GetEngineUpgradeLevel() < 3)
    {
        const int32 NextLevel = Vehicle->GetEngineUpgradeLevel() + 1;
        const int32 Cost = EngineBaseCost * NextLevel;
        if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Engine tune L%d - $%d"), NextLevel, Cost))) return;
        Vehicle->InstallEngineUpgrade();
        Economy->PushMessage(FString::Printf(TEXT("ENGINE TUNE installed: L%d/3. More power, cooling and reliability."), NextLevel), 5.0f);
        GameMode->SaveProgress();
        return;
    }

    if (Vehicle->GetTireUpgradeLevel() < 3)
    {
        const int32 NextLevel = Vehicle->GetTireUpgradeLevel() + 1;
        const int32 Cost = TireBaseCost * NextLevel;
        if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Tire upgrade L%d - $%d"), NextLevel, Cost))) return;
        Vehicle->InstallTireUpgrade();
        Economy->PushMessage(FString::Printf(TEXT("HEAVY-DUTY TIRES installed: L%d/3. Better grip and impact resistance."), NextLevel), 5.0f);
        GameMode->SaveProgress();
        return;
    }

    Economy->PushMessage(FString::Printf(TEXT("%s is fully tuned. %s"), *Vehicle->GetVehicleDisplayName().ToString(), *Vehicle->GetTuningSummary()), 4.0f);
}

FText AGTTTuningTerminal::GetInteractionText_Implementation() const
{
    return NSLOCTEXT("GTT", "TuneOwnedVehicle", "Tune / service nearby owned vehicle");
}

AGTTVehicleBase* AGTTTuningTerminal::FindNearestOwnedVehicle() const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || !Vehicle->IsOwnedByPlayer()) continue;

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
            if (bNativeTakeoverActive) continue;
        }

        const float DistSq = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Vehicle;
        }
    }
    return Best;
}
