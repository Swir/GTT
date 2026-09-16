#include "World/GTTContractBoardTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GTTContractBoardSubsystem.h"

namespace
{
    const FName RoadRunJob(TEXT("RoadRun"));
}

AGTTContractBoardTerminal::AGTTContractBoardTerminal()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.62f, 0.18f, 0.85f));

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0.0f, -54.0f, 105.0f));
    Label->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(23.0f);
    Label->SetTextRenderColor(FColor(255, 218, 90));
    Label->SetCastShadow(true);
}

void AGTTContractBoardTerminal::BeginPlay()
{
    Super::BeginPlay();
    RefreshLabel();
}

void AGTTContractBoardTerminal::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshClock += DeltaSeconds;
    if (RefreshClock >= 0.5f)
    {
        RefreshClock = 0.0f;
        RefreshLabel();
    }
}

void AGTTContractBoardTerminal::Configure(FName InJobTag)
{
    JobTag = InJobTag;
    RefreshLabel();
}

void AGTTContractBoardTerminal::RefreshLabel()
{
    if (!Label || !GetWorld() || JobTag.IsNone()) return;
    const UGTTContractBoardSubsystem* Contracts = GetWorld()->GetSubsystem<UGTTContractBoardSubsystem>();
    if (!Contracts) return;

    const FGTTContractBoardOffer Offer = Contracts->BuildOffer(JobTag);
    const FString Role = UGTTGarageFleetSubsystem::FleetRoleLabel(Offer.RequiredRole);
    const FString Readiness = UGTTGarageFleetSubsystem::MissionReadinessLabel(Offer.Fleet.Readiness);
    const FString Vehicle = Offer.Fleet.AssignedVehicleName.IsEmpty() ? TEXT("NO LOADOUT") : Offer.Fleet.AssignedVehicleName.ToUpper();
    const FString Action = Offer.bCanAcceptNow ? TEXT("E - ACCEPT") : (Offer.bNeedsPreparation ? TEXT("E - PREP") : (!Offer.bScheduleOpen ? TEXT("CLOSED") : TEXT("UNAVAILABLE")));

    if (JobTag == RoadRunJob)
    {
        Label->SetText(FText::FromString(FString::Printf(
            TEXT("CONTRACT BOARD\n%s | %s\nPAY $%d-$%d\n%s | %s\nC%.0f F%.0f T%.0f B%.0f\nPREP $%d | NET MAX $%d\nREP %s %d | BONUS %d%% | %s\n%s"),
            *Offer.Title.ToUpper(), *Role,
            Offer.BaseReward, Offer.MaximumReward,
            *Vehicle, *Readiness,
            Offer.Fleet.ConditionPercent * 100.0f,
            Offer.Fleet.FuelPercent * 100.0f,
            Offer.Fleet.TireIntegrity * 100.0f,
            Offer.Fleet.BodyHealth * 100.0f,
            Offer.PreparationEstimate,
            Offer.MaximumNetReward,
            *Offer.LogisticsTier.ToUpper(), Offer.LogisticsReputation, Offer.PayoutBonusPercent, *Offer.ScheduleStatus,
            *Action)));
        return;
    }

    Label->SetText(FText::FromString(FString::Printf(
        TEXT("CONTRACT BOARD\n%s | %s\nPAY $%d-$%d\n%s | %s\nC%.0f F%.0f T%.0f B%.0f\nPREP $%d | NET MAX $%d\n%s"),
        *Offer.Title.ToUpper(), *Role,
        Offer.BaseReward, Offer.MaximumReward,
        *Vehicle, *Readiness,
        Offer.Fleet.ConditionPercent * 100.0f,
        Offer.Fleet.FuelPercent * 100.0f,
        Offer.Fleet.TireIntegrity * 100.0f,
        Offer.Fleet.BodyHealth * 100.0f,
        Offer.PreparationEstimate,
        Offer.MaximumNetReward,
        *Action)));
}

void AGTTContractBoardTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !GetWorld() || JobTag.IsNone()) return;

    UGTTContractBoardSubsystem* Contracts = GetWorld()->GetSubsystem<UGTTContractBoardSubsystem>();
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    if (!Contracts || !Economy) return;

    const FGTTContractBoardOffer Offer = Contracts->BuildOffer(JobTag);
    FString Summary;
    bool bSuccess = false;
    if (Offer.bCanAcceptNow)
    {
        bSuccess = Contracts->TryAcceptContract(Pawn, JobTag, Summary);
    }
    else if (Offer.bNeedsPreparation)
    {
        const FVector StageLocation = GetActorLocation() + FVector(0.0f, -420.0f, 80.0f);
        const FTransform StageTransform(FRotator(0.0f, 90.0f, 0.0f), StageLocation);
        bSuccess = Contracts->TryPrepareContract(Pawn, JobTag, StageTransform, Summary);
    }
    else if (!Offer.bScheduleOpen)
    {
        Summary = FString::Printf(TEXT("%s is %s. Your ROAD loadout can wait staged for the 06:00 opening."), *Offer.Title, *Offer.ScheduleStatus);
    }
    else
    {
        Summary = Offer.Fleet.Reason.IsEmpty() ? TEXT("Contract is currently unavailable.") : Offer.Fleet.Reason;
    }

    Economy->PushMessage(Summary.IsEmpty() ? (bSuccess ? TEXT("Contract board updated.") : TEXT("Contract board action failed.")) : Summary, bSuccess ? 6.0f : 4.5f);
    RefreshLabel();
}

FText AGTTContractBoardTerminal::GetInteractionText_Implementation() const
{
    if (!GetWorld() || JobTag.IsNone()) return FText::GetEmpty();
    const UGTTContractBoardSubsystem* Contracts = GetWorld()->GetSubsystem<UGTTContractBoardSubsystem>();
    if (!Contracts) return FText::GetEmpty();

    const FGTTContractBoardOffer Offer = Contracts->BuildOffer(JobTag);
    if (Offer.bCanAcceptNow)
    {
        return FText::FromString(FString::Printf(TEXT("Accept %s | $%d-$%d | %s ready"),
            *Offer.Title, Offer.BaseReward, Offer.MaximumReward, *Offer.Fleet.AssignedVehicleName));
    }
    if (Offer.bNeedsPreparation)
    {
        return FText::FromString(FString::Printf(TEXT("Prepare %s | %s | $%d | net max $%d"),
            *Offer.Title,
            *UGTTGarageFleetSubsystem::MissionReadinessLabel(Offer.Fleet.Readiness),
            Offer.PreparationEstimate,
            Offer.MaximumNetReward));
    }
    if (!Offer.bScheduleOpen)
    {
        return FText::FromString(FString::Printf(TEXT("%s | %s | REP %s %d"),
            *Offer.Title, *Offer.ScheduleStatus, *Offer.LogisticsTier, Offer.LogisticsReputation));
    }
    return FText::FromString(FString::Printf(TEXT("%s unavailable | %s"), *Offer.Title, *Offer.Fleet.Reason));
}
