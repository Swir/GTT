#include "Traffic/GTTRoadsideSceneSafetySubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Traffic/GTTRoadsideResponderVehicle.h"
#include "Traffic/GTTTrafficCarPawn.h"
#include "GTT.h"

TStatId UGTTRoadsideSceneSafetySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTRoadsideSceneSafetySubsystem, STATGROUP_Tickables);
}

void UGTTRoadsideSceneSafetySubsystem::Tick(float DeltaSeconds)
{
    ScanAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (ScanAccumulator < ScanIntervalSeconds)
    {
        return;
    }
    ScanAccumulator = 0.0f;

    PruneYieldHistory();

    AGTTRoadsideResponderVehicle* Responder = FindOnSceneResponder();
    if (!Responder)
    {
        TrackedResponder.Reset();
        bSafetyCorridorActive = false;
        LastYieldCount = 0;
        return;
    }

    if (TrackedResponder.Get() != Responder)
    {
        TrackedResponder = Responder;
        UE_LOG(LogGTT, Log,
            TEXT("ROADSIDE_SAFETY_CORRIDOR_ACTIVE incident=%s location=%s"),
            *Responder->GetAssignedIncidentId().ToString(),
            *Responder->GetActorLocation().ToCompactString());
    }

    bSafetyCorridorActive = true;
    ApplySafetyCorridor(Responder);
}

AGTTRoadsideResponderVehicle* UGTTRoadsideSceneSafetySubsystem::FindOnSceneResponder() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AGTTRoadsideResponderVehicle> It(World); It; ++It)
    {
        AGTTRoadsideResponderVehicle* Candidate = *It;
        if (Candidate && Candidate->IsParkedAtScene() && Candidate->IsSafetyCorridorDeployed() && !Candidate->GetAssignedIncidentId().IsNone())
        {
            return Candidate;
        }
    }
    return nullptr;
}

void UGTTRoadsideSceneSafetySubsystem::ApplySafetyCorridor(AGTTRoadsideResponderVehicle* Responder)
{
    UWorld* World = GetWorld();
    if (!World || !Responder)
    {
        LastYieldCount = 0;
        return;
    }

    const FVector SceneLocation = Responder->GetActorLocation();
    const float OuterRadiusSq = FMath::Square(SafetyRadiusCm);
    const float InnerRadiusSq = FMath::Square(InnerPassRadiusCm);
    const float NowSeconds = World->GetTimeSeconds();
    int32 Yielded = 0;

    for (TActorIterator<AGTTTrafficCarPawn> It(World); It; ++It)
    {
        AGTTTrafficCarPawn* TrafficCar = *It;
        if (!TrafficCar || TrafficCar->IsIncidentDisabled() || TrafficCar->IsRoadsideAssistanceActive() || TrafficCar->IsRoadsideResponderSceneAuthority() || TrafficCar->IsYieldingForRangerStop())
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared2D(TrafficCar->GetActorLocation(), SceneLocation);
        if (DistanceSq > OuterRadiusSq || DistanceSq < InnerRadiusSq)
        {
            continue;
        }

        const TWeakObjectPtr<AGTTTrafficCarPawn> Key(TrafficCar);
        if (const float* LastYieldTime = LastYieldTimeByCar.Find(Key))
        {
            if ((NowSeconds - *LastYieldTime) < ReYieldCooldownSeconds)
            {
                continue;
            }
        }

        TrafficCar->ReactToNearbyIncident(SceneLocation, YieldSeverity);
        LastYieldTimeByCar.Add(Key, NowSeconds);
        ++Yielded;
    }

    LastYieldCount = Yielded;
    if (Yielded > 0)
    {
        UE_LOG(LogGTT, Verbose,
            TEXT("ROADSIDE_SAFETY_CORRIDOR_YIELD incident=%s drivers=%d radius_cm=%.0f cooldown_s=%.1f"),
            *Responder->GetAssignedIncidentId().ToString(), Yielded, SafetyRadiusCm, ReYieldCooldownSeconds);
    }
}

void UGTTRoadsideSceneSafetySubsystem::PruneYieldHistory()
{
    for (auto It = LastYieldTimeByCar.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
