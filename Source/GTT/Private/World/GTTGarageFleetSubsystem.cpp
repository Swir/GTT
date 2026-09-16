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
    const FName TimberHaulJob(TEXT("TimberHaul"));
    const FName FieldMowingJob(TEXT("FieldMowing"));
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

    void GetJobThresholds(FName JobTag, float& OutCondition, float& OutFuel, float& OutTires, float& OutBody)
    {
        OutCondition = 0.40f;
        OutFuel = 0.20f;
        OutTires = 0.35f;
        OutBody = 0.45f;
        if (JobTag == HeavyHaulJob)
        {
            OutCondition = 0.55f;
            OutFuel = 0.30f;
            OutTires = 0.50f;
            OutBody = 0.55f;
        }
        else if (JobTag == TimberHaulJob)
        {
            OutCondition = 0.45f;
            OutFuel = 0.25f;
            OutTires = 0.40f;
            OutBody = 0.45f;
        }
        else if (JobTag == FieldMowingJob)
        {
            OutCondition = 0.45f;
            OutFuel = 0.20f;
            OutTires = 0.35f;
            OutBody = 0.40f;
        }
        else if (JobTag == RoadRunJob)
        {
            OutCondition = 0.55f;
            OutFuel = 0.35f;
            OutTires = 0.55f;
            OutBody = 0.60f;
        }
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

FString UGTTGarageFleetSubsystem::MissionReadinessLabel(EGTTFleetMissionReadiness Readiness)
{
    switch (Readiness)
    {
        case EGTTFleetMissionReadiness::Ready: return TEXT("READY");
        case EGTTFleetMissionReadiness::Advisory: return TEXT("CAUTION");
        case EGTTFleetMissionReadiness::ServiceRequired: return TEXT("SERVICE REQUIRED");
        default: return TEXT("UNAVAILABLE");
    }
}

FName UGTTGarageFleetSubsystem::RecommendedVehicleForJob(FName JobTag)
{
    if (JobTag == FarmCargoJob || JobTag == TimberHaulJob) return MuleboxId;
    if (JobTag == HeavyHaulJob || JobTag == FieldMowingJob) return FieldmasterId;
    if (JobTag == RoadRunJob) return RattlebackId;
    return NAME_None;
}

EGTTGarageFleetRole UGTTGarageFleetSubsystem::RecommendedRoleForJob(FName JobTag)
{
    if (JobTag == FarmCargoJob || JobTag == TimberHaulJob) return EGTTGarageFleetRole::Cargo;
    if (JobTag == HeavyHaulJob || JobTag == FieldMowingJob) return EGTTGarageFleetRole::Tractor;
    if (JobTag == RoadRunJob) return EGTTGarageFleetRole::Road;
    return EGTTGarageFleetRole::Utility;
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

FName UGTTGarageFleetSubsystem::GetRoleLoadoutVehicleId(EGTTGarageFleetRole Role) const
{
    switch (Role)
    {
        case EGTTGarageFleetRole::Tractor: return PreferredTractorVehicleId;
        case EGTTGarageFleetRole::Road: return PreferredRoadVehicleId;
        case EGTTGarageFleetRole::Cargo: return PreferredCargoVehicleId;
        default: return NAME_None;
    }
}

void UGTTGarageFleetSubsystem::SeedRoleLoadoutIfNeeded(EGTTGarageFleetRole Role, FName VehicleId)
{
    if (VehicleId.IsNone() || ClassifyVehicleRole(VehicleId) != Role) return;
    FName* Target = nullptr;
    if (Role == EGTTGarageFleetRole::Tractor) Target = &PreferredTractorVehicleId;
    else if (Role == EGTTGarageFleetRole::Road) Target = &PreferredRoadVehicleId;
    else if (Role == EGTTGarageFleetRole::Cargo) Target = &PreferredCargoVehicleId;
    if (Target && (Target->IsNone() || !IsOwnedFleetVehicle(*Target))) *Target = VehicleId;
}

bool UGTTGarageFleetSubsystem::SetRoleLoadoutVehicleId(EGTTGarageFleetRole Role, FName VehicleId)
{
    if (!IsOwnedFleetVehicle(VehicleId) || ClassifyVehicleRole(VehicleId) != Role) return false;
    switch (Role)
    {
        case EGTTGarageFleetRole::Tractor: PreferredTractorVehicleId = VehicleId; return true;
        case EGTTGarageFleetRole::Road: PreferredRoadVehicleId = VehicleId; return true;
        case EGTTGarageFleetRole::Cargo: PreferredCargoVehicleId = VehicleId; return true;
        default: return false;
    }
}

bool UGTTGarageFleetSubsystem::SetPreferredVehicleId(FName VehicleId)
{
    if (!IsOwnedFleetVehicle(VehicleId)) return false;
    PreferredVehicleId = VehicleId;
    SetRoleLoadoutVehicleId(ClassifyVehicleRole(VehicleId), VehicleId);
    return true;
}

void UGTTGarageFleetSubsystem::RestoreRoleLoadouts(FName TractorVehicleId, FName RoadVehicleId, FName CargoVehicleId)
{
    PreferredTractorVehicleId = IsOwnedFleetVehicle(TractorVehicleId) && ClassifyVehicleRole(TractorVehicleId) == EGTTGarageFleetRole::Tractor ? TractorVehicleId : NAME_None;
    PreferredRoadVehicleId = IsOwnedFleetVehicle(RoadVehicleId) && ClassifyVehicleRole(RoadVehicleId) == EGTTGarageFleetRole::Road ? RoadVehicleId : NAME_None;
    PreferredCargoVehicleId = IsOwnedFleetVehicle(CargoVehicleId) && ClassifyVehicleRole(CargoVehicleId) == EGTTGarageFleetRole::Cargo ? CargoVehicleId : NAME_None;

    for (AGTTVehicleBase* Vehicle : GatherOwnedVehicles())
    {
        if (Vehicle) SeedRoleLoadoutIfNeeded(ClassifyVehicleRole(Vehicle->GetPersistentVehicleId()), Vehicle->GetPersistentVehicleId());
    }
}

void UGTTGarageFleetSubsystem::RestorePreferredVehicleId(FName VehicleId)
{
    if (IsOwnedFleetVehicle(VehicleId))
    {
        PreferredVehicleId = VehicleId;
        SeedRoleLoadoutIfNeeded(ClassifyVehicleRole(VehicleId), VehicleId);
        return;
    }

    const TArray<AGTTVehicleBase*> Vehicles = GatherOwnedVehicles();
    PreferredVehicleId = Vehicles.Num() > 0 && Vehicles[0] ? Vehicles[0]->GetPersistentVehicleId() : NAME_None;
    if (!PreferredVehicleId.IsNone()) SeedRoleLoadoutIfNeeded(ClassifyVehicleRole(PreferredVehicleId), PreferredVehicleId);
}

bool UGTTGarageFleetSubsystem::IsPreferredVehicleFitForJob(FName JobTag) const
{
    const EGTTGarageFleetRole RequiredRole = RecommendedRoleForJob(JobTag);
    return RequiredRole != EGTTGarageFleetRole::Utility && ClassifyVehicleRole(PreferredVehicleId) == RequiredRole;
}

FGTTFleetMissionAssessment UGTTGarageFleetSubsystem::AssessJobReadiness(FName JobTag) const
{
    FGTTFleetMissionAssessment Assessment;
    Assessment.JobTag = JobTag;
    Assessment.RequiredRole = RecommendedRoleForJob(JobTag);

    if (Assessment.RequiredRole == EGTTGarageFleetRole::Utility)
    {
        Assessment.Readiness = EGTTFleetMissionReadiness::Advisory;
        Assessment.Reason = TEXT("No dedicated fleet role is required for this activity.");
        return Assessment;
    }

    FName Assigned = GetRoleLoadoutVehicleId(Assessment.RequiredRole);
    if (Assigned.IsNone() || !IsOwnedFleetVehicle(Assigned)) Assigned = RecommendedVehicleForJob(JobTag);
    Assessment.AssignedVehicleId = Assigned;

    const TArray<FGTTGarageFleetSnapshot> Fleet = BuildFleetSnapshot(8);
    const FGTTGarageFleetSnapshot* Snapshot = Fleet.FindByPredicate([Assigned](const FGTTGarageFleetSnapshot& Candidate)
    {
        return Candidate.VehicleId == Assigned;
    });
    if (!Snapshot)
    {
        Assessment.Readiness = EGTTFleetMissionReadiness::Unavailable;
        Assessment.Reason = FString::Printf(TEXT("No owned %s loadout is available."), *FleetRoleLabel(Assessment.RequiredRole));
        return Assessment;
    }

    Assessment.AssignedVehicleName = Snapshot->DisplayName;
    Assessment.AssignedSlot = Snapshot->SlotIndex;
    Assessment.ConditionPercent = Snapshot->ConditionPercent;
    Assessment.FuelPercent = Snapshot->FuelPercent;
    Assessment.TireIntegrity = Snapshot->TireIntegrity;
    Assessment.BodyHealth = Snapshot->BodyHealth;
    Assessment.PrepEstimate = Snapshot->RepairEstimate + Snapshot->FuelEstimate + Snapshot->TireServiceEstimate;

    float MinCondition = 0.4f;
    float MinFuel = 0.2f;
    float MinTires = 0.35f;
    float MinBody = 0.45f;
    GetJobThresholds(JobTag, MinCondition, MinFuel, MinTires, MinBody);

    const bool bHardFailure = Snapshot->ServiceStatus == TEXT("IMMOBILE") || Snapshot->ServiceStatus == TEXT("TOW") ||
        Snapshot->ConditionPercent < 0.25f || Snapshot->FuelPercent < 0.03f || Snapshot->TireIntegrity < 0.15f || Snapshot->BodyHealth < 0.25f;
    const bool bBelowTarget = Snapshot->ConditionPercent < MinCondition || Snapshot->FuelPercent < MinFuel ||
        Snapshot->TireIntegrity < MinTires || Snapshot->BodyHealth < MinBody || Snapshot->ServiceStatus == TEXT("LIMP") || Snapshot->ServiceStatus == TEXT("SERVICE");

    if (bHardFailure)
    {
        Assessment.Readiness = EGTTFleetMissionReadiness::ServiceRequired;
        Assessment.Reason = FString::Printf(TEXT("%s needs workshop/refuel service before this contract."), *Snapshot->DisplayName);
    }
    else if (bBelowTarget)
    {
        Assessment.Readiness = EGTTFleetMissionReadiness::Advisory;
        Assessment.Reason = FString::Printf(TEXT("%s can attempt the job, but current wear/fuel is below the recommended margin."), *Snapshot->DisplayName);
    }
    else
    {
        Assessment.Readiness = EGTTFleetMissionReadiness::Ready;
        Assessment.Reason = FString::Printf(TEXT("%s is mission-ready."), *Snapshot->DisplayName);
    }
    return Assessment;
}

bool UGTTGarageFleetSubsystem::IsJobFleetReady(FName JobTag) const
{
    return AssessJobReadiness(JobTag).Readiness == EGTTFleetMissionReadiness::Ready;
}

FString UGTTGarageFleetSubsystem::BuildJobDispatchHint(FName JobTag) const
{
    const FGTTFleetMissionAssessment Assessment = AssessJobReadiness(JobTag);
    if (Assessment.RequiredRole == EGTTGarageFleetRole::Utility)
    {
        return TEXT("FLEET: use a healthy vehicle suited to the contract.");
    }
    if (Assessment.Readiness == EGTTFleetMissionReadiness::Unavailable)
    {
        return FString::Printf(TEXT("FLEET %s: %s"), *MissionReadinessLabel(Assessment.Readiness), *Assessment.Reason);
    }

    const bool bActive = Assessment.AssignedVehicleId == PreferredVehicleId;
    const FString Prep = Assessment.PrepEstimate > 0 ? FString::Printf(TEXT(" | prep~$%d"), Assessment.PrepEstimate) : FString();
    return FString::Printf(TEXT("FLEET %s: slot %d %s [%s]%s | C%.0f F%.0f T%.0f B%.0f%s | %s"),
        *MissionReadinessLabel(Assessment.Readiness),
        Assessment.AssignedSlot + 1,
        *Assessment.AssignedVehicleName,
        *FleetRoleLabel(Assessment.RequiredRole),
        bActive ? TEXT(" ACTIVE") : TEXT(""),
        Assessment.ConditionPercent * 100.0f,
        Assessment.FuelPercent * 100.0f,
        Assessment.TireIntegrity * 100.0f,
        Assessment.BodyHealth * 100.0f,
        *Prep,
        *Assessment.Reason);
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
        Snapshot.bRoleLoadout = Snapshot.VehicleId == GetRoleLoadoutVehicleId(Snapshot.Role);
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
                const FGTTBreakdownAssessment BreakdownAssessment = Breakdown->AssessVehicle(NativeRoad, GarageWorkshopBaseCost);
                Snapshot.ServiceStatus = Snapshot.bOccupied ? TEXT("IN USE") : BreakdownStatus(BreakdownAssessment.Recommendation);
                Snapshot.RepairEstimate = NativeRoad->NeedsNativeWorkshopService() ? BreakdownAssessment.RepairEstimate : 0;
                Snapshot.TowEstimate = BreakdownAssessment.TowEstimate;
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

    Summary += FString::Printf(TEXT("\nLOADOUTS T:%s R:%s C:%s"),
        PreferredTractorVehicleId.IsNone() ? TEXT("-") : *PreferredTractorVehicleId.ToString(),
        PreferredRoadVehicleId.IsNone() ? TEXT("-") : *PreferredRoadVehicleId.ToString(),
        PreferredCargoVehicleId.IsNone() ? TEXT("-") : *PreferredCargoVehicleId.ToString());

    for (const FGTTGarageFleetSnapshot& Vehicle : Fleet)
    {
        const FString NativeTag = Vehicle.bNativeAuthority ? TEXT(" N") : TEXT("");
        const FString ActiveTag = Vehicle.bPreferredDispatch ? TEXT(" ACTIVE") : TEXT("");
        const FString LoadoutTag = Vehicle.bRoleLoadout ? TEXT(" LOADOUT") : TEXT("");
        FString Costs;
        if (Vehicle.RepairEstimate > 0) Costs += FString::Printf(TEXT(" repair~$%d"), Vehicle.RepairEstimate);
        if (Vehicle.FuelEstimate > 0) Costs += FString::Printf(TEXT(" fuel~$%d"), Vehicle.FuelEstimate);
        if (Vehicle.TireServiceEstimate > 0) Costs += FString::Printf(TEXT(" tires$%d"), Vehicle.TireServiceEstimate);
        if (Vehicle.NextEngineUpgradeCost > 0) Costs += FString::Printf(TEXT(" eng$%d"), Vehicle.NextEngineUpgradeCost);
        if (Vehicle.NextTireUpgradeCost > 0) Costs += FString::Printf(TEXT(" grip$%d"), Vehicle.NextTireUpgradeCost);
        if (Vehicle.TowEstimate > 0 && (Vehicle.ServiceStatus == TEXT("TOW") || Vehicle.ServiceStatus == TEXT("IMMOBILE")))
            Costs += FString::Printf(TEXT(" tow~$%d"), Vehicle.TowEstimate);

        Summary += FString::Printf(TEXT("\n%d %s%s | %s%s%s | %s | C%.0f F%.0f T%.0f B%.0f | E%d/3 G%d/3 | NEXT %s%s"),
            Vehicle.SlotIndex + 1,
            *Vehicle.DisplayName,
            *NativeTag,
            *FleetRoleLabel(Vehicle.Role),
            *ActiveTag,
            *LoadoutTag,
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
