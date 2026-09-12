#include "World/GTTTuningTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"

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

    AGTTVehicleBase* Vehicle = FindNearestOwnedVehicle();
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("TUNING: park one of your owned vehicles nearby first."), 4.0f);
        return;
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode) return;

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
        const float DistSq = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Vehicle;
        }
    }
    return Best;
}
