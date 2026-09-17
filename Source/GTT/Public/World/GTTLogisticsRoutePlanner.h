#pragma once

#include "CoreMinimal.h"

class UWorld;

enum class EGTTLogisticsPriorityLane : uint8
{
    Wait,
    Road,
    Cargo,
    Split
};

struct GTT_API FGTTLogisticsRoutePlan
{
    EGTTLogisticsPriorityLane RecommendedLane = EGTTLogisticsPriorityLane::Wait;
    int32 RoadScore = 0;
    int32 CargoScore = 0;
    int32 RoadUrgency = 0;
    int32 CargoUrgency = 0;
    int32 RoadFailureDebt = 0;
    int32 CargoFailureDebt = 0;
    int32 BacklogPressure = 0;
    int32 ConsequencePressure = 0;
    int32 RoadPrepEstimate = 0;
    int32 CargoPrepEstimate = 0;
    FString RoadReadiness;
    FString CargoReadiness;
    FString RecommendationLabel;
    FString ConsequenceLedger;
};

// 0.1.12 shared decision layer. It consumes the existing living market, fleet-readiness,
// priority-SLA and persisted delivery-history signals instead of inventing another mission state.
// The returned plan is advisory, but its ROAD-vs-CARGO conflict cap is authoritative when the
// dispatcher writes a scarce CARGO hold, making route choice affect real reservation timing.
class GTT_API FGTTLogisticsRoutePlanner final
{
public:
    static FGTTLogisticsRoutePlan Build(UWorld* World);
    static FString BuildSummary(UWorld* World);
    static FString GetRecommendationLabel(UWorld* World);
    static FString GetConsequenceLedgerSummary(UWorld* World);

    // 0 means no extra cross-lane cap. A positive value means urgent ROAD is materially more
    // valuable/ready than urgent CARGO, so CARGO stock cannot be protected for the full favor
    // window while the village is waiting on the competing parts route.
    static int32 GetCargoConflictHoldCapMinutes(UWorld* World);
};
