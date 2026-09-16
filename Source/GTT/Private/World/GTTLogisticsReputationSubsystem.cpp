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
    // Matches the existing citizen work schedule (07:00-17:30): cargo counters are staffed while rural NPCs are at work.
    constexpr float CargoOpenHour = 7.0f;
    constexpr float CargoCloseHour = 17.5f;
    constexpr int32 MaxRecentContracts = 6;
}

float UGTTLogisticsReputationSubsystem::GetTimeOfDayHours() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GameMode ? GameMode->GetDayNightCycle() : nullptr;
    return Cycle ? Cycle->GetTimeOfDayHours() : 12.0f;
}

int32 UGTTLogisticsReputationSubsystem::GetDayNumber() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GameMode ? GameMode->GetDayNightCycle() : nullptr;
    return Cycle ? FMath::Max(1, Cycle->GetDayNumber()) : 1;
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

bool UGTTLogisticsReputationSubsystem::IsCargoDepotWindowOpen() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour >= CargoOpenHour && Hour < CargoCloseHour;
}

FString UGTTLogisticsReputationSubsystem::GetCargoScheduleLabel() const
{
    return IsCargoDepotWindowOpen() ? TEXT("STAFFED 07:00-17:30") : TEXT("CLOSED 17:30-07:00");
}

int32 UGTTLogisticsReputationSubsystem::GetCargoRouteTier() const
{
    if (Reputation >= 50) return 3;
    if (Reputation >= 20) return 2;
    return 1;
}

float UGTTLogisticsReputationSubsystem::GetCargoMarketMultiplier() const
{
    const float Hour = GetTimeOfDayHours();
    const float TimeDemandBonus = Hour < 10.0f ? 0.12f : (Hour >= 14.0f ? 0.08f : 0.0f);
    const int32 DemandCycle = (GetDayNumber() + CargoCompletedRuns + CompletedRuns) % 3;
    const float DailyDemandBonus = DemandCycle == 0 ? 0.10f : (DemandCycle == 1 ? 0.04f : 0.0f);
    const float ReputationBonus = FMath::Min(0.12f, static_cast<float>(Reputation) * 0.0015f);
    const float StreakBonus = FMath::Min(0.04f, static_cast<float>(CleanStreak) * 0.008f);
    return FMath::Clamp(1.0f + TimeDemandBonus + DailyDemandBonus + ReputationBonus + StreakBonus, 1.0f, 1.38f);
}

FString UGTTLogisticsReputationSubsystem::GetCargoMarketLabel() const
{
    if (!IsCargoDepotWindowOpen()) return TEXT("MARKET PAUSED");
    const float Hour = GetTimeOfDayHours();
    const FString Demand = Hour < 10.0f ? TEXT("MORNING RUSH") : (Hour >= 14.0f ? TEXT("LATE FEED DEMAND") : TEXT("STEADY DEMAND"));
    const int32 Bonus = FMath::RoundToInt((GetCargoMarketMultiplier() - 1.0f) * 100.0f);
    return FString::Printf(TEXT("%s +%d%% | ROUTE T%d"), *Demand, Bonus, GetCargoRouteTier());
}

FString UGTTLogisticsReputationSubsystem::GetRecentHistorySummary() const
{
    if (RecentContractTags.Num() <= 0) return TEXT("NO DELIVERY HISTORY");
    const int32 Last = RecentContractTags.Num() - 1;
    return FString::Printf(TEXT("LAST %s | $%d | Q%d%%"),
        *RecentContractTags[Last].ToString().ToUpper(),
        RecentPayouts.IsValidIndex(Last) ? RecentPayouts[Last] : 0,
        RecentQualityPercent.IsValidIndex(Last) ? RecentQualityPercent[Last] : 0);
}

void UGTTLogisticsReputationSubsystem::AppendHistory(FName ContractTag, int32 Payout, int32 QualityPercent)
{
    RecentContractTags.Add(ContractTag);
    RecentPayouts.Add(FMath::Max(0, Payout));
    RecentQualityPercent.Add(FMath::Clamp(QualityPercent, 0, 100));
    while (RecentContractTags.Num() > MaxRecentContracts)
    {
        RecentContractTags.RemoveAt(0);
        RecentPayouts.RemoveAt(0);
        RecentQualityPercent.RemoveAt(0);
    }
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
    AppendHistory(FName(TEXT("RoadRun")), Payout, FMath::RoundToInt(ParcelIntegrity * 100.0f));
}

void UGTTLogisticsReputationSubsystem::RecordCourierFailure(bool bSevereFailure)
{
    Reputation = FMath::Clamp(Reputation - (bSevereFailure ? 12 : 8), 0, 100);
    CleanStreak = 0;
    ++FailedRuns;
    AppendHistory(FName(TEXT("RoadRunFail")), 0, 0);
}

void UGTTLogisticsReputationSubsystem::RecordCargoSuccess(
    int32 Payout,
    float CargoIntegrity,
    bool bFastDelivery,
    bool bPoliceIncident,
    bool bExtendedRoute)
{
    const bool bClean = CargoIntegrity >= 0.94f && !bPoliceIncident;
    int32 ReputationDelta = bClean ? 6 + (bFastDelivery ? 2 : 0) : (CargoIntegrity >= 0.70f ? 3 : 1);
    if (bExtendedRoute) ReputationDelta += 2;
    if (bPoliceIncident) ReputationDelta -= 3;

    Reputation = FMath::Clamp(Reputation + ReputationDelta, 0, 100);
    CleanStreak = bClean ? FMath::Min(CleanStreak + 1, 99) : 0;
    ++CompletedRuns;
    ++CargoCompletedRuns;
    LifetimeRevenue = FMath::Max(0, LifetimeRevenue + FMath::Max(0, Payout));
    CargoLifetimeRevenue = FMath::Max(0, CargoLifetimeRevenue + FMath::Max(0, Payout));
    AppendHistory(bExtendedRoute ? FName(TEXT("CargoChain")) : FName(TEXT("FarmCargo")), Payout, FMath::RoundToInt(CargoIntegrity * 100.0f));
}

void UGTTLogisticsReputationSubsystem::RecordCargoFailure(float CargoIntegrity, bool bSevereFailure)
{
    Reputation = FMath::Clamp(Reputation - (bSevereFailure ? 10 : 6), 0, 100);
    CleanStreak = 0;
    ++FailedRuns;
    ++CargoFailedRuns;
    AppendHistory(FName(TEXT("CargoFail")), 0, FMath::RoundToInt(CargoIntegrity * 100.0f));
}

void UGTTLogisticsReputationSubsystem::CaptureToSave(UGTTSaveGame* Save) const
{
    if (!Save) return;
    Save->LogisticsReputation = Reputation;
    Save->LogisticsCleanStreak = CleanStreak;
    Save->LogisticsCompletedRuns = CompletedRuns;
    Save->LogisticsFailedRuns = FailedRuns;
    Save->LogisticsLifetimeRevenue = LifetimeRevenue;
    Save->LogisticsCargoCompletedRuns = CargoCompletedRuns;
    Save->LogisticsCargoFailedRuns = CargoFailedRuns;
    Save->LogisticsCargoLifetimeRevenue = CargoLifetimeRevenue;
    Save->LogisticsRecentContractTags = RecentContractTags;
    Save->LogisticsRecentPayouts = RecentPayouts;
    Save->LogisticsRecentQualityPercent = RecentQualityPercent;
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
        CargoCompletedRuns = 0;
        CargoFailedRuns = 0;
        CargoLifetimeRevenue = 0;
        RecentContractTags.Reset();
        RecentPayouts.Reset();
        RecentQualityPercent.Reset();
        return;
    }

    Reputation = FMath::Clamp(Save->LogisticsReputation, 0, 100);
    CleanStreak = FMath::Max(0, Save->LogisticsCleanStreak);
    CompletedRuns = FMath::Max(0, Save->LogisticsCompletedRuns);
    FailedRuns = FMath::Max(0, Save->LogisticsFailedRuns);
    LifetimeRevenue = FMath::Max(0, Save->LogisticsLifetimeRevenue);
    CargoCompletedRuns = FMath::Max(0, Save->LogisticsCargoCompletedRuns);
    CargoFailedRuns = FMath::Max(0, Save->LogisticsCargoFailedRuns);
    CargoLifetimeRevenue = FMath::Max(0, Save->LogisticsCargoLifetimeRevenue);
    RecentContractTags = Save->LogisticsRecentContractTags;
    RecentPayouts = Save->LogisticsRecentPayouts;
    RecentQualityPercent = Save->LogisticsRecentQualityPercent;

    const int32 AlignedCount = FMath::Min3(RecentContractTags.Num(), RecentPayouts.Num(), RecentQualityPercent.Num());
    RecentContractTags.SetNum(AlignedCount);
    RecentPayouts.SetNum(AlignedCount);
    RecentQualityPercent.SetNum(AlignedCount);
    while (RecentContractTags.Num() > MaxRecentContracts)
    {
        RecentContractTags.RemoveAt(0);
        RecentPayouts.RemoveAt(0);
        RecentQualityPercent.RemoveAt(0);
    }
}
