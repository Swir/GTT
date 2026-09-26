#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "GTT.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
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
        if (!BoundCargoVehicle.IsValid() && !BoundCargoVehicleId.IsNone()) TryRebindBoundVehicle();
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

APawn* UGTTFarmCargoAuthoritySubsystem::ResolveVehicleByPersistentId(FName VehicleId) const
{
    if (VehicleId.IsNone()) return nullptr;
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (Vehicle && Vehicle->GetPersistentVehicleId() == VehicleId) return Vehicle;
    }
    for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (Vehicle && Vehicle->GetPersistentVehicleId() == VehicleId) return Vehicle;
    }
    return nullptr;
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
        GTT_LOG( Error, TEXT("FARM_CARGO_AUTHORITY event=BIND result=FAIL reason=no_vehicle"));
        return false;
    }

    BoundCargoVehicle = LoadedVehicle;
    BoundCargoVehicleId = ResolvePersistentVehicleId(LoadedVehicle);
    bObservedLoadedContract = true;

    OutSummary = FString::Printf(TEXT("CARGO VEHICLE LOCKED: %s must complete every handoff for this load."),
        *BoundCargoVehicleId.ToString());
    GTT_LOG( Display, TEXT("FARM_CARGO_AUTHORITY event=BIND result=PASS vehicle=%s actor=%s"),
        *BoundCargoVehicleId.ToString(), *LoadedVehicle->GetName());
    return true;
}

bool UGTTFarmCargoAuthoritySubsystem::TryRebindBoundVehicle()
{
    if (BoundCargoVehicleId.IsNone()) return false;
    if (BoundCargoVehicle.IsValid() && ResolvePersistentVehicleId(BoundCargoVehicle.Get()) == BoundCargoVehicleId) return true;

    BoundCargoVehicle.Reset();
    APawn* Resolved = ResolveVehicleByPersistentId(BoundCargoVehicleId);
    if (!Resolved)
    {
        GTT_LOG( Warning,
            TEXT("FARM_CARGO_RECOVERY event=REBIND result=WAIT vehicle=%s reason=actor_not_present"),
            *BoundCargoVehicleId.ToString());
        return false;
    }

    BoundCargoVehicle = Resolved;
    if (AGTTFarmJobDirector* Director = Cast<AGTTFarmJobDirector>(
        UGameplayStatics::GetActorOfClass(this, AGTTFarmJobDirector::StaticClass())))
    {
        Director->AdoptRestoredCargoVehicle(Resolved);
    }

    GTT_LOG( Display,
        TEXT("FARM_CARGO_RECOVERY event=REBIND result=PASS vehicle=%s actor=%s"),
        *BoundCargoVehicleId.ToString(), *Resolved->GetName());
    return true;
}

bool UGTTFarmCargoAuthoritySubsystem::ValidateHandoff(
    const FVector& HandoffLocation,
    float MaxDistanceCm,
    float MaxSpeedKmh,
    FString& OutReason)
{
    APawn* Vehicle = BoundCargoVehicle.Get();
    if (!Vehicle && !BoundCargoVehicleId.IsNone())
    {
        TryRebindBoundVehicle();
        Vehicle = BoundCargoVehicle.Get();
    }
    if (!Vehicle)
    {
        OutReason = BoundCargoVehicleId.IsNone()
            ? TEXT("DELIVERY YARD: no cargo vehicle identity is stored for this load. Return to Feed Depot and reload the contract.")
            : FString::Printf(TEXT("DELIVERY YARD: cargo vehicle %s is not available. Recover or recall that exact vehicle before handoff."),
                *BoundCargoVehicleId.ToString());
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
    GTT_LOG( Display,
        TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_CHECK result=PASS vehicle=%s distance_cm=%.1f speed_kmh=%.2f"),
        *BoundCargoVehicleId.ToString(), DistanceCm, SpeedKmh);
    return true;
}

void UGTTFarmCargoAuthoritySubsystem::CaptureToSave(UGTTSaveGame* Save) const
{
    if (!Save) return;
    const uint8 DeliverStage = static_cast<uint8>(EGTTFarmJobStage::DeliverCargo);
    const uint8 FinalStage = static_cast<uint8>(EGTTFarmJobStage::DeliverFinalStop);
    const bool bLoadedStage = Save->bFarmCargoContractActive &&
        (Save->FarmCargoStage == DeliverStage || Save->FarmCargoStage == FinalStage);
    Save->FarmCargoBoundVehicleId = bLoadedStage ? BoundCargoVehicleId : NAME_None;
}

void UGTTFarmCargoAuthoritySubsystem::RestoreFromSave(const UGTTSaveGame* Save)
{
    BoundCargoVehicle.Reset();
    BoundCargoVehicleId = NAME_None;
    bObservedLoadedContract = false;
    if (!Save || !Save->bFarmCargoContractActive) return;

    const uint8 DeliverStage = static_cast<uint8>(EGTTFarmJobStage::DeliverCargo);
    const uint8 FinalStage = static_cast<uint8>(EGTTFarmJobStage::DeliverFinalStop);
    if (Save->FarmCargoStage != DeliverStage && Save->FarmCargoStage != FinalStage) return;

    BoundCargoVehicleId = Save->FarmCargoBoundVehicleId;
    bObservedLoadedContract = !BoundCargoVehicleId.IsNone();
    if (!BoundCargoVehicleId.IsNone()) TryRebindBoundVehicle();
}

void UGTTFarmCargoAuthoritySubsystem::ClearLoadedVehicle(const TCHAR* Reason)
{
    if (BoundCargoVehicle.IsValid() || !BoundCargoVehicleId.IsNone())
    {
        GTT_LOG( Display, TEXT("FARM_CARGO_AUTHORITY event=CLEAR vehicle=%s reason=%s loaded_observed=%d"),
            *BoundCargoVehicleId.ToString(), Reason ? Reason : TEXT("unknown"), bObservedLoadedContract ? 1 : 0);
    }
    BoundCargoVehicle.Reset();
    BoundCargoVehicleId = NAME_None;
    bObservedLoadedContract = false;
}
