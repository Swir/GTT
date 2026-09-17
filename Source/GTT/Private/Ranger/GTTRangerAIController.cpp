#include "Ranger/GTTRangerAIController.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "World/GTTRuralEconomySubsystem.h"

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
        if (GameMode->TryRangerCitation(Target))
        {
            StopMovement();

            if (UGTTRuralEconomySubsystem* RuralEconomy = GetWorld()->GetSubsystem<UGTTRuralEconomySubsystem>())
            {
                int32 ConfiscatedValue = 0;
                const int32 ConfiscatedUnits = RuralEconomy->ConfiscateContraband(ConfiscatedValue);
                if (ConfiscatedUnits > 0)
                {
                    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Target))
                    {
                        Economy->PushMessage(
                            FString::Printf(TEXT("WARDEN SEIZURE: %d contraband unit%s confiscated | fence value $%d lost."),
                                ConfiscatedUnits, ConfiscatedUnits == 1 ? TEXT("") : TEXT("s"), ConfiscatedValue),
                            5.5f);
                    }
                }
            }
        }
    }
}
