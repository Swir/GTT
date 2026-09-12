#include "Police/GTTPoliceAIController.h"

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

    GetWorldTimerManager().SetTimer(
        PursuitTimer,
        this,
        &AGTTPoliceAIController::UpdatePursuit,
        RepathInterval,
        true,
        0.2f);
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
        if (UCharacterMovementComponent* Movement = PoliceCharacter->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = BaseChaseSpeed + (WantedLevel * SpeedPerWantedLevel);
        }
    }

    SetFocus(PlayerPawn, EAIFocusPriority::Gameplay);
    MoveToActor(PlayerPawn, AcceptanceRadius, true, true, true, nullptr, true);
}
