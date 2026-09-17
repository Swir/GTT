#include "Activities/GTTFarmJobTerminal.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
constexpr float LegalHandoffMaxSpeedKmh = 3.0f;

bool IsControlledCargoVehicleMovingTooFast(const UObject* WorldContextObject, float& OutSpeedKmh)
{
    OutSpeedKmh = 0.0f;
    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
    const bool bVehicleControlled = Cast<AGTTVehicleBase>(ControlledPawn) != nullptr || Cast<AGTTRoadVehicleNativePawn>(ControlledPawn) != nullptr;
    if (!ControlledPawn || !bVehicleControlled) return false;

    OutSpeedKmh = ControlledPawn->GetVelocity().Size2D() * 0.036f;
    return OutSpeedKmh > LegalHandoffMaxSpeedKmh;
}

bool BlockUnsafeDriveByHandoff(const UObject* WorldContextObject, APawn* PlayerPawn)
{
    float SpeedKmh = 0.0f;
    if (!IsControlledCargoVehicleMovingTooFast(WorldContextObject, SpeedKmh)) return false;

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->PushMessage(FString::Printf(
            TEXT("DELIVERY YARD: stop the cargo vehicle before handoff (%.1f km/h; max %.1f)."),
            SpeedKmh, LegalHandoffMaxSpeedKmh), 4.5f);
    }
    return true;
}
}

AGTTFarmJobTerminal::AGTTFarmJobTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.2f));
}

void AGTTFarmJobTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTFarmJobDirector* Director = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(this, AGTTFarmJobDirector::StaticClass()));
    if (!Pawn || !Director) return;

    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            Director->TryStartJob(Pawn);
            break;
        case EGTTFarmJobTerminalType::Pickup:
            Director->TryPickupCargo(Pawn);
            break;
        case EGTTFarmJobTerminalType::Finish:
            if (!BlockUnsafeDriveByHandoff(this, Pawn)) Director->TryCompleteJob(Pawn);
            break;
        case EGTTFarmJobTerminalType::FinalFinish:
            if (!BlockUnsafeDriveByHandoff(this, Pawn)) Director->TryCompleteFinalStop(Pawn);
            break;
    }
}

FText AGTTFarmJobTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            return NSLOCTEXT("GTT", "FarmJobStartV3", "Take rural cargo contract");
        case EGTTFarmJobTerminalType::Pickup:
            return NSLOCTEXT("GTT", "FarmJobPickupV2", "Load feed cargo");
        case EGTTFarmJobTerminalType::Finish:
            return NSLOCTEXT("GTT", "FarmJobFinishV4", "Stop and hand off cargo at Hill Farm");
        case EGTTFarmJobTerminalType::FinalFinish:
            return NSLOCTEXT("GTT", "FarmJobFinalFinishV2", "Stop and complete North Wood Yard handoff");
    }
    return FText::GetEmpty();
}
