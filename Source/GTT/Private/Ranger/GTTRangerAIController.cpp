#include "Ranger/GTTRangerAIController.h"

#include "Core/GTTGameMode.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGTTRangerAIController::AGTTRangerAIController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGTTRangerAIController::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(PursuitTimer, this, &AGTTRangerAIController::UpdatePursuit, RepathInterval, true, 0.2f);
}

void AGTTRangerAIController::UpdatePursuit()
{
    AGTTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGTTGameMode>() : nullptr;
    APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0);
    APawn* RangerPawn = GetPawn();
    if (!GameMode || !Target || !RangerPawn || GameMode->GetWildlifeAlertLevel() <= 0)
    {
        StopMovement();
        return;
    }

    if (ACharacter* RangerCharacter = Cast<ACharacter>(RangerPawn))
    {
        RangerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseChaseSpeed + GameMode->GetWildlifeAlertLevel() * SpeedPerAlertLevel;
    }

    MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);

    if (FVector::DistSquared2D(RangerPawn->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(CitationRadius))
    {
        GameMode->TryRangerCitation(Target);
    }
}
