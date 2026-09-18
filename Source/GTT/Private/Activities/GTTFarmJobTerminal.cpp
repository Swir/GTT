#include "Activities/GTTFarmJobTerminal.h"

#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "Activities/GTTFarmJobDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float LegalHandoffMaxSpeedKmh = 3.0f;
constexpr float LegalHandoffVehicleRadiusCm = 750.0f;

void PushCargoAuthorityMessage(APawn* PlayerPawn, const FString& Message, float Duration = 4.5f)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->PushMessage(Message, Duration);
    }
}

void SaveCargoCheckpoint(const UObject* WorldContextObject)
{
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(WorldContextObject)))
    {
        GameMode->SaveProgress();
    }
}

bool ValidateBoundCargoHandoff(const UObject* WorldContextObject, APawn* PlayerPawn, const FVector& HandoffLocation)
{
    if (!WorldContextObject || !PlayerPawn) return false;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return false;

    UGTTFarmCargoAuthoritySubsystem* Authority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>();
    if (!Authority)
    {
        PushCargoAuthorityMessage(PlayerPawn, TEXT("DELIVERY YARD: cargo authority unavailable; handoff refused."));
        return false;
    }

    FString Reason;
    if (!Authority->ValidateHandoff(HandoffLocation, LegalHandoffVehicleRadiusCm, LegalHandoffMaxSpeedKmh, Reason))
    {
        PushCargoAuthorityMessage(PlayerPawn, Reason);
        return false;
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

    UGTTFarmCargoAuthoritySubsystem* Authority = GetWorld() ? GetWorld()->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>() : nullptr;

    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            if (Director->TryStartJob(Pawn))
            {
                if (Authority) Authority->ClearLoadedVehicle(TEXT("new-contract"));
                // Persist ReachPickup only after the real reservation and stage transition.
                SaveCargoCheckpoint(this);
            }
            break;
        case EGTTFarmJobTerminalType::Pickup:
            if (Director->TryPickupCargo(Pawn))
            {
                if (Authority)
                {
                    FString Summary;
                    if (Authority->BindLoadedVehicle(Pawn, Summary))
                    {
                        PushCargoAuthorityMessage(Pawn, Summary, 5.5f);
                        // This checkpoint stores both DeliverCargo and the exact physical vehicle ID.
                        SaveCargoCheckpoint(this);
                    }
                    else PushCargoAuthorityMessage(Pawn, Summary, 7.0f);
                }
                else
                {
                    PushCargoAuthorityMessage(Pawn, TEXT("CARGO AUTHORITY ERROR: load accepted without a physical vehicle lock."), 7.0f);
                }
            }
            break;
        case EGTTFarmJobTerminalType::Finish:
            if (ValidateBoundCargoHandoff(this, Pawn, GetActorLocation()) && Director->TryCompleteJob(Pawn))
            {
                if (Authority && Director->GetStage() == EGTTFarmJobStage::Idle)
                {
                    Authority->ClearLoadedVehicle(TEXT("direct-route-complete"));
                }
                else if (Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop)
                {
                    // Extended chains now survive a save/reload after the Hill Farm relay.
                    SaveCargoCheckpoint(this);
                }
            }
            break;
        case EGTTFarmJobTerminalType::FinalFinish:
            if (ValidateBoundCargoHandoff(this, Pawn, GetActorLocation()) && Director->TryCompleteFinalStop(Pawn))
            {
                if (Authority) Authority->ClearLoadedVehicle(TEXT("extended-route-complete"));
            }
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
            return NSLOCTEXT("GTT", "FarmJobPickupV3", "Load cargo and lock this vehicle to the contract");
        case EGTTFarmJobTerminalType::Finish:
            return NSLOCTEXT("GTT", "FarmJobFinishV5", "Stop the loaded cargo vehicle and hand off at Hill Farm");
        case EGTTFarmJobTerminalType::FinalFinish:
            return NSLOCTEXT("GTT", "FarmJobFinalFinishV3", "Stop the same loaded vehicle for North Wood Yard handoff");
    }
    return FText::GetEmpty();
}
