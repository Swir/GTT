#include "World/GTTWorldPerformanceSubsystem.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

float UGTTWorldPerformanceSubsystem::GetSquaredDistanceToPlayer(const AActor* Actor) const
{
    if (!Actor || !GetWorld()) return TNumericLimits<float>::Max();
    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn) return TNumericLimits<float>::Max();
    return FVector::DistSquared2D(Actor->GetActorLocation(), PlayerPawn->GetActorLocation());
}

EGTTWorldSimulationTier UGTTWorldPerformanceSubsystem::GetSimulationTier(const AActor* Actor, bool bForceCritical) const
{
    if (bForceCritical) return EGTTWorldSimulationTier::Critical;

    const float DistanceSq = GetSquaredDistanceToPlayer(Actor);
    if (DistanceSq <= FMath::Square(CriticalRadius)) return EGTTWorldSimulationTier::Critical;
    if (DistanceSq <= FMath::Square(NearRadius)) return EGTTWorldSimulationTier::Near;
    if (DistanceSq <= FMath::Square(MidRadius)) return EGTTWorldSimulationTier::Mid;
    if (DistanceSq <= FMath::Square(FarRadius)) return EGTTWorldSimulationTier::Far;
    return EGTTWorldSimulationTier::Dormant;
}

float UGTTWorldPerformanceSubsystem::GetRecommendedTickInterval(const AActor* Actor, bool bForceCritical) const
{
    switch (GetSimulationTier(Actor, bForceCritical))
    {
        case EGTTWorldSimulationTier::Critical: return 0.0f;
        case EGTTWorldSimulationTier::Near: return 0.05f;
        case EGTTWorldSimulationTier::Mid: return 0.20f;
        case EGTTWorldSimulationTier::Far: return 0.65f;
        case EGTTWorldSimulationTier::Dormant: return 1.50f;
        default: return 0.20f;
    }
}

bool UGTTWorldPerformanceSubsystem::AllowsExpensiveQueries(const AActor* Actor, bool bForceCritical) const
{
    const EGTTWorldSimulationTier Tier = GetSimulationTier(Actor, bForceCritical);
    return Tier == EGTTWorldSimulationTier::Critical || Tier == EGTTWorldSimulationTier::Near;
}

FString UGTTWorldPerformanceSubsystem::GetSimulationTierLabel(const AActor* Actor, bool bForceCritical) const
{
    switch (GetSimulationTier(Actor, bForceCritical))
    {
        case EGTTWorldSimulationTier::Critical: return TEXT("CRITICAL");
        case EGTTWorldSimulationTier::Near: return TEXT("NEAR");
        case EGTTWorldSimulationTier::Mid: return TEXT("MID");
        case EGTTWorldSimulationTier::Far: return TEXT("FAR");
        case EGTTWorldSimulationTier::Dormant: return TEXT("DORMANT");
        default: return TEXT("UNKNOWN");
    }
}
