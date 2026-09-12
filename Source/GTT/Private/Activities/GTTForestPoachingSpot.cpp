#include "Activities/GTTForestPoachingSpot.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTForestPoachingSpot::AGTTForestPoachingSpot()
{
    PrimaryActorTick.bCanEverTick = false;
    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    SetRootComponent(MarkerMesh);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MarkerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MarkerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    MarkerMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.35f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeFinder.Succeeded()) MarkerMesh->SetStaticMesh(ConeFinder.Object);
}

void AGTTForestPoachingSpot::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !GetWorld()) return;

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (!Economy || !GameMode) return;

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextAllowedAttemptTime)
    {
        Economy->PushMessage(TEXT("The forest has gone quiet. Move around before trying again."), 2.5f);
        return;
    }
    NextAllowedAttemptTime = Now + AttemptCooldownSeconds;
    GameMode->ReportWildlifeCrime(Pawn, WildlifeHeatPerAttempt);

    const float Roll = FMath::FRand();
    if (Roll < 0.32f)
    {
        Economy->PushMessage(TEXT("POACHING: nothing found, but the game warden noticed activity."), 4.0f);
        return;
    }

    FString Species;
    int32 Reward = 0;
    if (Roll < 0.67f)
    {
        Species = TEXT("forest hare");
        Reward = FMath::RandRange(45, 80);
    }
    else if (Roll < 0.92f)
    {
        Species = TEXT("wild boar");
        Reward = FMath::RandRange(110, 190);
    }
    else
    {
        Species = TEXT("red deer");
        Reward = FMath::RandRange(220, 340);
    }

    Economy->AddCash(Reward, FString::Printf(TEXT("Black-market buyer paid $%d for %s."), Reward, *Species));
    Economy->PushMessage(
        FString::Printf(TEXT("POACHING SUCCESS: %s | +$%d | WARDEN ALERT %d/3"), *Species, Reward, GameMode->GetWildlifeAlertLevel()),
        5.0f);
}

FText AGTTForestPoachingSpot::GetInteractionText_Implementation() const
{
    return NSLOCTEXT("GTT", "ForestPoaching", "Poach in the forest (illegal)");
}
