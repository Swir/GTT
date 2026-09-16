#include "World/GTTGarageFleetSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
    constexpr int32 GarageWorkshopBaseCost = 75;
    constexpr int32 GarageTireServiceCost = 65;
    constexpr int32 GarageEngineBaseCost = 240;
    constexpr int32 GarageTireUpgradeBaseCost = 170;
    constexpr float GarageFuelPricePerLiter = 3.25f;

    const FName FieldmasterId(TEXT("RustyFieldmaster60"));
    const FName RattlebackId(TEXT("Rattleback82"));
    const FName MuleboxId(TEXT("Mulebox1200"));
    const FName FarmCargoJob(TEXT("FarmCargo"));
    const FName HeavyHaulJob(TEXT("HeavyHaul"));
    const FName RoadRunJob(TEXT("RoadRun"));

    float MinimumBodyHealth(const FGTTRoadBodyDamageSnapshot& Body)
    {
        return FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth));
    }

    FString BreakdownStatus(EGTTBreakdownRecommendation Recommendation)
    {
        switch (Recommendation)
        {
            case EGTTBreakdownRecommendation::LimpToWorkshop: return TEXT("LIMP");
            case EGTTBreakdownRecommendation::TowRecommended: return TEXT("TOW");
            case EGTTBreakdownRecommendation::Immobilized: return TEXT("IMMOBILE");
            default: return TEXT("READY");
        }
    }

    void PopulateServicePlan(FGTTGarageFleetSnapshot& Snapshot)
    {
        Snapshot.FuelEstimate = Snapshot.FuelCapacityLiters > 0.0f
            ? FMath::Max(0, FMath::CeilToInt(FMath::Max(0.0f, Snapshot.FuelCapacityLiters - Snapshot.FuelLiters) * GarageFuelPricePerLiter))
            : 0;
        Snapshot.TireServiceEstimate = Snapshot.TireIntegrity < 0.80f ? GarageTireServiceCost : 0;
        Snapshot.NextEngineUpgradeCost = Snapshot.EngineUpgradeLevel < 3 ? GarageEngineBaseCost * (Snapshot.EngineUpgradeLevel + 1) : 0;
        Snapshot.NextTireUpgradeCost = Snapshot.TireUpgradeLevel < 3 ? GarageTireUpgradeBaseCost * (Snapshot.TireUpgradeLevel + 1) : 0;

        if (Snapshot.bOccupied) Snapshot.NextServiceAction = TEXT("IN USE");
        else if (Snapshot.ServiceStatus == TEXT("IMMOBILE") || Snapshot.ServiceStatus == TEXT("TOW") || Snapshot.ServiceStatus == TEXT("LIMP") || Snapshot.ServiceStatus == TEXT("SERVICE")) Snapshot.NextServiceAction = TEXT("WORKSHOP");
        else if (Snapshot.TireServiceEstimate > 0) Snapshot.NextServiceAction = TEXT("TIRES");
        else if (Snapshot.FuelEstimate > 0) Snapshot.NextServiceAction = TEXT("REFUEL");
        else if (Snapshot.NextEngineUpgradeCost > 0) Snapshot.NextServiceAction = TEXT("ENGINE TUNE");
        else if (Snapshot.NextTireUpgradeCost > 0) Snapshot.NextServiceAction = TEXT("TIRE UPGRADE");
        else Snapshot.NextServiceAction = TEXT("COMPLETE");
    }
}

int32 UGTTGarageFleetSubsystem::GetVehicleSortPriority(const AGTTVehicleBase* Vehicle)
{
    if (!Vehicle) return 1000;
    const FName Id = Vehicle->GetPersistentVehicleId();
    if (Id == FieldmasterId) return 0;
    if (Id == RattlebackId) return 10;
    if (Id == MuleboxId) return 20;
    return 100 + static_cast<int32>(GetTypeHash(Id) % 500);
}

EGTTGarageFleetRole UGTTGarageFleetSubsystem::ClassifyVehicleRole(FName VehicleId)
{
    if (VehicleId == FieldmasterId) return EGTTGarageFleetRole::Tractor;
    if (VehicleId == RattlebackId) return EGTTGarageFleetRole::Road;
    if (VehicleId == MuleboxId) return EGTTGarageFleetRole::Cargo;
    return EGTTGarageFleetRole::Utility;
}

FString UGTTGarageFleetSubsystem::FleetRoleLabel(EGTTGarageFleetRole Role)
{
    switch (Role)
    {
        case EGTTGarageFleetRole::Tractor: return TEXT("TRACTOR");
        case EGTTGarageFleetRole::Road: return TEXT("ROAD");
        case EGTTGarageFleetRole::Cargo: return TEXT("CARGO");
        default: return TEXT("UTILITY");
    }
}

FName UGTTGarageFleetSubsystem::RecommendedVehicleForJob(FName JobTag)
{
    if (JobTag == FarmCargoJob) return MuleboxId;
    if (JobTag == HeavyHaulJob) return FieldmasterId;
    if (JobTag == RoadRunJob) return RattlebackId;
    return NAME_None;
}

TArray<AGTTVehicleBase*> UGTTGarageFleetSubsystem::GatherOwnedVehicles() const
{
    TArray<AGTTVehicleBase*> Vehicles;
    if (!GetWorld()) return Vehicles;

    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (IsValid(Vehicle) && Vehicle->IsOwnedByPlayer() && !Vehicle->GetPersistentVehicleId().IsNone())
        {
            Vehicles.Add(Vehicle);
        }
    }

    Vehicles.Sort([](const AGTTVehicleBase& A, const AGTTVehicleBase& B)
    {
        const int32 PriorityA = UGTTGarageFleetSubsystem::GetVehicleSortPriority(&A);
        const int32 PriorityB = UGTTGarageFleetSubsystem::GetVehicleSortPriority(&B);
        if (PriorityA != PriorityB) return PriorityA < PriorityB;
        return A.GetPersistentVehicleId().ToString() < B.GetPersistentVehicleId().ToString();
    });
    return Vehicles;
}

bool UGTTGarageFleetSubsystem::IsOwnedFleetVehicle(FName VehicleId) const
{
    if (VehicleId.IsNone()) return false;
    const TArray<AGTTVehicleBase*> Vehicles = GatherOwnedVehicles();
    return Vehicles.ContainsByPredicate([VehicleId](const AGTTVehicleBase* Vehicle)
    {
        return Vehicle && Vehicle->GetPersistentVehicleId() == VehicleId;
    });
}

bool UGTTGarageFleetSubsystem::SetPreferredVehicleId(FName VehicleId)
{
    if (!IsOwnedFleetVehicle(VehicleId)) return false;
    PreferredVehicleId = VehicleId;
    return true;
}

void UGTTGarageFleetSubsystem::RestorePreferredVehicleId(FName VehicleId)
{
    if (IsOwnedFleetVehicle(VehicleId))
    {
        PreferredVehicleId = VehicleId;
        return;
    }

    const TArray<AGTTVehicleBase*> Vehicles = GatherOwnedVehicles();
    PreferredVehicleId = Vehicles.Num() > 0 && Vehicles[0] ? Vehicles[0]->GetPersistentVehicleId() : NAME_None;
}

bool UGTTGarageFleetSubsystem::IsPreferredVehicleFitForJob(FName JobTag) const
{
    const FName Recommended = RecommendedVehicleForJob(JobTag);
    return !Recommended.IsNone() && PreferredVehicleId == Recommended;
}

FString UGTTGarageFleetSubsystem::BuildJobDispatchHint(FName JobTag) const
{
    const FName Recommended = RecommendedVehicleForJob(JobTag);
    if (Recommended.IsNone()) return TEXT("FLEET: use a healthy vehicle suited to the contract.");

    const TArray<FGTTGarageFleetSnapshot> Fleet = BuildFleetSnapshot(8);
    const FGTTGarageFleetSnapshot* RecommendedSnapshot = Fleet.FindByPredicate([Recommended](const FGTTGarageFleetSnapshot& Snapshot)
    {
        return Snapshot.VehicleId == Recommended;
    });

    if (!RecommendedSnapshot)
    {
        return FString::Printf(TEXT("FLEET GAP: recommended %s is not registered in your garage."), *Recommended.ToString());
    }

    const FString Role = FleetRoleLabel(RecommendedSnapshot->Role);
    if (RecommendedSnapshot->bPreferredDispatch)
    {
        return FString::Printf(TEXT("FLEET READY: %s [%s] is your active dispatch vehicle | %s."),
            *RecommendedSnapshot->DisplayName, *Role, *RecommendedSnapshot->ServiceStatus);
    }

    return FString::Printf(TEXT("FLEET TIP: dispatch slot %d %s [%s] for this job | %s | recall sets it active."),
        RecommendedSnapshot->SlotIndex + 1,
        *RecommendedSnapshot->DisplayName,
        *Role,
        *RecommendedSnapshot->ServiceStatus);
}

AGTTVehicleBase* UGTTGarageFleetSubsystem::ResolveLegacyVehicleForSlot(int32 SlotIndex) const
{
    if (SlotIndex < 0) return nullptr;
    const TArray<AGTTVehicleBase*> Vehicles = GatherOwnedVehicles();
    return Vehicles.IsValidIndex(SlotIndex) ? Vehicles[SlotIndex] : nullptr;
}

AGTTRoadVehicleNativePawn* UGTTGarageFleetSubsystem::FindActiveNativeRoadVehicle(FName VehicleId) const
{
    if (!GetWorld() || VehicleId.IsNone()) return nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(GetWorld()); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Native = *It;
        if (IsValid(Native) && Native->IsLegacyTakeoverActive() && Native->GetPersistentVehicleId() == VehicleId)
        {
            return Native;
        }
    }
    return nullptr;
}

TArray<FGTTGarageFleetSnapshot> UGTTGarageFleetSubsystem::BuildFleetSnapshot(int32 MaxSlots) const
{
    TArray<FGTTGarageFleetSnapshot> Result;
    if (!GetWorld() || MaxSlots <= 0) return Result;

    const TArray<AGTTVehicleBase*> Vehicles = GatherOwnedVehicles();
    const int32 Count = FMath::Min(MaxSlots, Vehicles.Num());
    Result.Reserve(Count);

    const UGTTBreakdownDecisionSubsystem* Breakdown = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();

    for (int32 Index = 0; Index < Count; ++Index)
    {
        AGTTVehicleBase* Vehicle = Vehicles[Index];
        if (!Vehicle) continue;

        FGTTGarageFleetSnapshot Snapshot;
        Snapshot.SlotIndex = Index;
        Snapshot.VehicleId = Vehicle->GetPersistentVehicleId();
        Snapshot.DisplayName = Vehicle->GetVehicleDisplayName().ToString();
        Snapshot.Role = ClassifyVehicleRole(Snapshot.VehicleId);
        Snapshot.bPreferredDispatch = Snapshot.VehicleId == PreferredVehicleId;
        Snapshot.ConditionPercent = FMath::Clamp(Vehicle->GetConditionPercent(), 0.0f, 1.0f);
        Snapshot.FuelCapacityLiters = FMath::Max(0.0f, Vehicle->GetFuelCapacity());
        Snapshot.FuelLiters = FMath::Clamp(Vehicle->GetFuelLiters(), 0.0f, Snapshot.FuelCapacityLiters);
        Snapshot.FuelPercent = Snapshot.FuelCapacityLiters > KINDA_SMALL_NUMBER ? Snapshot.FuelLiters / Snapshot.FuelCapacityLiters : 1.0f;
        Snapshot.TireIntegrity = FMath::Clamp(Vehicle->GetTireIntegrity(), 0.0f, 1.0f);
        Snapshot.EngineUpgradeLevel = Vehicle->GetEngineUpgradeLevel();
        Snapshot.TireUpgradeLevel = Vehicle->GetTireUpgradeLevel();
        Snapshot.bOccupied = Vehicle->IsOccupied();

        if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(Snapshot.VehicleId))
        {
            const FGTTRoadVehicleMigrationSnapshot State = NativeRoad->GetMigrationSnapshot();
            const FGTTRoadBodyDamageSnapshot Body = NativeRoad->GetBodyDamageSnapshot();

            Snapshot.DisplayName = NativeRoad->GetVehicleDisplayName().ToString();
            Snapshot.ConditionPercent = FMath::Clamp(State.ConditionPercent, 0.0f, 1.0f);
            Snapshot.FuelCapacityLiters = NativeRoad->GetFuelCapacityLiters();
            Snapshot.FuelLiters = FMath::Clamp(State.FuelLiters, 0.0f, Snapshot.FuelCapacityLiters);
            Snapshot.FuelPercent = Snapshot.FuelCapacityLiters > KINDA_SMALL_NUMBER ? Snapshot.FuelLiters / Snapshot.FuelCapacityLiters : 1.0f;
            Snapshot.TireIntegrity = FMath::Clamp(State.TireIntegrity, 0.0f, 1.0f);
            Snapshot.BodyHealth = MinimumBodyHealth(Body);
            Snapshot.EngineUpgradeLevel = State.EngineUpgradeLevel;
            Snapshot.TireUpgradeLevel = State.TireUpgradeLevel;
            Snapshot.bNativeAuthority = true;
            Snapshot.bOccupied = NativeRoad->GetDriverPawn() != nullptr;

            if (Breakdown)
            {
                const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeRoad, GarageWorkshopBaseCost);
                Snapshot.ServiceStatus = Snapshot.bOccupied ? TEXT("IN USE") : BreakdownStatus(Assessment.Recommendation);
                Snapshot.RepairEstimate = NativeRoad->NeedsNativeWorkshopService() ? Assessment.RepairEstimate : 0;
                Snapshot.TowEstimate = Assessment.TowEstimate;
            }
        }
        else if (Snapshot.VehicleId == FieldmasterId)
        {
            for (TActorIterator<AGTTFieldmasterNativePawn> It(GetWorld()); It; ++It)
            {
                AGTTFieldmasterNativePawn* Native = *It;
                if (!IsValid(Native) || !Native->IsLegacyTakeoverActive()) continue;
                const FGTTVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
                Snapshot.DisplayName = Native->GetVehicleDisplayName().ToString();
                Snapshot.ConditionPercent = FMath::Clamp(State.ConditionPercent, 0.0f, 1.0f);
                Snapshot.FuelCapacityLiters = FMath::Max(0.0f, Vehicle->GetFuelCapacity());
                Snapshot.FuelLiters = FMath::Clamp(State.FuelLiters, 0.0f, Snapshot.FuelCapacityLiters);
                Snapshot.FuelPercent = Snapshot.FuelCapacityLiters > KINDA_SMALL_NUMBER ? Snapshot.FuelLiters / Snapshot.FuelCapacityLiters : 1.0f;
                Snapshot.TireIntegrity = FMath::Clamp(State.TireIntegrity, 0.0f, 1.0f);
                Snapshot.EngineUpgradeLevel = State.EngineUpgradeLevel;
                Snapshot.TireUpgradeLevel = State.TireUpgradeLevel;
                Snapshot.bNativeAuthority = true;
                Snapshot.bOccupied = Native->IsOccupied();
                break;
            }
        }

        if (!Snapshot.bOccupied && Snapshot.ServiceStatus == TEXT("READY"))
        {
            if (Snapshot.ConditionPercent <= 0.05f || Snapshot.FuelPercent <= 0.01f || Snapshot.TireIntegrity <= 0.08f)
                Snapshot.ServiceStatus = TEXT("IMMOBILE");
            else if (Snapshot.ConditionPercent <= 0.35f || Snapshot.TireIntegrity <= 0.30f || Snapshot.BodyHealth <= 0.35f)
                Snapshot.ServiceStatus = TEXT("SERVICE");
        }
        else if (Snapshot.bOccupied)
        {
            Snapshot.ServiceStatus = TEXT("IN USE");
        }

        PopulateServicePlan(Snapshot);
        Result.Add(Snapshot);
    }

    return Result;
}

bool UGTTGarageFleetSubsystem::GetSlotSnapshot(int32 SlotIndex, FGTTGarageFleetSnapshot& OutSnapshot) const
{
    if (SlotIndex < 0) return false;
    const TArray<FGTTGarageFleetSnapshot> Fleet = BuildFleetSnapshot(SlotIndex + 1);
    if (!Fleet.IsValidIndex(SlotIndex)) return false;
    OutSnapshot = Fleet[SlotIndex];
    return true;
}

FString UGTTGarageFleetSubsystem::BuildFleetSummary(int32 MaxSlots) const
{
    const TArray<FGTTGarageFleetSnapshot> Fleet = BuildFleetSnapshot(MaxSlots);
    FString Summary = FString::Printf(TEXT("GARAGE FLEET %d/%d"), Fleet.Num(), FMath::Max(1, MaxSlots));
    if (Fleet.Num() == 0) return Summary + TEXT(" | no registered vehicles");

    if (!PreferredVehicleId.IsNone())
    {
        if (const FGTTGarageFleetSnapshot* Preferred = Fleet.FindByPredicate([this](const FGTTGarageFleetSnapshot& Snapshot)
            { return Snapshot.VehicleId == PreferredVehicleId; }))
        {
            Summary += FString::Printf(TEXT(" | ACTIVE %s [%s]"), *Preferred->DisplayName, *FleetRoleLabel(Preferred->Role));
        }
    }

    for (const FGTTGarageFleetSnapshot& Vehicle : Fleet)
    {
        const FString NativeTag = Vehicle.bNativeAuthority ? TEXT(" N") : TEXT("");
        const FString ActiveTag = Vehicle.bPreferredDispatch ? TEXT(" ACTIVE") : TEXT("");
        FString Costs;
        if (Vehicle.RepairEstimate > 0) Costs += FString::Printf(TEXT(" repair~$%d"), Vehicle.RepairEstimate);
        if (Vehicle.FuelEstimate > 0) Costs += FString::Printf(TEXT(" fuel~$%d"), Vehicle.FuelEstimate);
        if (Vehicle.TireServiceEstimate > 0) Costs += FString::Printf(TEXT(" tires$%d"), Vehicle.TireServiceEstimate);
        if (Vehicle.NextEngineUpgradeCost > 0) Costs += FString::Printf(TEXT(" eng$%d"), Vehicle.NextEngineUpgradeCost);
        if (Vehicle.NextTireUpgradeCost > 0) Costs += FString::Printf(TEXT(" grip$%d"), Vehicle.NextTireUpgradeCost);
        if (Vehicle.TowEstimate > 0 && (Vehicle.ServiceStatus == TEXT("TOW") || Vehicle.ServiceStatus == TEXT("IMMOBILE")))
            Costs += FString::Printf(TEXT(" tow~$%d"), Vehicle.TowEstimate);

        Summary += FString::Printf(TEXT("\n%d %s%s | %s%s | %s | C%.0f F%.0f T%.0f B%.0f | E%d/3 G%d/3 | NEXT %s%s"),
            Vehicle.SlotIndex + 1,
            *Vehicle.DisplayName,
            *NativeTag,
            *FleetRoleLabel(Vehicle.Role),
            *ActiveTag,
            *Vehicle.ServiceStatus,
            Vehicle.ConditionPercent * 100.0f,
            Vehicle.FuelPercent * 100.0f,
            Vehicle.TireIntegrity * 100.0f,
            Vehicle.BodyHealth * 100.0f,
            Vehicle.EngineUpgradeLevel,
            Vehicle.TireUpgradeLevel,
            *Vehicle.NextServiceAction,
            *Costs);
    }
    return Summary;
}
