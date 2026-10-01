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
    constexpr float NearbyReactionRadiusCm = 1800.0f;
    constexpr float HitAndRunEscapeRadiusCm = 1700.0f;
    constexpr float HitAndRunWindowSeconds = 7.0f;
}

void UGTTNativeRoadIncidentSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    InWorld.GetTimerManager().SetTimer(IncidentScanTimer, this,
        &UGTTNativeRoadIncidentSubsystem::ScanNativeRoadIncidents,
        IncidentScanIntervalSeconds, true, 0.20f);
}

void UGTTNativeRoadIncidentSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(IncidentScanTimer);
    }
    LastSeenImpactCounts.Reset();
    ActiveIncidents.Reset();
    Super::Deinitialize();
}

void UGTTNativeRoadIncidentSubsystem::UpdateActiveIncidents(float NowSeconds)
{
    for (int32 Index = ActiveIncidents.Num() - 1; Index >= 0; --Index)
    {
        FActiveTrafficIncident& Incident = ActiveIncidents[Index];
        AGTTRoadVehicleNativePawn* NativeVehicle = Incident.NativeVehicle.Get();
        AGTTTrafficCarPawn* Victim = Incident.Victim.Get();
        if (!NativeVehicle || !Victim || NowSeconds >= Incident.ExpiresAtSeconds)
        {
            ActiveIncidents.RemoveAtSwap(Index);
            continue;
        }

        if (Incident.bHitAndRunEscalated || !NativeVehicle->IsLegacyTakeoverActive() || !NativeVehicle->GetDriverPawn())
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared2D(NativeVehicle->GetActorLocation(), Incident.Origin);
        if (DistanceSq < FMath::Square(HitAndRunEscapeRadiusCm))
        {
            continue;
        }

        APawn* DriverPawn = NativeVehicle->GetDriverPawn();
        UGTTWantedComponent* Wanted = DriverPawn ? DriverPawn->FindComponentByClass<UGTTWantedComponent>() : nullptr;
        if (!Wanted)
        {
            continue;
        }

        const float EscapeHeat = FMath::Clamp(6.0f + Incident.ImpactSpeedKmh * 0.12f, 7.0f, 18.0f);
        Wanted->AddHeat(EscapeHeat);
        Incident.bHitAndRunEscalated = true;
        GTT_LOG( Warning,
            TEXT("NATIVE_ROAD_HIT_AND_RUN vehicle=%s victim=%s escape_distance=%.0f heat_added=%.1f wanted_level=%d"),
            *NativeVehicle->GetPersistentVehicleId().ToString(), *Victim->GetName(),
            FMath::Sqrt(DistanceSq), EscapeHeat, Wanted->GetWantedLevel());
    }
}

void UGTTNativeRoadIncidentSubsystem::ScanNativeRoadIncidents()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float NowSeconds = World->GetTimeSeconds();
    UpdateActiveIncidents(NowSeconds);

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
            if (!TrafficCar || TrafficCar->IsHidden())
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
            GTT_LOG( Verbose, TEXT("NATIVE_ROAD_INCIDENT_NO_TRAFFIC vehicle=%s impact_speed_kmh=%.1f"),
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

        ClosestTraffic->RegisterCollisionIncident(ImpactSpeedKmh, NativeVehicle->GetActorLocation());

        int32 NearbyReactors = 0;
        for (TActorIterator<AGTTTrafficCarPawn> TrafficIt(World); TrafficIt; ++TrafficIt)
        {
            AGTTTrafficCarPawn* TrafficCar = *TrafficIt;
            if (!TrafficCar || TrafficCar == ClosestTraffic || TrafficCar->IsHidden())
            {
                continue;
            }
            if (FVector::DistSquared2D(TrafficCar->GetActorLocation(), ClosestTraffic->GetActorLocation()) <= FMath::Square(NearbyReactionRadiusCm))
            {
                TrafficCar->ReactToNearbyIncident(ClosestTraffic->GetActorLocation(), FMath::Clamp(Severity, 0.0f, 1.0f));
                ++NearbyReactors;
            }
        }

        APawn* DriverPawn = NativeVehicle->GetDriverPawn();
        UGTTWantedComponent* Wanted = DriverPawn ? DriverPawn->FindComponentByClass<UGTTWantedComponent>() : nullptr;
        const float CargoHeat = NativeVehicle->GetCargoLoadFactor() * 2.5f;
        const float CrimeHeat = FMath::Clamp(5.0f + Severity * 17.0f + CargoHeat, 5.0f, 30.0f);
        if (Wanted)
        {
            Wanted->AddHeat(CrimeHeat);
        }

        FActiveTrafficIncident ActiveIncident;
        ActiveIncident.NativeVehicle = NativeVehicle;
        ActiveIncident.Victim = ClosestTraffic;
        ActiveIncident.Origin = ClosestTraffic->GetActorLocation();
        ActiveIncident.ImpactSpeedKmh = ImpactSpeedKmh;
        ActiveIncident.ExpiresAtSeconds = NowSeconds + HitAndRunWindowSeconds;
        ActiveIncidents.Add(ActiveIncident);

        const FVector LocalVictim = NativeVehicle->GetActorTransform().InverseTransformPosition(ClosestTraffic->GetActorLocation());
        const TCHAR* ImpactZone = FMath::Abs(LocalVictim.X) >= FMath::Abs(LocalVictim.Y)
            ? (LocalVictim.X >= 0.0f ? TEXT("FRONT") : TEXT("REAR"))
            : (LocalVictim.Y >= 0.0f ? TEXT("RIGHT") : TEXT("LEFT"));

        GTT_LOG( Warning,
            TEXT("NATIVE_ROAD_TRAFFIC_INCIDENT vehicle=%s victim=%s zone=%s impact_speed_kmh=%.1f victim_damage=%.1f victim_tire_damage=%.3f nearby_reactors=%d cargo=%.2f"),
            *NativeVehicle->GetPersistentVehicleId().ToString(), *ClosestTraffic->GetName(), ImpactZone,
            ImpactSpeedKmh, VictimBodyDamage, VictimTireDamage, NearbyReactors, NativeVehicle->GetCargoLoadFactor());

        if (Wanted)
        {
            GTT_LOG( Warning,
                TEXT("NATIVE_ROAD_CRIME_ESCALATION vehicle=%s heat_added=%.1f wanted_level=%d total_heat=%.1f"),
                *NativeVehicle->GetPersistentVehicleId().ToString(), CrimeHeat,
                Wanted->GetWantedLevel(), Wanted->GetHeat());
        }
    }
}
