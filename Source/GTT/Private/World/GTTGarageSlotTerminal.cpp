#include "World/GTTGarageSlotTerminal.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTGarageFleetSubsystem.h"

namespace
{
    AGTTFieldmasterNativePawn* ResolveActiveNativeFieldmaster(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            AGTTFieldmasterNativePawn* NativeFieldmaster = *It;
            if (NativeFieldmaster && NativeFieldmaster->IsLegacyTakeoverActive()) return NativeFieldmaster;
        }
        return nullptr;
    }
}

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
    Label->SetWorldSize(28.0f);
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

AGTTVehicleBase* AGTTGarageSlotTerminal::ResolveSlotVehicle() const
{
    if (!GetWorld()) return nullptr;
    if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
    {
        return Fleet->ResolveLegacyVehicleForSlot(SlotIndex);
    }
    return nullptr;
}

void AGTTGarageSlotTerminal::RefreshLabel()
{
    if (!Label || !GetWorld()) return;

    FGTTGarageFleetSnapshot Snapshot;
    const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>();
    if (!Fleet || !Fleet->GetSlotSnapshot(SlotIndex, Snapshot))
    {
        Label->SetText(FText::FromString(FString::Printf(TEXT("GARAGE SLOT %d\nEMPTY"), SlotIndex + 1)));
        return;
    }

    const FString NativeTag = Snapshot.bNativeAuthority ? TEXT(" [N]") : TEXT("");
    Label->SetText(FText::FromString(FString::Printf(
        TEXT("GARAGE %d\n%s%s | %s\nC %.0f  F %.0f  T %.0f  B %.0f\nE - RECALL $%d"),
        SlotIndex + 1,
        *Snapshot.DisplayName.ToUpper(),
        *NativeTag,
        *Snapshot.ServiceStatus,
        Snapshot.ConditionPercent * 100.0f,
        Snapshot.FuelPercent * 100.0f,
        Snapshot.TireIntegrity * 100.0f,
        Snapshot.BodyHealth * 100.0f,
        RecallServiceCost)));
}

void AGTTGarageSlotTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !GetWorld()) return;

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

    UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>();
    AGTTVehicleBase* Vehicle = Fleet ? Fleet->ResolveLegacyVehicleForSlot(SlotIndex) : nullptr;
    if (!Vehicle)
    {
        Economy->PushMessage(FString::Printf(TEXT("GARAGE SLOT %d is empty."), SlotIndex + 1), 3.0f);
        return;
    }

    FGTTGarageFleetSnapshot Snapshot;
    if (Fleet) Fleet->GetSlotSnapshot(SlotIndex, Snapshot);

    const FName VehicleId = Vehicle->GetPersistentVehicleId();
    const bool bNativeRoadSlot = VehicleId == FName(TEXT("Rattleback82")) || VehicleId == FName(TEXT("Mulebox1200"));
    AGTTRoadVehicleNativePawn* NativeRoad = (Fleet && bNativeRoadSlot) ? Fleet->FindActiveNativeRoadVehicle(VehicleId) : nullptr;
    AGTTFieldmasterNativePawn* NativeFieldmaster = nullptr;
    if (!NativeRoad && VehicleId == FName(TEXT("RustyFieldmaster60")))
    {
        NativeFieldmaster = ResolveActiveNativeFieldmaster(GetWorld());
    }

    if ((NativeRoad && NativeRoad->GetDriverPawn() != nullptr) || (NativeFieldmaster && NativeFieldmaster->IsOccupied()) || (!NativeRoad && !NativeFieldmaster && Vehicle->IsOccupied()))
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
    bool bRecalled = false;

    if (NativeRoad)
    {
        if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativeRoad->GetVehicleMovementComponent()))
        {
            Movement->SetThrottleInput(0.0f);
            Movement->SetSteeringInput(0.0f);
            Movement->SetBrakeInput(1.0f);
        }
        NativeRoad->SetActorTransform(Destination, false, nullptr, ETeleportType::TeleportPhysics);
        if (USkeletalMeshComponent* NativeMesh = NativeRoad->GetMesh())
        {
            NativeMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
            NativeMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
        NativeRoad->FlushNativePersistenceMirror();
        bRecalled = true;
    }
    else if (NativeFieldmaster)
    {
        bRecalled = NativeFieldmaster->RecallToTransform(Destination);
    }
    else
    {
        bRecalled = Vehicle->RecallToTransform(Destination);
    }

    if (!bRecalled)
    {
        Economy->PushMessage(TEXT("That vehicle cannot be recalled right now."), 3.0f);
        return;
    }

    if (!Economy->SpendCash(RecallServiceCost, FString::Printf(TEXT("Garage recall service: -$%d"), RecallServiceCost)))
    {
        return;
    }

    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->SaveProgress();
    }

    const FString DisplayName = Snapshot.DisplayName.IsEmpty() ? Vehicle->GetVehicleDisplayName().ToString() : Snapshot.DisplayName;
    FString ServiceHint;
    if (Snapshot.RepairEstimate > 0 && Snapshot.ServiceStatus != TEXT("READY"))
    {
        ServiceHint = FString::Printf(TEXT(" %s; workshop estimate $%d."), *Snapshot.ServiceStatus, Snapshot.RepairEstimate);
    }
    Economy->PushMessage(FString::Printf(TEXT("SLOT %d RECALL: %s delivered for $%d.%s Damage, fuel and tuning were preserved."),
        SlotIndex + 1, *DisplayName, RecallServiceCost, *ServiceHint), 6.0f);
}

FText AGTTGarageSlotTerminal::GetInteractionText_Implementation() const
{
    if (!GetWorld()) return FText::GetEmpty();
    FGTTGarageFleetSnapshot Snapshot;
    const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>();
    if (!Fleet || !Fleet->GetSlotSnapshot(SlotIndex, Snapshot))
    {
        return FText::Format(NSLOCTEXT("GTT", "GarageSlotEmptyFleet", "Garage slot {0}: empty"), FText::AsNumber(SlotIndex + 1));
    }

    return FText::FromString(FString::Printf(TEXT("Recall slot %d: %s [%s] ($%d)"),
        SlotIndex + 1, *Snapshot.DisplayName, *Snapshot.ServiceStatus, RecallServiceCost));
}
