#include "Activities/GTTFarmCargoAuthoritySubsystem.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
constexpr float DepotVehicleSearchRadiusCm = 700.0f;

AGTTVehicleBase* FindNearbyLegacyWorkVehicle(const UObject* WorldContextObject, APawn* PlayerPawn, float Radius)
{
    if (!WorldContextObject || !PlayerPawn) return nullptr;

    if (AGTTVehicleBase* Controlled = Cast<AGTTVehicleBase>(UGameplayStatics::GetPlayerPawn(WorldContextObject, 0)))
    {
        if (Controlled->GetConditionPercent() > 0.0f) return Controlled;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return nullptr;

    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || Vehicle->GetConditionPercent() <= 0.0f || Vehicle->IsHidden()) continue;

        const float DistSq = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Vehicle;
        }
    }
    return Best;
}
}

TStatId UGTTFarmCargoAuthoritySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoAuthoritySubsystem, STATGROUP_Tickables);
}

void UGTTFarmCargoAuthoritySubsystem::Tick(float DeltaTime)
{
    (void)DeltaTime;
    if (!BoundCargoVehicle.IsValid() && BoundCargoVehicleId.IsNone()) return;

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    AGTTFarmJobDirector* Director = Cast<AGTTFarmJobDirector>(
        UGameplayStatics::GetActorOfClass(World, AGTTFarmJobDirector::StaticClass()));

    if (!Director || Director->GetStage() == EGTTFarmJobStage::Idle)
    {
        ClearLoadedVehicle(TEXT("contract-idle"));
        return;
    }

    if (Director->GetStage() == EGTTFarmJobStage::DeliverCargo ||
        Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop)
    {
        bObservedLoadedContract = true;
    }
}

APawn* UGTTFarmCargoAuthoritySubsystem::ResolveVehicleLoadedAtDepot(APawn* PlayerPawn) const
{
    if (!PlayerPawn) return nullptr;

    APawn* Controlled = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTMuleboxNativePawn* NativeMulebox = Cast<AGTTMuleboxNativePawn>(Controlled))
    {
        // Mirror AGTTFarmJobDirector::TryPickupCargo exactly: a controlled native Mulebox is
        // the loaded actor. Runtime native readiness remains the existing Chaos acceptance gate.
        return NativeMulebox;
    }

    return FindNearbyLegacyWorkVehicle(this, PlayerPawn, DepotVehicleSearchRadiusCm);
}

FName UGTTFarmCargoAuthoritySubsystem::ResolvePersistentVehicleId(const APawn* Vehicle)
{
    if (const AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Vehicle))
    {
        return Native->GetPersistentVehicleId();
    }
    if (const AGTTVehicleBase* Legacy = Cast<AGTTVehicleBase>(Vehicle))
    {
        return Legacy->GetPersistentVehicleId();
    }
    return Vehicle ? Vehicle->GetFName() : NAME_None;
}

bool UGTTFarmCargoAuthoritySubsystem::BindLoadedVehicle(APawn* PlayerPawn, FString& OutSummary)
{
    APawn* LoadedVehicle = ResolveVehicleLoadedAtDepot(PlayerPawn);
    if (!LoadedVehicle)
    {
        OutSummary = TEXT("CARGO AUTHORITY ERROR: the loaded contract has no resolvable physical vehicle.");
        UE_LOG(LogTemp, Error, TEXT("FARM_CARGO_AUTHORITY event=BIND result=FAIL reason=no_vehicle"));
        return false;
    }

    BoundCargoVehicle = LoadedVehicle;
    BoundCargoVehicleId = ResolvePersistentVehicleId(LoadedVehicle);
    bObservedLoadedContract = true;

    OutSummary = FString::Printf(TEXT("CARGO VEHICLE LOCKED: %s must complete every handoff for this load."),
        *BoundCargoVehicleId.ToString());
    UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_AUTHORITY event=BIND result=PASS vehicle=%s actor=%s"),
        *BoundCargoVehicleId.ToString(), *LoadedVehicle->GetName());
    return true;
}

bool UGTTFarmCargoAuthoritySubsystem::ValidateHandoff(
    const FVector& HandoffLocation,
    float MaxDistanceCm,
    float MaxSpeedKmh,
    FString& OutReason) const
{
    APawn* Vehicle = BoundCargoVehicle.Get();
    if (!Vehicle)
    {
        OutReason = TEXT("DELIVERY YARD: the original cargo vehicle is missing. Return with the vehicle loaded at Feed Depot.");
        return false;
    }

    const float DistanceCm = FVector::Dist2D(Vehicle->GetActorLocation(), HandoffLocation);
    if (DistanceCm > MaxDistanceCm)
    {
        OutReason = FString::Printf(
            TEXT("DELIVERY YARD: bring the original cargo vehicle %s into the handoff zone (%.0f m away)."),
            *BoundCargoVehicleId.ToString(), DistanceCm / 100.0f);
        return false;
    }

    const float SpeedKmh = Vehicle->GetVelocity().Size2D() * 0.036f;
    if (SpeedKmh > MaxSpeedKmh)
    {
        OutReason = FString::Printf(
            TEXT("DELIVERY YARD: stop cargo vehicle %s before handoff (%.1f km/h; max %.1f)."),
            *BoundCargoVehicleId.ToString(), SpeedKmh, MaxSpeedKmh);
        return false;
    }

    OutReason = FString::Printf(
        TEXT("CARGO HANDOFF VERIFIED: %s | distance %.1f m | speed %.1f km/h."),
        *BoundCargoVehicleId.ToString(), DistanceCm / 100.0f, SpeedKmh);
    UE_LOG(LogTemp, Display,
        TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_CHECK result=PASS vehicle=%s distance_cm=%.1f speed_kmh=%.2f"),
        *BoundCargoVehicleId.ToString(), DistanceCm, SpeedKmh);
    return true;
}

void UGTTFarmCargoAuthoritySubsystem::ClearLoadedVehicle(const TCHAR* Reason)
{
    if (BoundCargoVehicle.IsValid() || !BoundCargoVehicleId.IsNone())
    {
        UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_AUTHORITY event=CLEAR vehicle=%s reason=%s loaded_observed=%d"),
            *BoundCargoVehicleId.ToString(), Reason ? Reason : TEXT("unknown"), bObservedLoadedContract ? 1 : 0);
    }
    BoundCargoVehicle.Reset();
    BoundCargoVehicleId = NAME_None;
    bObservedLoadedContract = false;
}
