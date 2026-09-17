#include "Activities/GTTFarmJobTerminal.h"

#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "Activities/GTTFarmJobDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void PushCargoAuthorityFailure(APawn* PlayerPawn, const FString& FailureReason)
{
    if (!PlayerPawn || FailureReason.IsEmpty()) return;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->PushMessage(FString::Printf(TEXT("DELIVERY YARD: %s"), *FailureReason), 5.0f);
    }
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
    UWorld* World = GetWorld();
    UGTTFarmCargoAuthoritySubsystem* CargoAuthority = World ? World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>() : nullptr;
    if (!Pawn || !Director || !CargoAuthority) return;

    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            if (Director->GetStage() == EGTTFarmJobStage::Idle) CargoAuthority->ResetForNewContract();
            Director->TryStartJob(Pawn);
            break;

        case EGTTFarmJobTerminalType::Pickup:
            // Lock the exact actor before the director applies physical load state. If the
            // director rejects pickup for any reason, roll the lock back in the same action.
            if (!CargoAuthority->CaptureLoadedVehicle(Pawn))
            {
                PushCargoAuthorityFailure(Pawn, TEXT("park a working cargo vehicle at the Feed Depot before loading."));
                break;
            }
            if (!Director->TryPickupCargo(Pawn))
            {
                CargoAuthority->ClearLoadedVehicle(TEXT("pickup-rejected"));
            }
            break;

        case EGTTFarmJobTerminalType::Finish:
        {
            FString FailureReason;
            float SpeedKmh = 0.0f;
            float DistanceCm = 0.0f;
            if (!CargoAuthority->ValidateHandoff(GetActorLocation(), FailureReason, SpeedKmh, DistanceCm))
            {
                PushCargoAuthorityFailure(Pawn, FailureReason);
                break;
            }
            if (Director->TryCompleteJob(Pawn))
            {
                const bool bContractComplete = Director->GetStage() == EGTTFarmJobStage::Idle;
                CargoAuthority->MarkAcceptedHandoff(TEXT("HILL_FARM"), bContractComplete, SpeedKmh, DistanceCm);
                if (bContractComplete) CargoAuthority->ClearLoadedVehicle(TEXT("contract-complete"));
            }
            break;
        }

        case EGTTFarmJobTerminalType::FinalFinish:
        {
            FString FailureReason;
            float SpeedKmh = 0.0f;
            float DistanceCm = 0.0f;
            if (!CargoAuthority->ValidateHandoff(GetActorLocation(), FailureReason, SpeedKmh, DistanceCm))
            {
                PushCargoAuthorityFailure(Pawn, FailureReason);
                break;
            }
            if (Director->TryCompleteFinalStop(Pawn))
            {
                CargoAuthority->MarkAcceptedHandoff(TEXT("NORTH_WOOD_YARD"), true, SpeedKmh, DistanceCm);
                CargoAuthority->ClearLoadedVehicle(TEXT("contract-complete"));
            }
            break;
        }
    }
}

FText AGTTFarmJobTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            return NSLOCTEXT("GTT", "FarmJobStartV3", "Take rural cargo contract");
        case EGTTFarmJobTerminalType::Pickup:
            return NSLOCTEXT("GTT", "FarmJobPickupV3", "Load cargo into this vehicle");
        case EGTTFarmJobTerminalType::Finish:
            return NSLOCTEXT("GTT", "FarmJobFinishV5", "Stop the loaded vehicle and hand off cargo at Hill Farm");
        case EGTTFarmJobTerminalType::FinalFinish:
            return NSLOCTEXT("GTT", "FarmJobFinalFinishV3", "Stop the loaded vehicle and complete North Wood Yard handoff");
    }
    return FText::GetEmpty();
}
