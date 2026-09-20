#include "Vehicles/GTTNativeRuntimeTelemetrySubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterChaosMovementComponent.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTNativeAxleTractionSubsystem.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace
{
    constexpr float EvidenceIntervalSeconds = 3.0f;
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));

    const TCHAR* TrailerBrakeThermalStateToken(EGTTTrailerBrakeThermalState State)
    {
        switch (State)
        {
            case EGTTTrailerBrakeThermalState::Hot:
                return TEXT("HOT");
            case EGTTTrailerBrakeThermalState::Fading:
                return TEXT("FADING");
            case EGTTTrailerBrakeThermalState::Critical:
                return TEXT("CRITICAL");
            case EGTTTrailerBrakeThermalState::Normal:
            default:
                return TEXT("NORMAL");
        }
    }

    AGTTFarmTrailer* FindAttachedTrailer(UWorld* World, AGTTFieldmasterNativePawn* Vehicle)
    {
        if (!World || !Vehicle) return nullptr;
        for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
        {
            if (It->IsAttached() && It->GetTowActor() == Vehicle)
            {
                return *It;
            }
        }
        return nullptr;
    }

    AGTTVehicleBase* FindLegacyFieldmasterMirror(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
        {
            if (It->GetPersistentVehicleId() == FieldmasterVehicleId)
            {
                return *It;
            }
        }
        return nullptr;
    }

    bool ValidateNativeAuthority(
        AGTTFieldmasterNativePawn* Vehicle,
        UChaosWheeledVehicleMovementComponent* Movement,
        AGTTVehicleBase* LegacyMirror,
        FString& OutReason)
    {
        if (!Vehicle || !Movement)
        {
            OutReason = TEXT("MISSING_NATIVE_VEHICLE_OR_MOVEMENT");
            return false;
        }
        if (!Cast<UGTTFieldmasterChaosMovementComponent>(Movement))
        {
            OutReason = TEXT("WRONG_MOVEMENT_CLASS");
            return false;
        }
        if (!Movement->IsActive())
        {
            OutReason = TEXT("MOVEMENT_INACTIVE");
            return false;
        }
        if (!Vehicle->GetMesh() || !Vehicle->GetMesh()->GetPhysicsAsset())
        {
            OutReason = TEXT("PHYSICS_ASSET_MISSING");
            return false;
        }
        if (!LegacyMirror)
        {
            OutReason = TEXT("LEGACY_MIRROR_MISSING");
            return false;
        }
        if (!LegacyMirror->IsHidden())
        {
            OutReason = TEXT("LEGACY_MIRROR_VISIBLE");
            return false;
        }
        if (LegacyMirror->GetActorEnableCollision())
        {
            OutReason = TEXT("LEGACY_MIRROR_COLLISION_ENABLED");
            return false;
        }
        if (LegacyMirror->IsActorTickEnabled())
        {
            OutReason = TEXT("LEGACY_MIRROR_TICK_ENABLED");
            return false;
        }
        if (Vehicle->IsHidden())
        {
            OutReason = TEXT("NATIVE_VEHICLE_HIDDEN");
            return false;
        }
        if (!Vehicle->GetActorEnableCollision())
        {
            OutReason = TEXT("NATIVE_COLLISION_DISABLED");
            return false;
        }

        OutReason = TEXT("PASS");
        return true;
    }
}

void UGTTNativeRuntimeTelemetrySubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || DeltaSeconds <= 0.0f) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        SampleFieldmaster(*It, DeltaSeconds);
    }

    for (auto It = EvidenceSecondsByVehicle.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid()) It.RemoveCurrent();
    }
}

TStatId UGTTNativeRuntimeTelemetrySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeRuntimeTelemetrySubsystem, STATGROUP_Tickables);
}

bool UGTTNativeRuntimeTelemetrySubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativeRuntimeTelemetrySubsystem::SampleFieldmaster(AGTTFieldmasterNativePawn* Vehicle, float DeltaSeconds)
{
    if (!Vehicle) return;

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(Vehicle);
    if (!Vehicle->IsLegacyTakeoverActive() || !Vehicle->IsNativeFieldmasterReady())
    {
        EvidenceSecondsByVehicle.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement =
        Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent());
    AGTTVehicleBase* LegacyMirror = FindLegacyFieldmasterMirror(GetWorld());
    FString AuthorityReason;
    if (!ValidateNativeAuthority(Vehicle, Movement, LegacyMirror, AuthorityReason))
    {
        UE_LOG(LogGTT, Error,
            TEXT("NATIVE_FIELDMASTER_AUTHORITY_FAULT vehicle=RustyFieldmaster60 reason=%s action=RESTORE_LEGACY"),
            *AuthorityReason);
        Vehicle->DeactivateLegacyTakeover();
        EvidenceSecondsByVehicle.Remove(Key);
        return;
    }

    UGTTFieldmasterChaosMovementComponent* FieldmasterMovement =
        Cast<UGTTFieldmasterChaosMovementComponent>(Movement);
    if (!FieldmasterMovement)
    {
        UE_LOG(LogGTT, Error,
            TEXT("NATIVE_FIELDMASTER_AUTHORITY_FAULT vehicle=RustyFieldmaster60 reason=FIELDMASTER_MOVEMENT_CAST_FAILED action=RESTORE_LEGACY"));
        Vehicle->DeactivateLegacyTakeover();
        EvidenceSecondsByVehicle.Remove(Key);
        return;
    }

    float& EvidenceSeconds = EvidenceSecondsByVehicle.FindOrAdd(Key);
    EvidenceSeconds += DeltaSeconds;
    if (EvidenceSeconds < EvidenceIntervalSeconds) return;
    EvidenceSeconds = 0.0f;

    const FGTTVehicleMigrationSnapshot Migration = Vehicle->GetMigrationSnapshot();
    FGTTNativeAxleTractionSnapshot AxleSnapshot;
    if (UGTTNativeAxleTractionSubsystem* Axle = GetWorld()->GetSubsystem<UGTTNativeAxleTractionSubsystem>())
    {
        AxleSnapshot = Axle->SampleSnapshot(Movement, Migration.TireIntegrity, Migration.TireUpgradeLevel);
    }

    float MaxSlipMagnitude = 0.0f;
    float MaxSlipAngle = 0.0f;
    const int32 WheelCount = FMath::Min(4, Movement->GetNumWheels());
    for (int32 WheelIndex = 0; WheelIndex < WheelCount; ++WheelIndex)
    {
        const FWheelStatus& Wheel = Movement->GetWheelState(WheelIndex);
        if (!Wheel.bIsValid) continue;
        MaxSlipMagnitude = FMath::Max(MaxSlipMagnitude, FMath::Abs(Wheel.SlipMagnitude));
        MaxSlipAngle = FMath::Max(MaxSlipAngle, FMath::Abs(Wheel.SlipAngle));
    }

    AGTTFarmTrailer* Trailer = FindAttachedTrailer(GetWorld(), Vehicle);
    const float TowLoad = Trailer ? Trailer->GetTowLoadFactor() : 0.0f;
    const float SpeedKmh = Vehicle->GetVelocity().Size() * 0.036f;
    const float SignedSpeedKmh = FVector::DotProduct(Vehicle->GetVelocity(), Vehicle->GetActorForwardVector()) * 0.036f;
    const bool bAutomaticGears = Movement->TransmissionSetup.bUseAutomaticGears;
    const int32 ForwardGearCount = Movement->TransmissionSetup.ForwardGearRatios.Num();

    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_FIELDMASTER_RUNTIME_TELEMETRY vehicle=RustyFieldmaster60 authority=NATIVE_CHAOS takeover_integrity=PASS movement_class=%s legacy_mirror=QUIESCENT legacy_collision=NO legacy_tick=NO native_collision=YES physics_asset=YES movement=ACTIVE speed_kmh=%.2f signed_speed_kmh=%.2f current_gear=%d automatic_gears=%s forward_gears=%d throttle=%.2f brake=%.2f steer=%.2f valid_wheels=%d contacts=%d front_contacts=%d rear_contacts=%d suspension_ready=%s suspension_samples=%d suspension_min=%.3f suspension_max=%.3f front_slip_risk=%.3f rear_slip_risk=%.3f max_slip_magnitude=%.2f max_slip_angle=%.2f left_load=%.3f right_load=%.3f axle_imbalance=%.3f traction_authority=%.3f driver=%s trailer=%s tow_load=%.3f terrain_throttle_authority=%.3f travel_grade_deg=%.2f hill_haul_brake=%.3f hill_hold=%s downhill_tow_brake=%s trailer_brake_heat=%.3f trailer_brake_authority=%.3f trailer_brake_state=%s trailer_brake_fade=%s trailer_brake_cooling=%s runaway_mitigation=%s runaway_brake=%.3f"),
        *Movement->GetClass()->GetName(),
        SpeedKmh,
        SignedSpeedKmh,
        Movement->GetCurrentGear(),
        bAutomaticGears ? TEXT("YES") : TEXT("NO"),
        ForwardGearCount,
        Movement->GetThrottleInput(),
        Movement->GetBrakeInput(),
        Movement->GetSteeringInput(),
        AxleSnapshot.ValidWheels,
        AxleSnapshot.ContactWheels,
        AxleSnapshot.FrontContacts,
        AxleSnapshot.RearContacts,
        AxleSnapshot.bSuspensionRuntimeReady ? TEXT("YES") : TEXT("NO"),
        AxleSnapshot.SuspensionSamples,
        AxleSnapshot.MinSuspensionTravel,
        AxleSnapshot.MaxSuspensionTravel,
        AxleSnapshot.FrontSlipRisk,
        AxleSnapshot.RearSlipRisk,
        MaxSlipMagnitude,
        MaxSlipAngle,
        AxleSnapshot.LeftLoadProxy,
        AxleSnapshot.RightLoadProxy,
        AxleSnapshot.AxleImbalance,
        AxleSnapshot.TractionAuthority,
        Vehicle->IsOccupied() ? TEXT("YES") : TEXT("NO"),
        Trailer ? TEXT("ATTACHED") : TEXT("NONE"),
        TowLoad,
        FieldmasterMovement->GetTerrainThrottleAuthority(),
        FieldmasterMovement->GetTravelGradeDegrees(),
        FieldmasterMovement->GetHillHaulBrake(),
        FieldmasterMovement->IsHillHoldActive() ? TEXT("YES") : TEXT("NO"),
        FieldmasterMovement->IsDownhillTowBrakeActive() ? TEXT("YES") : TEXT("NO"),
        FieldmasterMovement->GetTrailerBrakeHeat01(),
        FieldmasterMovement->GetTrailerBrakeAuthority(),
        TrailerBrakeThermalStateToken(FieldmasterMovement->GetTrailerBrakeThermalState()),
        FieldmasterMovement->IsTrailerBrakeFadeActive() ? TEXT("YES") : TEXT("NO"),
        FieldmasterMovement->IsTrailerBrakeCoolingActive() ? TEXT("YES") : TEXT("NO"),
        FieldmasterMovement->IsTrailerRunawayMitigationActive() ? TEXT("YES") : TEXT("NO"),
        FieldmasterMovement->GetTrailerRunawaySafetyBrake());
}
