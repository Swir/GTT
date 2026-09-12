#include "World/GTTGarageSlotTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

AGTTGarageSlotTerminal::AGTTGarageSlotTerminal()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.75f));

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(34.0f);
    Label->SetTextRenderColor(FColor(100, 210, 255));
    Label->SetCastShadow(true);
}

void AGTTGarageSlotTerminal::BeginPlay()
{
    Super::BeginPlay();
    RefreshLabel();
}

void AGTTGarageSlotTerminal::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshClock += DeltaSeconds;
    if (RefreshClock >= 0.5f)
    {
        RefreshClock = 0.0f;
        RefreshLabel();
    }
}

void AGTTGarageSlotTerminal::SetSlotIndex(int32 InSlotIndex)
{
    SlotIndex = FMath::Max(0, InSlotIndex);
    RefreshLabel();
}

int32 AGTTGarageSlotTerminal::GetVehicleSortPriority(const AGTTVehicleBase* Vehicle) const
{
    if (!Vehicle) return 1000;
    const FName Id = Vehicle->GetPersistentVehicleId();
    if (Id == FName(TEXT("RustyFieldmaster60"))) return 0;
    if (Id == FName(TEXT("Rattleback82"))) return 10;
    if (Id == FName(TEXT("Mulebox1200"))) return 20;
    return 100 + GetTypeHash(Id) % 500;
}

AGTTVehicleBase* AGTTGarageSlotTerminal::ResolveSlotVehicle() const
{
    if (!GetWorld()) return nullptr;
    TArray<AGTTVehicleBase*> OwnedVehicles;
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (Vehicle && Vehicle->IsOwnedByPlayer() && !Vehicle->GetPersistentVehicleId().IsNone()) OwnedVehicles.Add(Vehicle);
    }
    OwnedVehicles.Sort([this](const AGTTVehicleBase& A, const AGTTVehicleBase& B)
    {
        const int32 PriorityA = GetVehicleSortPriority(&A);
        const int32 PriorityB = GetVehicleSortPriority(&B);
        if (PriorityA != PriorityB) return PriorityA < PriorityB;
        return A.GetPersistentVehicleId().ToString() < B.GetPersistentVehicleId().ToString();
    });
    return OwnedVehicles.IsValidIndex(SlotIndex) ? OwnedVehicles[SlotIndex] : nullptr;
}

void AGTTGarageSlotTerminal::RefreshLabel()
{
    if (!Label) return;
    const AGTTVehicleBase* Vehicle = ResolveSlotVehicle();
    const FString VehicleName = Vehicle ? Vehicle->GetVehicleDisplayName().ToString().ToUpper() : TEXT("EMPTY");
    Label->SetText(FText::FromString(FString::Printf(TEXT("GARAGE SLOT %d\n%s\nE - RECALL $%d"), SlotIndex + 1, *VehicleName, RecallServiceCost)));
}

void AGTTGarageSlotTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn) return;

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    if (!Economy) return;

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Pawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            Economy->PushMessage(TEXT("Garage recall locked while police are looking for you."), 4.0f);
            return;
        }
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            Economy->PushMessage(TEXT("Garage recall locked while the game warden is looking for you."), 4.0f);
            return;
        }
    }

    AGTTVehicleBase* Vehicle = ResolveSlotVehicle();
    if (!Vehicle)
    {
        Economy->PushMessage(FString::Printf(TEXT("GARAGE SLOT %d is empty."), SlotIndex + 1), 3.0f);
        return;
    }
    if (Vehicle->IsOccupied())
    {
        Economy->PushMessage(TEXT("Cannot recall a vehicle while someone is driving it."), 3.0f);
        return;
    }
    if (Economy->GetCash() < RecallServiceCost)
    {
        Economy->PushMessage(FString::Printf(TEXT("Recall service costs $%d."), RecallServiceCost), 3.0f);
        return;
    }

    const FVector BayLocation = GetActorLocation() + FVector(0.0f, -260.0f, 95.0f);
    const FTransform Destination(FRotator(0.0f, 90.0f, 0.0f), BayLocation);
    if (!Vehicle->RecallToTransform(Destination))
    {
        Economy->PushMessage(TEXT("That vehicle cannot be recalled right now."), 3.0f);
        return;
    }

    Economy->SpendCash(RecallServiceCost, FString::Printf(TEXT("Garage recall service: -$%d"), RecallServiceCost));
    Economy->PushMessage(FString::Printf(TEXT("SLOT %d RECALL: %s delivered to its bay."), SlotIndex + 1, *Vehicle->GetVehicleDisplayName().ToString()), 4.0f);
}

FText AGTTGarageSlotTerminal::GetInteractionText_Implementation() const
{
    const AGTTVehicleBase* Vehicle = ResolveSlotVehicle();
    if (!Vehicle)
    {
        return FText::Format(NSLOCTEXT("GTT", "GarageSlotEmpty", "Garage slot {0}: empty"), FText::AsNumber(SlotIndex + 1));
    }
    return FText::Format(NSLOCTEXT("GTT", "GarageSlotRecall", "Recall slot {0}: {1} (${2})"),
        FText::AsNumber(SlotIndex + 1), Vehicle->GetVehicleDisplayName(), FText::AsNumber(RecallServiceCost));
}
