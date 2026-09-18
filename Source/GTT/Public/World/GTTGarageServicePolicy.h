#pragma once

#include "CoreMinimal.h"
#include "World/GTTGarageFleetSubsystem.h"

namespace GTTGarageServicePolicy
{
    /**
     * Hard dispatch hold used after a recovery tow or any equivalent immobile state.
     * LIMP/SERVICE remain advisory so a player may still drive a marginal vehicle to the shop.
     * TOW/IMMOBILE must be serviced first; this prevents a cheap garage recall from bypassing
     * the damage-preserving roadside recovery consequence.
     */
    inline bool RequiresWorkshopBeforeDispatch(const FGTTGarageFleetSnapshot& Snapshot)
    {
        return !Snapshot.bOccupied
            && (Snapshot.ServiceStatus == TEXT("TOW") || Snapshot.ServiceStatus == TEXT("IMMOBILE"));
    }

    inline FString BuildWorkshopHoldReason(const FGTTGarageFleetSnapshot& Snapshot)
    {
        const FString Estimate = Snapshot.RepairEstimate > 0
            ? FString::Printf(TEXT(" Workshop estimate $%d."), Snapshot.RepairEstimate)
            : FString(TEXT(" Workshop inspection required."));
        return FString::Printf(
            TEXT("%s is on WORKSHOP HOLD (%s). Service it before garage dispatch.%s"),
            *Snapshot.DisplayName,
            *Snapshot.ServiceStatus,
            *Estimate);
    }
}
