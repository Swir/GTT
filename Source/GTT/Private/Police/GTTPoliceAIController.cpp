#include "Police/GTTPoliceAIController.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGTTPoliceAIController::AGTTPoliceAIController()
{
    bAttachToPawn = true;
}

void AGTTPoliceAIController::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(PursuitTimer, this, &AGTTPoliceAIController::UpdatePursuit, RepathInterval, true, 0.2f);
}

void AGTTPoliceAIController::UpdatePursuit()
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const int32 WantedLevel = UGTTGameplayStatics::GetPlayerWantedLevel(this, 0);
    if (!PlayerPawn || WantedLevel <= 0)
    {
        StopMovement();
        ClearFocus(EAIFocusPriority::Gameplay);
        return;
    }

    if (ACharacter* PoliceCharacter = Cast<ACharacter>(GetPawn()))
    {
        if (UCharacterMovementComponent* Movement = PoliceCharacter->GetCharacterMovement()) Movement->MaxWalkSpeed = BaseChaseSpeed + WantedLevel * SpeedPerWantedLevel;
    }

    const float Distance = GetPawn() ? FVector::Dist2D(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation()) : TNumericLimits<float>::Max();
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (Distance <= ArrestRadius && Now - LastArrestAttemptTime >= ArrestCooldown)
    {
        LastArrestAttemptTime = Now;
        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            if (GameMode->TryArrestPlayer(PlayerPawn))
            {
                StopMovement();
                return;
            }
        }
    }

    SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);
    MoveToActor(PlayerPawn, AcceptanceRadius, true, true, true, nullptr, true);
}
