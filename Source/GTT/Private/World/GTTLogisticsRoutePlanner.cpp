#include "World/GTTLogisticsRoutePlanner.h"

#include "Engine/World.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTLogisticsReputationSubsystem.h"

namespace
{
const FName RoadRunJob(TEXT("RoadRun"));
const FName FarmCargoJob(TEXT("FarmCargo"));

int32 ReadinessPoints(EGTTFleetMissionReadiness Readiness)
{
    switch (Readiness)
    {
        case EGTTFleetMissionReadiness::Ready: return 28;
        case EGTTFleetMissionReadiness::Advisory: return 16;
        case EGTTFleetMissionReadiness::ServiceRequired: return -14;
        default: return -45;
    }
}

int32 PrepPenalty(int32 PrepEstimate)
{
    return FMath::Min(30, FMath::Max(0, PrepEstimate) / 8);
}

const TCHAR* LaneLabel(EGTTLogisticsPriorityLane Lane)
{
    switch (Lane)
    {
        case EGTTLogisticsPriorityLane::Road: return TEXT("ROAD / RATTLEBACK 82");
        case EGTTLogisticsPriorityLane::Cargo: return TEXT("CARGO / MULEBOX 1200");
        case EGTTLogisticsPriorityLane::Split: return TEXT("SPLIT / CHOOSE COMMITMENT");
        default: return TEXT("WAIT / SERVICE FLEET");
    }
}
}

FGTTLogisticsRoutePlan FGTTLogisticsRoutePlanner::Build(UWorld* World)
{
    FGTTLogisticsRoutePlan Plan;
    Plan.RoadReadiness = TEXT("OFFLINE");
    Plan.CargoReadiness = TEXT("OFFLINE");
    Plan.RecommendationLabel = TEXT("ROUTE PLANNER OFFLINE");
    Plan.ConsequenceLedger = TEXT("LEDGER OFFLINE");
    if (!World) return Plan;

    UGTTLogisticsReputationSubsystem* Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    UGTTGarageFleetSubsystem* Fleet = World->GetSubsystem<UGTTGarageFleetSubsystem>();
    if (!Logistics || !Fleet) return Plan;

    const FGTTFleetMissionAssessment RoadFleet = Fleet->AssessJobReadiness(RoadRunJob);
    const FGTTFleetMissionAssessment CargoFleet = Fleet->AssessJobReadiness(FarmCargoJob);

    Plan.RoadUrgency = Logistics->GetRoadPriorityUrgency();
    Plan.CargoUrgency = Logistics->GetCargoPriorityUrgency();
    Plan.RoadPrepEstimate = FMath::Max(0, RoadFleet.PrepEstimate);
    Plan.CargoPrepEstimate = FMath::Max(0, CargoFleet.PrepEstimate);
    Plan.RoadReadiness = UGTTGarageFleetSubsystem::MissionReadinessLabel(RoadFleet.Readiness);
    Plan.CargoReadiness = UGTTGarageFleetSubsystem::MissionReadinessLabel(CargoFleet.Readiness);
    Plan.BacklogPressure = Logistics->GetCargoBacklogPressure();

    // Failure debt is derived from counters already persisted in schema v8. Successful work
    // gradually earns it down, avoiding a permanent punishment while keeping abandoned work
    // visible beyond the single frame in which it failed.
    Plan.RoadFailureDebt = FMath::Clamp(
        Logistics->GetFailedRuns() - Logistics->GetCompletedRuns() / 3, 0, 4);
    Plan.CargoFailureDebt = FMath::Clamp(
        Logistics->GetCargoFailedRuns() - Logistics->GetCargoCompletedRuns() / 3, 0, 4);
    Plan.ConsequencePressure = FMath::Clamp(
        Plan.BacklogPressure * 2 + Plan.RoadFailureDebt * 3 + Plan.CargoFailureDebt * 3, 0, 30);

    Plan.RoadScore =
        Plan.RoadUrgency * 35 +
        ReadinessPoints(RoadFleet.Readiness) -
        PrepPenalty(Plan.RoadPrepEstimate) +
        (Logistics->IsRoadCourierWindowOpen() ? 12 : -22) +
        FMath::Min(18, Logistics->GetWoodYardDemand() * 2) +
        Plan.RoadFailureDebt * 4;

    Plan.CargoScore =
        Plan.CargoUrgency * 35 +
        ReadinessPoints(CargoFleet.Readiness) -
        PrepPenalty(Plan.CargoPrepEstimate) +
        (Logistics->IsCargoDepotWindowOpen() ? 12 : -22) +
        FMath::Min(20, Logistics->GetHillFarmDemand() + Logistics->GetWoodYardDemand()) +
        FMath::Min(16, Logistics->GetCargoReservationCount() * 8) +
        Plan.CargoFailureDebt * 4;

    const bool bCargoHasCommittedLoad = Logistics->GetCargoReservationCount() > 0;
    if (!Logistics->CanAcceptCargoContract() && !bCargoHasCommittedLoad)
    {
        Plan.CargoScore -= 45;
    }

    const bool bRoadUnavailable = RoadFleet.Readiness == EGTTFleetMissionReadiness::Unavailable;
    const bool bCargoUnavailable = CargoFleet.Readiness == EGTTFleetMissionReadiness::Unavailable;
    if (bRoadUnavailable && bCargoUnavailable)
    {
        Plan.RecommendedLane = EGTTLogisticsPriorityLane::Wait;
    }
    else if (FMath::Abs(Plan.RoadScore - Plan.CargoScore) <= 8 &&
             Plan.RoadUrgency > 0 && Plan.CargoUrgency > 0)
    {
        Plan.RecommendedLane = EGTTLogisticsPriorityLane::Split;
    }
    else if (!bRoadUnavailable && (bCargoUnavailable || Plan.RoadScore >= Plan.CargoScore))
    {
        Plan.RecommendedLane = EGTTLogisticsPriorityLane::Road;
    }
    else
    {
        Plan.RecommendedLane = EGTTLogisticsPriorityLane::Cargo;
    }

    Plan.RecommendationLabel = LaneLabel(Plan.RecommendedLane);
    Plan.ConsequenceLedger = FString::Printf(
        TEXT("LEDGER pressure %d | backlog %d | ROAD debt %d | CARGO debt %d"),
        Plan.ConsequencePressure, Plan.BacklogPressure, Plan.RoadFailureDebt, Plan.CargoFailureDebt);
    return Plan;
}

int32 FGTTLogisticsRoutePlanner::GetCargoConflictHoldCapMinutes(UWorld* World)
{
    if (!World) return 0;
    UGTTLogisticsReputationSubsystem* Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Logistics) return 0;

    const FGTTLogisticsRoutePlan Plan = Build(World);
    if (Plan.RoadUrgency <= 0 || Plan.CargoUrgency <= 0) return 0;
    if (Plan.RecommendedLane != EGTTLogisticsPriorityLane::Road) return 0;

    const int32 ScoreGap = Plan.RoadScore - Plan.CargoScore;
    if (ScoreGap < 15) return 0;

    const int32 CargoSlaMinutes = Logistics->GetCargoPriorityPickupSlaMinutes();
    if (CargoSlaMinutes <= 0) return 0;

    // The weaker lane remains playable, but it cannot lock scarce pallets for the entire
    // relationship favor while a materially stronger ROAD emergency is waiting. Larger score
    // gaps release stock sooner and therefore make route prioritization consequential.
    const float Scale = ScoreGap >= 50 ? 0.55f : (ScoreGap >= 30 ? 0.65f : 0.75f);
    return FMath::Max(10, FMath::RoundToInt(static_cast<float>(CargoSlaMinutes) * Scale));
}

FString FGTTLogisticsRoutePlanner::GetRecommendationLabel(UWorld* World)
{
    return Build(World).RecommendationLabel;
}

FString FGTTLogisticsRoutePlanner::GetConsequenceLedgerSummary(UWorld* World)
{
    return Build(World).ConsequenceLedger;
}

FString FGTTLogisticsRoutePlanner::BuildSummary(UWorld* World)
{
    const FGTTLogisticsRoutePlan Plan = Build(World);
    const int32 ConflictCap = GetCargoConflictHoldCapMinutes(World);
    const FString Conflict = ConflictCap > 0
        ? FString::Printf(TEXT("CARGO CONFLICT HOLD %d MIN"), ConflictCap)
        : TEXT("NO CROSS-LANE HOLD PENALTY");

    return FString::Printf(
        TEXT("ROUTE PLAN %s | ROAD U%d score %d %s prep $%d | CARGO U%d score %d %s prep $%d | %s | %s"),
        *Plan.RecommendationLabel,
        Plan.RoadUrgency, Plan.RoadScore, *Plan.RoadReadiness, Plan.RoadPrepEstimate,
        Plan.CargoUrgency, Plan.CargoScore, *Plan.CargoReadiness, Plan.CargoPrepEstimate,
        *Plan.ConsequenceLedger, *Conflict);
}
