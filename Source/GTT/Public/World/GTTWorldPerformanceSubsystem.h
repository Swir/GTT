#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTWorldPerformanceSubsystem.generated.h"

UENUM(BlueprintType)
enum class EGTTWorldSimulationTier : uint8
{
    Critical,
    Near,
    Mid,
    Far,
    Dormant
};

/**
 * Central distance-based simulation budget for the sandbox.
 * Actors can remain fully reactive near the player while reducing expensive work
 * across the larger countryside. Combat/mission-critical actors can opt out by
 * requesting a critical budget.
 */
UCLASS()
class GTT_API UGTTWorldPerformanceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="GTT|Performance")
    EGTTWorldSimulationTier GetSimulationTier(const AActor* Actor, bool bForceCritical = false) const;

    UFUNCTION(BlueprintPure, Category="GTT|Performance")
    float GetRecommendedTickInterval(const AActor* Actor, bool bForceCritical = false) const;

    UFUNCTION(BlueprintPure, Category="GTT|Performance")
    bool AllowsExpensiveQueries(const AActor* Actor, bool bForceCritical = false) const;

    UFUNCTION(BlueprintPure, Category="GTT|Performance")
    FString GetSimulationTierLabel(const AActor* Actor, bool bForceCritical = false) const;

private:
    float GetSquaredDistanceToPlayer(const AActor* Actor) const;

    static constexpr float CriticalRadius = 2500.0f;
    static constexpr float NearRadius = 6000.0f;
    static constexpr float MidRadius = 11000.0f;
    static constexpr float FarRadius = 17000.0f;
};
