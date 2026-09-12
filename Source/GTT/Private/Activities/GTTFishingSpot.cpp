#include "Activities/GTTFishingSpot.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTFishingSpot::AGTTFishingSpot()
{
    PrimaryActorTick.bCanEverTick = false;

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    SetRootComponent(MarkerMesh);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MarkerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MarkerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    MarkerMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.18f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderFinder.Succeeded())
    {
        MarkerMesh->SetStaticMesh(CylinderFinder.Object);
    }
}

void AGTTFishingSpot::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !GetWorld())
    {
        return;
    }

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    if (!Economy)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextAllowedCastTime)
    {
        Economy->PushMessage(TEXT("Give the water a moment before casting again."), 2.0f);
        return;
    }

    NextAllowedCastTime = Now + CastCooldownSeconds;

    if (bRestrictedFishing)
    {
        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->ReportWildlifeCrime(Pawn, RestrictedFishingHeat);
        }
    }

    const float Roll = FMath::FRand();
    if (Roll < 0.22f)
    {
        Economy->PushMessage(bRestrictedFishing
            ? TEXT("Illegal cast... no bite, but the game warden noticed activity near the lake.")
            : TEXT("No bite this time."));
        return;
    }

    FString Species;
    float WeightKg = 0.0f;

    if (Roll < 0.58f)
    {
        Species = TEXT("River Perch");
        WeightKg = FMath::FRandRange(0.18f, 0.85f);
    }
    else if (Roll < 0.88f)
    {
        Species = TEXT("Village Carp");
        WeightKg = FMath::FRandRange(0.75f, 2.8f);
    }
    else
    {
        Species = TEXT("Old Pike");
        WeightKg = FMath::FRandRange(2.2f, 6.5f);
    }

    Economy->AddFish(WeightKg, Species);
}

FText AGTTFishingSpot::GetInteractionText_Implementation() const
{
    return bRestrictedFishing
        ? NSLOCTEXT("GTT", "PoachFish", "Poach fish")
        : NSLOCTEXT("GTT", "FishHere", "Fish here");
}
