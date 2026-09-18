#pragma once

#include "CoreMinimal.h"

namespace GTTWorkshopHoursPolicy
{
    constexpr float OpeningHour = 6.5f;
    constexpr float ClosingHour = 20.0f;
    constexpr int32 AfterHoursRecoverySurchargePercent = 35;

    inline float NormalizeHour(float Hour)
    {
        float Normalized = FMath::Fmod(Hour, 24.0f);
        if (Normalized < 0.0f) Normalized += 24.0f;
        return Normalized;
    }

    inline bool IsOpen(float CurrentHour)
    {
        const float Normalized = NormalizeHour(CurrentHour);
        if (OpeningHour <= ClosingHour)
        {
            return Normalized >= OpeningHour && Normalized < ClosingHour;
        }
        return Normalized >= OpeningHour || Normalized < ClosingHour;
    }

    inline void ResolveNextOpening(int32 CurrentDay, float CurrentHour, int32& OutDay, float& OutHour)
    {
        const float Normalized = NormalizeHour(CurrentHour);
        OutHour = OpeningHour;
        OutDay = Normalized < OpeningHour ? FMath::Max(1, CurrentDay) : FMath::Max(1, CurrentDay + 1);
    }

    inline float HoursUntilNextOpening(int32 CurrentDay, float CurrentHour)
    {
        int32 ReadyDay = CurrentDay;
        float ReadyHour = OpeningHour;
        ResolveNextOpening(CurrentDay, CurrentHour, ReadyDay, ReadyHour);
        return FMath::Max(0.0f,
            static_cast<float>(ReadyDay - CurrentDay) * 24.0f + ReadyHour - NormalizeHour(CurrentHour));
    }

    inline int32 CalculateEmergencyRecoveryTotal(int32 BaseQuote)
    {
        const int32 SafeBase = FMath::Max(0, BaseQuote);
        if (SafeBase <= 0) return 0;
        const int32 Surcharge = FMath::Max(1, FMath::CeilToInt(static_cast<float>(SafeBase) *
            (static_cast<float>(AfterHoursRecoverySurchargePercent) / 100.0f)));
        return SafeBase + Surcharge;
    }

    inline FString FormatHour(float Hour)
    {
        const float Normalized = NormalizeHour(Hour);
        int32 WholeHour = FMath::FloorToInt(Normalized);
        int32 Minute = FMath::RoundToInt((Normalized - static_cast<float>(WholeHour)) * 60.0f);
        if (Minute >= 60)
        {
            Minute = 0;
            WholeHour = (WholeHour + 1) % 24;
        }
        return FString::Printf(TEXT("%02d:%02d"), WholeHour, Minute);
    }

    inline FString GetScheduleText()
    {
        return FString::Printf(TEXT("%s-%s"), *FormatHour(OpeningHour), *FormatHour(ClosingHour));
    }
}
