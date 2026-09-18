#include "World/GTTGarageFleetSubsystem.h"

#include "World/GTTGarageServicePolicy.h"

bool UGTTGarageFleetSubsystem::IsVehicleOnWorkshopHold(FName VehicleId) const
{
    if (VehicleId.IsNone()) return false;
    const TArray<FGTTGarageFleetSnapshot> Fleet = BuildFleetSnapshot(8);
    if (const FGTTGarageFleetSnapshot* Snapshot = Fleet.FindByPredicate([VehicleId](const FGTTGarageFleetSnapshot& Candidate)
        { return Candidate.VehicleId == VehicleId; }))
    {
        return GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(*Snapshot);
    }
    return false;
}

int32 UGTTGarageFleetSubsystem::GetWorkshopHoldCount(int32 MaxSlots) const
{
    const TArray<FGTTGarageFleetSnapshot> Fleet = BuildFleetSnapshot(FMath::Max(1, MaxSlots));
    return Fleet.CountByPredicate([](const FGTTGarageFleetSnapshot& Snapshot)
    {
        return GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot);
    });
}
