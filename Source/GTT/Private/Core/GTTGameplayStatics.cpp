#include "Core/GTTGameplayStatics.h"

#include "Economy/GTTPlayerEconomyComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Radio/GTTRadioComponent.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

namespace
{
    APawn* ResolveGTTDriverPawn(APawn* Pawn)
    {
        if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Pawn))
        {
            return Vehicle->GetDriverPawn();
        }
        if (const AGTTFieldmasterNativePawn* NativeFieldmaster = Cast<AGTTFieldmasterNativePawn>(Pawn))
        {
            return NativeFieldmaster->GetDriverPawn();
        }
        return nullptr;
    }
}

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

    if (APawn* Driver = ResolveGTTDriverPawn(Pawn))
    {
        return Driver->FindComponentByClass<UGTTWantedComponent>();
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

    if (APawn* Driver = ResolveGTTDriverPawn(Pawn))
    {
        return Driver->FindComponentByClass<UGTTPlayerEconomyComponent>();
    }

    return nullptr;
}

UGTTRadioComponent* UGTTGameplayStatics::FindRadioComponentForPawn(APawn* Pawn)
{
    if (!Pawn)
    {
        return nullptr;
    }

    if (UGTTRadioComponent* Radio = Pawn->FindComponentByClass<UGTTRadioComponent>())
    {
        return Radio;
    }

    if (APawn* Driver = ResolveGTTDriverPawn(Pawn))
    {
        return Driver->FindComponentByClass<UGTTRadioComponent>();
    }

    return nullptr;
}

int32 UGTTGameplayStatics::GetPlayerWantedLevel(const UObject* WorldContextObject, int32 PlayerIndex)
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, PlayerIndex);
    const UGTTWantedComponent* Wanted = FindWantedComponentForPawn(PlayerPawn);
    return Wanted ? Wanted->GetWantedLevel() : 0;
}
