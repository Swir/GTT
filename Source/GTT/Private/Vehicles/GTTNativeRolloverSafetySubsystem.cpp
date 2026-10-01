#include "Vehicles/GTTNativeRolloverSafetySubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "WheeledVehiclePawn.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr float EvidenceIntervalSeconds = 3.0f;

    FString NativeVehicleLabel(const AWheeledVehiclePawn* Vehicle)
    {
        if (const AGTTFieldmasterNativePawn* Fieldmaster = Cast<AGTTFieldmasterNativePawn>(Vehicle))
            return Fieldmaster->GetPersistentVehicleId().ToString();
        if (const AGTTRoadVehicleNativePawn* RoadVehicle = Cast<AGTTRoadVehicleNativePawn>(Vehicle))
            return RoadVehicle->GetPersistentVehicleId().ToString();
        return Vehicle ? Vehicle->GetName() : TEXT("NONE");
    }
}

TStatId UGTTNativeRolloverSafetySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeRolloverSafetySubsystem, STATGROUP_Tickables);
}

void UGTTNativeRolloverSafetySubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || DeltaSeconds <= 0.0f) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* Vehicle = *It;
        const FGTTVehicleMigrationSnapshot Migration = Vehicle->GetMigrationSnapshot();
        EvaluateVehicle(Vehicle,
            Vehicle->IsNativeFieldmasterReady() && Vehicle->IsLegacyTakeoverActive(),
            Vehicle->IsOccupied(),
            Migration.TireIntegrity,
            DeltaSeconds);
    }

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        const FGTTRoadVehicleMigrationSnapshot Migration = Vehicle->GetMigrationSnapshot();
        EvaluateVehicle(Vehicle,
            Vehicle->IsNativeReady() && Vehicle->IsLegacyTakeoverActive(),
            Vehicle->GetDriverPawn() != nullptr,
            Migration.TireIntegrity,
            DeltaSeconds);
    }

    for (auto It = RuntimeByVehicle.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid()) It.RemoveCurrent();
    }
}

FGTTNativeRolloverSnapshot UGTTNativeRolloverSafetySubsystem::GetSnapshot(const AWheeledVehiclePawn* Vehicle) const
{
    if (!Vehicle) return FGTTNativeRolloverSnapshot();
    const TWeakObjectPtr<AWheeledVehiclePawn> Key(const_cast<AWheeledVehiclePawn*>(Vehicle));
    const FRuntimeState* State = RuntimeByVehicle.Find(Key);
    return State ? State->Snapshot : FGTTNativeRolloverSnapshot();
}

int32 UGTTNativeRolloverSafetySubsystem::CountNativeWheelContacts(AWheeledVehiclePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent());
    if (!Movement || Movement->GetNumWheels() <= 0) return 0;

    int32 Contacts = 0;
    const int32 Wheels = Movement->GetNumWheels();
    for (int32 WheelIndex = 0; WheelIndex < Wheels; ++WheelIndex)
    {
        const FWheelStatus& WheelState = Movement->GetWheelState(WheelIndex);
        if (WheelState.bIsValid && WheelState.bInContact) ++Contacts;
    }
    return Contacts;
}

void UGTTNativeRolloverSafetySubsystem::EvaluateVehicle(AWheeledVehiclePawn* Vehicle, bool bTakeoverActive, bool bDriverPresent, float TireIntegrity, float DeltaSeconds)
{
    if (!Vehicle) return;

    FRuntimeState& State = RuntimeByVehicle.FindOrAdd(Vehicle);
    State.EvidenceCooldown = FMath::Max(0.0f, State.EvidenceCooldown - DeltaSeconds);
    State.EmergencyCooldown = FMath::Max(0.0f, State.EmergencyCooldown - DeltaSeconds);
    State.RecoveryPulseRemaining = FMath::Max(0.0f, State.RecoveryPulseRemaining - DeltaSeconds);

    FGTTNativeRolloverSnapshot Snapshot;
    Snapshot.bActive = bTakeoverActive;

    if (!bTakeoverActive)
    {
        State.TippedAccumulator = 0.0f;
        State.RecoveryPulseRemaining = 0.0f;
        State.Snapshot = Snapshot;
        return;
    }

    USkeletalMeshComponent* Body = Vehicle->GetMesh();
    if (!Body || !Body->IsSimulatingPhysics())
    {
        State.TippedAccumulator = 0.0f;
        State.Snapshot = Snapshot;
        return;
    }

    Snapshot.WheelContacts = CountNativeWheelContacts(Vehicle);
    Snapshot.SpeedKmh = Vehicle->GetVelocity().Size() * 0.036f;
    const float UpDot = FMath::Clamp(FVector::DotProduct(Vehicle->GetActorUpVector(), FVector::UpVector), -1.0f, 1.0f);
    Snapshot.TiltAngleDeg = FMath::RadiansToDegrees(FMath::Acos(UpDot));

    const float TireAuthority = FMath::Lerp(0.62f, 1.0f, FMath::Clamp(TireIntegrity, 0.0f, 1.0f));
    const bool bGroundedEnough = Snapshot.WheelContacts >= 2;
    const bool bInAntiRollWindow = Snapshot.TiltAngleDeg >= AntiRollStartAngleDeg && Snapshot.TiltAngleDeg < EmergencyTiltAngleDeg;
    const bool bMovingEnough = Snapshot.SpeedKmh >= AntiRollMinSpeedKmh;

    if (bGroundedEnough && bInAntiRollWindow && bMovingEnough)
    {
        const float AngleAlpha = FMath::Clamp((Snapshot.TiltAngleDeg - AntiRollStartAngleDeg) /
            FMath::Max(1.0f, AntiRollFullAngleDeg - AntiRollStartAngleDeg), 0.0f, 1.0f);
        const float SpeedAlpha = FMath::Clamp((Snapshot.SpeedKmh - AntiRollMinSpeedKmh) / 35.0f, 0.15f, 1.0f);
        Snapshot.CorrectionStrength = AngleAlpha * SpeedAlpha * TireAuthority;

        FVector RightingAxis = FVector::CrossProduct(Vehicle->GetActorUpVector(), FVector::UpVector);
        if (!RightingAxis.IsNearlyZero())
        {
            RightingAxis.Normalize();
            Body->AddTorqueInRadians(RightingAxis * AntiRollTorque * Snapshot.CorrectionStrength, NAME_None, true);
        }
    }

    const bool bEmergencyPose = Snapshot.TiltAngleDeg >= EmergencyTiltAngleDeg && Snapshot.SpeedKmh <= EmergencyMaxSpeedKmh;
    if (bEmergencyPose && bDriverPresent)
        State.TippedAccumulator = FMath::Min(6.0f, State.TippedAccumulator + DeltaSeconds);
    else
        State.TippedAccumulator = 0.0f;

    Snapshot.TippedSeconds = State.TippedAccumulator;

    if (bEmergencyPose && bDriverPresent && State.TippedAccumulator >= EmergencyArmSeconds &&
        State.EmergencyCooldown <= 0.0f && State.RecoveryPulseRemaining <= 0.0f)
    {
        State.RecoveryPulseRemaining = EmergencyRecoverySeconds;
        State.EmergencyCooldown = EmergencyCooldownSeconds;
        GTT_LOG( Warning,
            TEXT("NATIVE_FLEET_EMERGENCY_RIGHTING vehicle=%s tilt=%.1f speed=%.1f contacts=%d tire_integrity=%.2f"),
            *NativeVehicleLabel(Vehicle), Snapshot.TiltAngleDeg, Snapshot.SpeedKmh, Snapshot.WheelContacts, TireIntegrity);
    }

    if (State.RecoveryPulseRemaining > 0.0f && bDriverPresent)
    {
        Snapshot.bEmergencyRighting = true;
        FVector RightingAxis = FVector::CrossProduct(Vehicle->GetActorUpVector(), FVector::UpVector);
        if (RightingAxis.IsNearlyZero()) RightingAxis = Vehicle->GetActorForwardVector();
        RightingAxis.Normalize();
        Body->AddTorqueInRadians(RightingAxis * EmergencyRightingTorque * TireAuthority, NAME_None, true);
        Body->AddForce(FVector::UpVector * EmergencyLiftForce, NAME_None, true);
    }

    State.Snapshot = Snapshot;
    if (State.EvidenceCooldown <= 0.0f)
    {
        State.EvidenceCooldown = EvidenceIntervalSeconds;
        EmitEvidence(Vehicle, State);
    }
}

void UGTTNativeRolloverSafetySubsystem::EmitEvidence(AWheeledVehiclePawn* Vehicle, FRuntimeState& State) const
{
    const FGTTNativeRolloverSnapshot& Snapshot = State.Snapshot;
    GTT_LOG( Log,
        TEXT("NATIVE_FLEET_ROLLOVER_EVIDENCE vehicle=%s active=%d contacts=%d speed_kmh=%.1f tilt=%.1f correction=%.2f tipped=%.2f emergency=%d cooldown=%.2f"),
        *NativeVehicleLabel(Vehicle), Snapshot.bActive ? 1 : 0, Snapshot.WheelContacts, Snapshot.SpeedKmh,
        Snapshot.TiltAngleDeg, Snapshot.CorrectionStrength, Snapshot.TippedSeconds,
        Snapshot.bEmergencyRighting ? 1 : 0, State.EmergencyCooldown);
}
