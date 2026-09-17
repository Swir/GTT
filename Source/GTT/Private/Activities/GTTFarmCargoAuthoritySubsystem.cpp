#include "Activities/GTTFarmCargoAuthoritySubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
constexpr float CargoPickupRadiusCm = 700.0f;
constexpr float CargoHandoffRadiusCm = 750.0f;
constexpr float CargoHandoffMaxSpeedKmh = 3.0f;
}

void UGTTFarmCargoAuthoritySubsystem::ResetForNewContract()
{
    ClearLoadedVehicle(TEXT("new-contract"));
}

bool UGTTFarmCargoAuthoritySubsystem::CaptureLoadedVehicle(APawn* PlayerPawn)
{
    AActor* Candidate = ResolvePickupCandidate(PlayerPawn);
    if (!IsUsableCargoVehicle(Candidate))
    {
        UE_LOG(LogTemp, Error, TEXT("FARM_CARGO_AUTHORITY event=LOAD_LOCK result=FAIL reason=no-valid-pickup-vehicle"));
        LoadedVehicle.Reset();
        LoadedVehicleId = NAME_None;
        return false;
    }

    LoadedVehicle = Candidate;
    LoadedVehicleId = ResolveVehicleId(Candidate);
    UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_AUTHORITY event=LOAD_LOCK result=PASS vehicle_id=%s actor=%s"),
        *LoadedVehicleId.ToString(), *Candidate->GetName());
    return true;
}

bool UGTTFarmCargoAuthoritySubsystem::ValidateHandoff(const FVector& HandoffLocation, FString& OutFailureReason, float& OutSpeedKmh, float& OutDistanceCm) const
{
    OutFailureReason.Reset();
    OutSpeedKmh = 0.0f;
    OutDistanceCm = -1.0f;

    AActor* CargoVehicle = LoadedVehicle.Get();
    if (!IsUsableCargoVehicle(CargoVehicle))
    {
        OutFailureReason = TEXT("The loaded cargo vehicle is missing or disabled. Bring back the vehicle that actually received the pallets.");
        UE_LOG(LogTemp, Warning, TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_REJECT reason=loaded-vehicle-missing vehicle_id=%s"), *LoadedVehicleId.ToString());
        return false;
    }

    OutDistanceCm = FVector::Dist2D(CargoVehicle->GetActorLocation(), HandoffLocation);
    if (OutDistanceCm > CargoHandoffRadiusCm)
    {
        OutFailureReason = FString::Printf(TEXT("Bring the loaded cargo vehicle into this yard (%.0f m away; must be within %.1f m)."),
            OutDistanceCm / 100.0f, CargoHandoffRadiusCm / 100.0f);
        UE_LOG(LogTemp, Warning, TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_REJECT reason=wrong-vehicle-or-loaded-vehicle-away vehicle_id=%s distance_cm=%.1f"),
            *LoadedVehicleId.ToString(), OutDistanceCm);
        return false;
    }

    OutSpeedKmh = CargoVehicle->GetVelocity().Size2D() * 0.036f;
    if (OutSpeedKmh > CargoHandoffMaxSpeedKmh)
    {
        OutFailureReason = FString::Printf(TEXT("Stop the loaded cargo vehicle before handoff (%.1f km/h; max %.1f)."),
            OutSpeedKmh, CargoHandoffMaxSpeedKmh);
        UE_LOG(LogTemp, Warning, TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_REJECT reason=loaded-vehicle-moving vehicle_id=%s speed_kmh=%.2f distance_cm=%.1f"),
            *LoadedVehicleId.ToString(), OutSpeedKmh, OutDistanceCm);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_READY result=PASS vehicle_id=%s speed_kmh=%.2f distance_cm=%.1f"),
        *LoadedVehicleId.ToString(), OutSpeedKmh, OutDistanceCm);
    return true;
}

void UGTTFarmCargoAuthoritySubsystem::MarkAcceptedHandoff(const TCHAR* StopLabel, bool bContractComplete, float SpeedKmh, float DistanceCm) const
{
    UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_AUTHORITY event=HANDOFF_ACCEPT result=PASS stop=%s vehicle_id=%s speed_kmh=%.2f distance_cm=%.1f contract_complete=%d"),
        StopLabel, *LoadedVehicleId.ToString(), SpeedKmh, DistanceCm, bContractComplete ? 1 : 0);
}

void UGTTFarmCargoAuthoritySubsystem::ClearLoadedVehicle(const TCHAR* Reason)
{
    if (LoadedVehicle.IsValid() || !LoadedVehicleId.IsNone())
    {
        UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_AUTHORITY event=LOAD_LOCK_CLEAR vehicle_id=%s reason=%s"), *LoadedVehicleId.ToString(), Reason);
    }
    LoadedVehicle.Reset();
    LoadedVehicleId = NAME_None;
}

AActor* UGTTFarmCargoAuthoritySubsystem::ResolvePickupCandidate(APawn* PlayerPawn) const
{
    if (!PlayerPawn || !GetWorld()) return nullptr;

    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTMuleboxNativePawn* NativeMulebox = Cast<AGTTMuleboxNativePawn>(ControlledPawn))
    {
        if (IsUsableCargoVehicle(NativeMulebox)) return NativeMulebox;
    }
    if (AGTTVehicleBase* ControlledVehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        if (IsUsableCargoVehicle(ControlledVehicle)) return ControlledVehicle;
    }

    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(CargoPickupRadiusCm);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!IsUsableCargoVehicle(Vehicle)) continue;
        const float DistSq = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Vehicle;
        }
    }
    return Best;
}

bool UGTTFarmCargoAuthoritySubsystem::IsUsableCargoVehicle(const AActor* Candidate) const
{
    if (!Candidate || Candidate->IsHidden()) return false;
    if (const AGTTVehicleBase* Legacy = Cast<AGTTVehicleBase>(Candidate))
    {
        return Legacy->GetConditionPercent() > 0.0f;
    }
    if (const AGTTMuleboxNativePawn* NativeMulebox = Cast<AGTTMuleboxNativePawn>(Candidate))
    {
        return NativeMulebox->IsNativeReady() && NativeMulebox->GetMigrationSnapshot().ConditionPercent > 0.0f;
    }
    return false;
}

FName UGTTFarmCargoAuthoritySubsystem::ResolveVehicleId(const AActor* Candidate)
{
    if (const AGTTVehicleBase* Legacy = Cast<AGTTVehicleBase>(Candidate)) return Legacy->GetPersistentVehicleId();
    if (const AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Candidate)) return Native->GetPersistentVehicleId();
    return NAME_None;
}
