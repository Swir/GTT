#include "World/GTTVehicleStyleWorldSubsystem.h"

#include "Engine/World.h"
#include "World/GTTVehicleStyleTerminal.h"

void UGTTVehicleStyleWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.GetNetMode() == NM_Client) return;

    if (AGTTVehicleStyleTerminal* TractorStyle = InWorld.SpawnActor<AGTTVehicleStyleTerminal>(FVector(350.0f, 2350.0f, 55.0f), FRotator::ZeroRotator))
        TractorStyle->SetServiceType(EGTTVehicleStyleService::TractorVisual);

    if (AGTTVehicleStyleTerminal* CarStyle = InWorld.SpawnActor<AGTTVehicleStyleTerminal>(FVector(650.0f, 2350.0f, 55.0f), FRotator::ZeroRotator))
        CarStyle->SetServiceType(EGTTVehicleStyleService::OldCarVariant);

    if (AGTTVehicleStyleTerminal* BodyShop = InWorld.SpawnActor<AGTTVehicleStyleTerminal>(FVector(950.0f, 2350.0f, 55.0f), FRotator::ZeroRotator))
        BodyShop->SetServiceType(EGTTVehicleStyleService::BodyPanels);
}
