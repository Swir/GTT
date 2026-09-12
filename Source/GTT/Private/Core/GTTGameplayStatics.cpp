#include "Core/GTTGameplayStatics.h"

#include "Economy/GTTPlayerEconomyComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

UGTTWantedComponent* UGTTGameplayStatics::FindWantedComponentForPawn(APawn* Pawn)
{
    if (!Pawn)
    {
        return nullptr;
    }

    if (UGTTWantedComponent* Wanted = Pawn->FindComponentByClass<UGTTWantedComponent>())
    {
        return Wanted;
    }

    if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Pawn))
    {
        if (APawn* Driver = Vehicle->GetDriverPawn())
        {
            return Driver->FindComponentByClass<UGTTWantedComponent>();
        }
    }

    return nullptr;
}

UGTTPlayerEconomyComponent* UGTTGameplayStatics::FindEconomyComponentForPawn(APawn* Pawn)
{
    if (!Pawn)
    {
        return nullptr;
    }

    if (UGTTPlayerEconomyComponent* Economy = Pawn->FindComponentByClass<UGTTPlayerEconomyComponent>())
    {
        return Economy;
    }

    if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Pawn))
    {
        if (APawn* Driver = Vehicle->GetDriverPawn())
        {
            return Driver->FindComponentByClass<UGTTPlayerEconomyComponent>();
        }
    }

    return nullptr;
}

int32 UGTTGameplayStatics::GetPlayerWantedLevel(const UObject* WorldContextObject, int32 PlayerIndex)
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, PlayerIndex);
    const UGTTWantedComponent* Wanted = FindWantedComponentForPawn(PlayerPawn);
    return Wanted ? Wanted->GetWantedLevel() : 0;
}
