#include "Activities/GTTRecoveryTargetVehicle.h"

#include "Economy/GTTPlayerEconomyComponent.h"
#include "Core/GTTGameplayStatics.h"

AGTTRecoveryTargetVehicle::AGTTRecoveryTargetVehicle()
{
    VehicleDisplayName = FText::FromString(TEXT("Disabled Mulebox"));
    PersistentVehicleId = NAME_None;
    bIllegalToTake = false;
}

void AGTTRecoveryTargetVehicle::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn) return;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn))
    {
        Economy->PushMessage(TEXT("Engine is dead. Use the RECOVERY HOOK marker to attach a tow line."), 4.0f);
    }
}

FText AGTTRecoveryTargetVehicle::GetInteractionText_Implementation() const
{
    return FText::FromString(TEXT("Inspect disabled van"));
}
