#include "Police/GTTPolicePawn.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Police/GTTPoliceAIController.h"

AGTTPolicePawn::AGTTPolicePawn()
{
    AIControllerClass = AGTTPoliceAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    Tags.Add(TEXT("Police"));

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = 520.0f;
    }
}
