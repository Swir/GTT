#include "World/GTTGarageSlotTerminal.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageServicePolicy.h"

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
    const FString ActiveTag = Snapshot.bPreferredDispatch ? TEXT(" [ACTIVE]") : TEXT("");
    const FString LoadoutTag = Snapshot.bRoleLoadout ? TEXT(" [LOADOUT]") : TEXT("");
    const bool bWorkshopHold = GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot);
    const FString ActionLine = bWorkshopHold
        ? FString::Printf(TEXT("WORKSHOP HOLD ~$%d"), Snapshot.RepairEstimate)
        : FString::Printf(TEXT("E - DISPATCH $%d"), RecallServiceCost);
    Label->SetText(FText::FromString(FString::Printf(
        TEXT("GARAGE %d%s%s\n%s%s | %s | %s\nC %.0f  F %.0f  T %.0f  B %.0f\n%s"),
        SlotIndex + 1,
        *ActiveTag,
        *LoadoutTag,
        *Snapshot.DisplayName.ToUpper(),
        *NativeTag,
        *UGTTGarageFleetSubsystem::FleetRoleLabel(Snapshot.Role),
        *Snapshot.ServiceStatus,
        Snapshot.ConditionPercent * 100.0f,
        Snapshot.FuelPercent * 100.0f,
        Snapshot.TireIntegrity * 100.0f,
        Snapshot.BodyHealth * 100.0f,
        *ActionLine)));
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
            Economy->PushMessage(TEXT("Garage dispatch locked while police are looking for you."), 4.0f);
            return;
        }
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            Economy->PushMessage(TEXT("Garage dispatch locked while the game warden is looking for you."), 4.0f);
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
    if (!Fleet || !Fleet->GetSlotSnapshot(SlotIndex, Snapshot))
    {
        Economy->PushMessage(TEXT("Garage fleet snapshot unavailable; dispatch aborted without charge."), 4.0f);
        return;
    }

    // A damage-preserving tow ends at the workshop. A TOW/IMMOBILE fleet state is therefore a
    // real service consequence, not something that can be bypassed with the cheaper garage recall.
    // LIMP/SERVICE remain advisory so a marginal but mobile vehicle can still be deliberately used.
    if (GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot))
    {
        Economy->PushMessage(
            FString::Printf(TEXT("SLOT %d WORKSHOP HOLD: %s"), SlotIndex + 1, *GTTGarageServicePolicy::BuildWorkshopHoldReason(Snapshot)),
            7.0f);
        return;
    }

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
        Economy->PushMessage(TEXT("Cannot dispatch a vehicle while someone is driving it."), 3.0f);
        return;
    }
    if (Economy->GetCash() < RecallServiceCost)
    {
        Economy->PushMessage(FString::Printf(TEXT("Garage dispatch service costs $%d."), RecallServiceCost), 3.0f);
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
        Economy->PushMessage(TEXT("That vehicle cannot be dispatched right now."), 3.0f);
        return;
    }

    if (!Economy->SpendCash(RecallServiceCost, FString::Printf(TEXT("Garage dispatch service: -$%d"), RecallServiceCost)))
    {
        return;
    }

    if (!Fleet || !Fleet->SetPreferredVehicleId(VehicleId))
    {
        Economy->AddCash(RecallServiceCost, TEXT("Garage dispatch preference refund"));
        Economy->PushMessage(TEXT("Dispatch could not be committed to the fleet registry; payment refunded."), 4.0f);
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
    Economy->PushMessage(FString::Printf(TEXT("SLOT %d DISPATCH: %s delivered for $%d, set ACTIVE and saved as the %s mission loadout.%s Damage, fuel and tuning were preserved."),
        SlotIndex + 1, *DisplayName, RecallServiceCost, *UGTTGarageFleetSubsystem::FleetRoleLabel(Snapshot.Role), *ServiceHint), 6.0f);
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

    if (GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot))
    {
        return FText::FromString(FString::Printf(
            TEXT("Workshop hold slot %d: %s [%s] — service first (~$%d)"),
            SlotIndex + 1,
            *Snapshot.DisplayName,
            *Snapshot.ServiceStatus,
            Snapshot.RepairEstimate));
    }

    return FText::FromString(FString::Printf(TEXT("Dispatch slot %d: %s [%s%s%s] ($%d)"),
        SlotIndex + 1,
        *Snapshot.DisplayName,
        *UGTTGarageFleetSubsystem::FleetRoleLabel(Snapshot.Role),
        Snapshot.bPreferredDispatch ? TEXT(" ACTIVE") : TEXT(""),
        Snapshot.bRoleLoadout ? TEXT(" LOADOUT") : TEXT(""),
        RecallServiceCost));
}
