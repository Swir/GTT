#include "Vehicles/GTTNativeRoadIncidentSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Traffic/GTTTrafficCarPawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    constexpr float IncidentScanIntervalSeconds = 0.10f;
    constexpr float TrafficIncidentRadiusCm = 575.0f;
    constexpr float TrafficCrimeMinimumImpactKmh = 16.0f;
    constexpr float SevereTrafficImpactKmh = 38.0f;
}

void UGTTNativeRoadIncidentSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    InWorld.GetTimerManager().SetTimer(
        IncidentScanTimer,
        this,
        &UGTTNativeRoadIncidentSubsystem::ScanNativeRoadIncidents,
        IncidentScanIntervalSeconds,
        true,
        0.20f);
}

void UGTTNativeRoadIncidentSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IncidentScanTimer);
    }
    LastSeenImpactCounts.Reset();
    Super::Deinitialize();
}

void UGTTNativeRoadIncidentSubsystem::ScanNativeRoadIncidents()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* NativeVehicle = *It;
        if (!NativeVehicle)
        {
            continue;
        }

        const int32 CurrentImpactCount = NativeVehicle->GetNativeImpactCount();
        int32& LastSeenCount = LastSeenImpactCounts.FindOrAdd(NativeVehicle);

        if (!NativeVehicle->IsLegacyTakeoverActive() || !NativeVehicle->GetDriverPawn())
        {
            LastSeenCount = CurrentImpactCount;
            continue;
        }

        if (CurrentImpactCount <= LastSeenCount)
        {
            continue;
        }

        LastSeenCount = CurrentImpactCount;
        const float ImpactSpeedKmh = NativeVehicle->GetLastImpactSpeedKmh();
        if (ImpactSpeedKmh < TrafficCrimeMinimumImpactKmh)
        {
            continue;
        }

        AGTTTrafficCarPawn* ClosestTraffic = nullptr;
        float ClosestDistanceSq = FMath::Square(TrafficIncidentRadiusCm);
        for (TActorIterator<AGTTTrafficCarPawn> TrafficIt(World); TrafficIt; ++TrafficIt)
        {
            AGTTTrafficCarPawn* TrafficCar = *TrafficIt;
            if (!TrafficCar || TrafficCar->IsActorHiddenInGame())
            {
                continue;
            }

            const float DistanceSq = FVector::DistSquared(NativeVehicle->GetActorLocation(), TrafficCar->GetActorLocation());
            if (DistanceSq <= ClosestDistanceSq)
            {
                ClosestDistanceSq = DistanceSq;
                ClosestTraffic = TrafficCar;
            }
        }

        if (!ClosestTraffic)
        {
            UE_LOG(LogGTT, Verbose,
                TEXT("NATIVE_ROAD_INCIDENT_NO_TRAFFIC vehicle=%s impact_speed_kmh=%.1f"),
                *NativeVehicle->GetPersistentVehicleId().ToString(), ImpactSpeedKmh);
            continue;
        }

        const float Severity = FMath::Clamp((ImpactSpeedKmh - TrafficCrimeMinimumImpactKmh) / 64.0f, 0.0f, 1.25f);
        const float VictimBodyDamage = FMath::Clamp(4.0f + Severity * 19.0f, 4.0f, 28.0f);
        ClosestTraffic->ApplyVehicleDamage(VictimBodyDamage);

        float VictimTireDamage = 0.0f;
        if (ImpactSpeedKmh >= SevereTrafficImpactKmh)
        {
            VictimTireDamage = FMath::Clamp(0.025f + Severity * 0.075f, 0.025f, 0.12f);
            ClosestTraffic->ApplyTireDamage(VictimTireDamage);
        }

        APawn* DriverPawn = NativeVehicle->GetDriverPawn();
        UGTTWantedComponent* Wanted = DriverPawn ? DriverPawn->FindComponentByClass<UGTTWantedComponent>() : nullptr;
        const float CargoHeat = NativeVehicle->GetCargoLoadFactor() * 2.5f;
        const float CrimeHeat = FMath::Clamp(5.0f + Severity * 17.0f + CargoHeat, 5.0f, 30.0f);
        if (Wanted)
        {
            Wanted->AddHeat(CrimeHeat);
        }

        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROAD_TRAFFIC_INCIDENT vehicle=%s victim=%s impact_speed_kmh=%.1f victim_damage=%.1f victim_tire_damage=%.3f cargo=%.2f"),
            *NativeVehicle->GetPersistentVehicleId().ToString(),
            *ClosestTraffic->GetName(),
            ImpactSpeedKmh,
            VictimBodyDamage,
            VictimTireDamage,
            NativeVehicle->GetCargoLoadFactor());

        if (Wanted)
        {
            UE_LOG(LogGTT, Warning,
                TEXT("NATIVE_ROAD_CRIME_ESCALATION vehicle=%s heat_added=%.1f wanted_level=%d total_heat=%.1f"),
                *NativeVehicle->GetPersistentVehicleId().ToString(),
                CrimeHeat,
                Wanted->GetWantedLevel(),
                Wanted->GetHeat());
        }
    }
}
