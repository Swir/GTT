#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTStructuralDriveConsequenceSubsystem.h"

namespace
{
    const FVector WorkshopBaseLocation(-400.0f, 2650.0f, 105.0f);
    float MinimumBodyHealth(const FGTTRoadBodyDamageSnapshot& Body) { return FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth)); }
    float FuelCapacityForVehicle(const AGTTRoadVehicleNativePawn* Vehicle) { return Vehicle && Vehicle->GetPersistentVehicleId() == FName(TEXT("Mulebox1200")) ? 62.0f : 42.0f; }
}

int32 UGTTBreakdownDecisionSubsystem::CalculateRepairEstimate(const AGTTRoadVehicleNativePawn* Vehicle, int32 BaseWorkshopCost) const
{
    if (!Vehicle) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const float MissingFuel = FMath::Max(0.0f, FuelCapacityForVehicle(Vehicle) - State.FuelLiters);
    const int32 MechanicalLabor = FMath::RoundToInt((1.0f - FMath::Clamp(State.ConditionPercent, 0.0f, 1.0f)) * 260.0f);
    const int32 TireParts = FMath::RoundToInt((1.0f - FMath::Clamp(State.TireIntegrity, 0.0f, 1.0f)) * 170.0f);
    const int32 FuelCharge = FMath::RoundToInt(MissingFuel * 2.0f);
    return FMath::Clamp(FMath::Max(1, BaseWorkshopCost) + MechanicalLabor + TireParts + FuelCharge + Vehicle->GetBodyDamageRepairSurcharge(), FMath::Max(1, BaseWorkshopCost), 1500);
}

int32 UGTTBreakdownDecisionSubsystem::CalculateTowEstimate(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    const float DistanceMeters = FVector::Dist2D(Vehicle->GetActorLocation(), WorkshopBaseLocation) / 100.0f;
    const int32 DistanceCharge = FMath::Clamp(FMath::RoundToInt(DistanceMeters * 0.03f / 5.0f) * 5, 0, 220);
    const float Severity = FMath::Clamp(FMath::Max(FMath::Max(1.0f - State.ConditionPercent, 1.0f - State.TireIntegrity), 1.0f - MinimumBodyHealth(Body)), 0.0f, 1.0f);
    const int32 DamageHandling = FMath::RoundToInt(Severity * 150.0f);
    const int32 StructuralHandling = FMath::Clamp(Vehicle->GetBodyDamageRepairSurcharge() / 2, 0, 220);
    return FMath::Clamp(110 + DistanceCharge + DamageHandling + StructuralHandling, 110, 850);
}

bool UGTTBreakdownDecisionSubsystem::CanEmergencyPatch(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !Vehicle->IsLegacyTakeoverActive()) return false;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();

    const bool bBodyDisabled = Body.DetachedPanelCount >= 3 && MinimumBodyHealth(Body) <= 0.20f;
    if (bBodyDisabled) return false;

    if (const UWorld* World = GetWorld())
    {
        if (const UGTTStructuralDriveConsequenceSubsystem* Drive = World->GetSubsystem<UGTTStructuralDriveConsequenceSubsystem>())
        {
            const FGTTStructuralDriveState Structural = Drive->GetDriveStateForVehicle(Vehicle);
            if (Structural.DamageSeverity >= 0.72f) return false;
        }
    }

    return State.ConditionPercent < 0.30f || State.TireIntegrity < 0.32f || State.FuelLiters < 5.0f;
}

int32 UGTTBreakdownDecisionSubsystem::CalculateRoadsidePatchEstimate(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !CanEmergencyPatch(Vehicle)) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();

    const float ConditionNeed = FMath::Max(0.0f, 0.30f - State.ConditionPercent);
    const float TireNeed = FMath::Max(0.0f, 0.32f - State.TireIntegrity);
    const float FuelNeed = FMath::Max(0.0f, 5.0f - State.FuelLiters);
    const int32 MechanicalAid = FMath::RoundToInt(ConditionNeed * 290.0f);
    const int32 TireAid = FMath::RoundToInt(TireNeed * 220.0f);
    const int32 FuelAid = FMath::RoundToInt(FuelNeed * 3.0f);

    return FMath::Clamp(55 + MechanicalAid + TireAid + FuelAid, 55, 260);
}

FGTTBreakdownAssessment UGTTBreakdownDecisionSubsystem::AssessVehicle(const AGTTRoadVehicleNativePawn* Vehicle, int32 BaseWorkshopCost) const
{
    FGTTBreakdownAssessment Result;
    if (!Vehicle) return Result;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    FGTTStructuralDriveState Structural;
    if (const UWorld* World = GetWorld()) if (const UGTTStructuralDriveConsequenceSubsystem* Drive = World->GetSubsystem<UGTTStructuralDriveConsequenceSubsystem>()) Structural = Drive->GetDriveStateForVehicle(Vehicle);
    const float MechanicalSeverity = FMath::Max(1.0f - State.ConditionPercent, 1.0f - State.TireIntegrity);
    const float BodySeverity = 1.0f - MinimumBodyHealth(Body);
    Result.Severity = FMath::Clamp(FMath::Max3(MechanicalSeverity, BodySeverity, Structural.DamageSeverity), 0.0f, 1.0f);
    Result.RepairEstimate = CalculateRepairEstimate(Vehicle, BaseWorkshopCost);
    Result.TowEstimate = CalculateTowEstimate(Vehicle);
    Result.bEmergencyPatchPossible = CanEmergencyPatch(Vehicle);
    Result.EmergencyPatchEstimate = Result.bEmergencyPatchPossible ? CalculateRoadsidePatchEstimate(Vehicle) : 0;
    const bool bBodyDisabled = Body.DetachedPanelCount >= 3 && MinimumBodyHealth(Body) <= 0.20f;
    const bool bImmobilized = State.ConditionPercent <= 0.05f || State.FuelLiters <= 0.05f || State.TireIntegrity <= 0.08f || bBodyDisabled;
    Result.bTowRecommended = bImmobilized || Result.Severity >= 0.72f || State.TireIntegrity <= 0.22f || State.ConditionPercent <= 0.20f;
    Result.bCanLimpHome = !bImmobilized && (Structural.bLimpHomeActive || Result.Severity >= 0.34f || State.TireIntegrity <= 0.45f || State.ConditionPercent <= 0.45f);
    Result.Recommendation = bImmobilized ? EGTTBreakdownRecommendation::Immobilized : Result.bTowRecommended ? EGTTBreakdownRecommendation::TowRecommended : Result.bCanLimpHome ? EGTTBreakdownRecommendation::LimpToWorkshop : EGTTBreakdownRecommendation::DriveNormally;
    return Result;
}
