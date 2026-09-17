#include "Vehicles/GTTNativeRuntimeTelemetrySubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTNativeAxleTractionSubsystem.h"
#include "GTT.h"

namespace
{
    constexpr float EvidenceIntervalSeconds = 3.0f;

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
    if (!Movement || !Movement->IsActive()) return;

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
        TEXT("NATIVE_FIELDMASTER_RUNTIME_TELEMETRY vehicle=RustyFieldmaster60 movement=ACTIVE speed_kmh=%.2f signed_speed_kmh=%.2f current_gear=%d automatic_gears=%s forward_gears=%d throttle=%.2f brake=%.2f steer=%.2f valid_wheels=%d contacts=%d front_contacts=%d rear_contacts=%d suspension_ready=%s suspension_samples=%d suspension_min=%.3f suspension_max=%.3f front_slip_risk=%.3f rear_slip_risk=%.3f max_slip_magnitude=%.2f max_slip_angle=%.2f left_load=%.3f right_load=%.3f axle_imbalance=%.3f traction_authority=%.3f driver=%s trailer=%s tow_load=%.3f"),
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
        TowLoad);
}
