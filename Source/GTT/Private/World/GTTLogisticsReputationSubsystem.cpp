#include "World/GTTLogisticsReputationSubsystem.h"

#include "Core/GTTGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
#include "World/GTTDayNightCycle.h"

namespace
{
    constexpr int32 LogisticsSaveVersion = 8;
    constexpr float CourierOpenHour = 6.0f;
    constexpr float CourierCloseHour = 21.5f;
    constexpr float LateShiftHour = 18.5f;
}

float UGTTLogisticsReputationSubsystem::GetTimeOfDayHours() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GameMode ? GameMode->GetDayNightCycle() : nullptr;
    return Cycle ? Cycle->GetTimeOfDayHours() : 12.0f;
}

FString UGTTLogisticsReputationSubsystem::GetTierLabel() const
{
    if (Reputation >= 80) return TEXT("COUNTY PRO");
    if (Reputation >= 50) return TEXT("RELIABLE");
    if (Reputation >= 20) return TEXT("TRUSTED");
    return TEXT("NEWCOMER");
}

bool UGTTLogisticsReputationSubsystem::IsRoadCourierWindowOpen() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour >= CourierOpenHour && Hour < CourierCloseHour;
}

bool UGTTLogisticsReputationSubsystem::IsLateShift() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour >= LateShiftHour && Hour < CourierCloseHour;
}

FString UGTTLogisticsReputationSubsystem::GetRoadCourierScheduleLabel() const
{
    if (!IsRoadCourierWindowOpen()) return TEXT("CLOSED 21:30-06:00");
    if (IsLateShift()) return TEXT("LATE SHIFT +10%");
    return TEXT("OPEN 06:00-21:30");
}

float UGTTLogisticsReputationSubsystem::GetRoadCourierRewardMultiplier() const
{
    const float ReputationBonus = FMath::Min(0.20f, static_cast<float>(Reputation) * 0.002f);
    const float StreakBonus = FMath::Min(0.05f, static_cast<float>(CleanStreak) * 0.01f);
    const float ShiftMultiplier = IsLateShift() ? 1.10f : 1.0f;
    return (1.0f + ReputationBonus + StreakBonus) * ShiftMultiplier;
}

void UGTTLogisticsReputationSubsystem::RecordCourierSuccess(
    int32 Payout,
    float ParcelIntegrity,
    bool bFastDelivery,
    bool bPoliceIncident,
    int32 NativeImpacts)
{
    const bool bClean = ParcelIntegrity >= 0.97f && !bPoliceIncident && NativeImpacts <= 0;
    int32 ReputationDelta = bClean ? 8 + (bFastDelivery ? 2 : 0) : (ParcelIntegrity >= 0.75f ? 4 : 1);
    if (bPoliceIncident) ReputationDelta -= 3;

    Reputation = FMath::Clamp(Reputation + ReputationDelta, 0, 100);
    CleanStreak = bClean ? FMath::Min(CleanStreak + 1, 99) : 0;
    ++CompletedRuns;
    LifetimeRevenue = FMath::Max(0, LifetimeRevenue + FMath::Max(0, Payout));
}

void UGTTLogisticsReputationSubsystem::RecordCourierFailure(bool bSevereFailure)
{
    Reputation = FMath::Clamp(Reputation - (bSevereFailure ? 12 : 8), 0, 100);
    CleanStreak = 0;
    ++FailedRuns;
}

void UGTTLogisticsReputationSubsystem::CaptureToSave(UGTTSaveGame* Save) const
{
    if (!Save) return;
    Save->LogisticsReputation = Reputation;
    Save->LogisticsCleanStreak = CleanStreak;
    Save->LogisticsCompletedRuns = CompletedRuns;
    Save->LogisticsFailedRuns = FailedRuns;
    Save->LogisticsLifetimeRevenue = LifetimeRevenue;
}

void UGTTLogisticsReputationSubsystem::RestoreFromSave(const UGTTSaveGame* Save)
{
    if (!Save || Save->SaveVersion < LogisticsSaveVersion)
    {
        Reputation = 0;
        CleanStreak = 0;
        CompletedRuns = 0;
        FailedRuns = 0;
        LifetimeRevenue = 0;
        return;
    }

    Reputation = FMath::Clamp(Save->LogisticsReputation, 0, 100);
    CleanStreak = FMath::Max(0, Save->LogisticsCleanStreak);
    CompletedRuns = FMath::Max(0, Save->LogisticsCompletedRuns);
    FailedRuns = FMath::Max(0, Save->LogisticsFailedRuns);
    LifetimeRevenue = FMath::Max(0, Save->LogisticsLifetimeRevenue);
}
