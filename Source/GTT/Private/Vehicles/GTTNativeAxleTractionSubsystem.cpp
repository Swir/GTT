#include "Vehicles/GTTNativeAxleTractionSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "WheeledVehiclePawn.h"
#include "GTT.h"

namespace
{
    constexpr float EvidenceIntervalSeconds = 3.0f;
    constexpr float InterventionLogCooldownSeconds = 1.0f;
    constexpr float ModerateRiskThreshold = 0.38f;
    constexpr float SevereRiskThreshold = 0.68f;
    constexpr float TorqueCutRiskThreshold = 0.82f;
    constexpr float ModerateBrakeAssist = 0.12f;
    constexpr float SevereBrakeAssist = 0.30f;

    float WheelSlipRisk(const FWheelStatus& Wheel)
    {
        if (!Wheel.bIsValid) return 1.0f;
        if (!Wheel.bInContact) return 0.72f;
        const float SlipMagnitudeRisk = FMath::Clamp(FMath::Abs(Wheel.SlipMagnitude) / 650.0f, 0.0f, 1.0f);
        const float SlipAngleRisk = FMath::Clamp(FMath::Abs(Wheel.SlipAngle) / 32.0f, 0.0f, 1.0f);
        const float SlipFlagRisk = Wheel.bIsSlipping ? 0.62f : 0.0f;
        const float SkidFlagRisk = Wheel.bIsSkidding ? 0.82f : 0.0f;
        return FMath::Max(FMath::Max(SlipMagnitudeRisk, SlipAngleRisk), FMath::Max(SlipFlagRisk, SkidFlagRisk));
    }

    float WheelLoadProxy(const FWheelStatus& Wheel)
    {
        if (!Wheel.bIsValid || !Wheel.bInContact || Wheel.NormalizedSuspensionLength < 0.0f) return 0.0f;
        return FMath::Clamp(1.0f - Wheel.NormalizedSuspensionLength, 0.0f, 1.0f);
    }
}

TStatId UGTTNativeAxleTractionSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeAxleTractionSubsystem, STATGROUP_Tickables);
}

bool UGTTNativeAxleTractionSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativeAxleTractionSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || DeltaSeconds <= 0.0f) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        AGTTFieldmasterNativePawn* Vehicle = *It;
        UChaosWheeledVehicleMovementComponent* Movement = Vehicle ? Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent()) : nullptr;
        const FGTTVehicleMigrationSnapshot Migration = Vehicle ? Vehicle->GetMigrationSnapshot() : FGTTVehicleMigrationSnapshot();
        EvaluateVehicle(Vehicle, Movement, Vehicle ? Vehicle->GetPersistentVehicleId() : NAME_None,
            Vehicle && Vehicle->IsNativeFieldmasterReady() && Vehicle->IsLegacyTakeoverActive(),
            Vehicle && Vehicle->IsOccupied(), Migration.TireIntegrity, Migration.TireUpgradeLevel, DeltaSeconds);
    }

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        UChaosWheeledVehicleMovementComponent* Movement = Vehicle ? Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent()) : nullptr;
        const FGTTRoadVehicleMigrationSnapshot Migration = Vehicle ? Vehicle->GetMigrationSnapshot() : FGTTRoadVehicleMigrationSnapshot();
        EvaluateVehicle(Vehicle, Movement, Vehicle ? Vehicle->GetPersistentVehicleId() : NAME_None,
            Vehicle && Vehicle->IsNativeReady() && Vehicle->IsLegacyTakeoverActive(),
            Vehicle && Vehicle->GetDriverPawn() != nullptr, Migration.TireIntegrity, Migration.TireUpgradeLevel, DeltaSeconds);
    }

    for (auto It = RuntimeByVehicle.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid()) It.RemoveCurrent();
    }
}

FGTTNativeAxleTractionSnapshot UGTTNativeAxleTractionSubsystem::GetSnapshot(const AWheeledVehiclePawn* Vehicle) const
{
    if (!Vehicle) return FGTTNativeAxleTractionSnapshot();
    const TWeakObjectPtr<AWheeledVehiclePawn> Key(const_cast<AWheeledVehiclePawn*>(Vehicle));
    const FRuntimeState* State = RuntimeByVehicle.Find(Key);
    return State ? State->Snapshot : FGTTNativeAxleTractionSnapshot();
}

FGTTNativeAxleTractionSnapshot UGTTNativeAxleTractionSubsystem::SampleSnapshot(
    UChaosWheeledVehicleMovementComponent* Movement, float TireIntegrity, int32 TireUpgradeLevel) const
{
    FGTTNativeAxleTractionSnapshot Snapshot;
    if (!Movement || !Movement->IsActive() || Movement->GetNumWheels() < 4) return Snapshot;

    Snapshot.bActive = true;
    float FrontSlipTotal = 0.0f, RearSlipTotal = 0.0f, LeftLoadTotal = 0.0f, RightLoadTotal = 0.0f;
    float MinSuspension = 1.0f, MaxSuspension = 0.0f;

    const int32 WheelCount = FMath::Min(4, Movement->GetNumWheels());
    for (int32 WheelIndex = 0; WheelIndex < WheelCount; ++WheelIndex)
    {
        const FWheelStatus& Wheel = Movement->GetWheelState(WheelIndex);
        if (Wheel.bIsValid) ++Snapshot.ValidWheels;
        if (Wheel.bIsValid && Wheel.bInContact)
        {
            ++Snapshot.ContactWheels;
            if (WheelIndex < 2) ++Snapshot.FrontContacts; else ++Snapshot.RearContacts;
            if ((WheelIndex % 2) == 0) ++Snapshot.LeftContacts; else ++Snapshot.RightContacts;
        }

        if (Wheel.bIsValid && Wheel.NormalizedSuspensionLength >= 0.0f)
        {
            const float Travel = FMath::Clamp(Wheel.NormalizedSuspensionLength, 0.0f, 1.0f);
            ++Snapshot.SuspensionSamples;
            MinSuspension = FMath::Min(MinSuspension, Travel);
            MaxSuspension = FMath::Max(MaxSuspension, Travel);
        }

        const float SlipRisk = WheelSlipRisk(Wheel);
        if (WheelIndex < 2) FrontSlipTotal += SlipRisk; else RearSlipTotal += SlipRisk;
        const float LoadProxy = WheelLoadProxy(Wheel);
        if ((WheelIndex % 2) == 0) LeftLoadTotal += LoadProxy; else RightLoadTotal += LoadProxy;
    }

    Snapshot.MinSuspensionTravel = Snapshot.SuspensionSamples > 0 ? MinSuspension : 0.0f;
    Snapshot.MaxSuspensionTravel = Snapshot.SuspensionSamples > 0 ? MaxSuspension : 0.0f;
    Snapshot.bSuspensionRuntimeReady = Snapshot.ValidWheels == 4 && Snapshot.SuspensionSamples == 4;
    Snapshot.FrontSlipRisk = FMath::Clamp(FrontSlipTotal * 0.5f, 0.0f, 1.0f);
    Snapshot.RearSlipRisk = FMath::Clamp(RearSlipTotal * 0.5f, 0.0f, 1.0f);
    Snapshot.LeftLoadProxy = FMath::Clamp(LeftLoadTotal * 0.5f, 0.0f, 1.0f);
    Snapshot.RightLoadProxy = FMath::Clamp(RightLoadTotal * 0.5f, 0.0f, 1.0f);
    Snapshot.AxleImbalance = FMath::Clamp(FMath::Abs(Snapshot.LeftLoadProxy - Snapshot.RightLoadProxy), 0.0f, 1.0f);

    const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(Snapshot.ContactWheels)) / 3.0f, 0.0f, 1.0f);
    const float AxleContactRisk = (Snapshot.FrontContacts == 0 || Snapshot.RearContacts == 0) ? 0.88f : 0.0f;
    const float SlipRisk = FMath::Max(Snapshot.FrontSlipRisk, Snapshot.RearSlipRisk);
    const float TirePenalty = 1.0f - FMath::Clamp(TireIntegrity, 0.0f, 1.0f);
    const float UpgradeRelief = FMath::Clamp(static_cast<float>(FMath::Clamp(TireUpgradeLevel, 0, 3)) * 0.04f, 0.0f, 0.12f);
    const float CombinedRisk = FMath::Clamp(FMath::Max(FMath::Max(ContactRisk, AxleContactRisk), FMath::Max(SlipRisk, Snapshot.AxleImbalance * 0.90f)) + TirePenalty * 0.18f - UpgradeRelief, 0.0f, 1.0f);
    Snapshot.TractionAuthority = FMath::Clamp(1.0f - CombinedRisk, 0.0f, 1.0f);
    Snapshot.bTorqueCut = CombinedRisk >= TorqueCutRiskThreshold || Snapshot.ContactWheels <= 1;
    Snapshot.BrakeAssist = CombinedRisk >= SevereRiskThreshold ? SevereBrakeAssist : (CombinedRisk >= ModerateRiskThreshold ? ModerateBrakeAssist : 0.0f);
    return Snapshot;
}

void UGTTNativeAxleTractionSubsystem::EvaluateVehicle(
    AWheeledVehiclePawn* Vehicle, UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId,
    bool bNativeTakeoverActive, bool bDriverPresent, float TireIntegrity, int32 TireUpgradeLevel, float DeltaSeconds)
{
    if (!Vehicle) return;
    FRuntimeState& State = RuntimeByVehicle.FindOrAdd(Vehicle);
    State.EvidenceSeconds += DeltaSeconds;
    State.InterventionCooldown = FMath::Max(0.0f, State.InterventionCooldown - DeltaSeconds);

    if (!bNativeTakeoverActive || !bDriverPresent || !Movement || !Movement->IsActive())
    {
        State.Snapshot = FGTTNativeAxleTractionSnapshot();
        return;
    }

    State.Snapshot = SampleSnapshot(Movement, TireIntegrity, TireUpgradeLevel);
    const FGTTNativeAxleTractionSnapshot& Snapshot = State.Snapshot;
    if (!Snapshot.bActive) return;

    const float SpeedKmh = Vehicle->GetVelocity().Size() * 0.036f;
    const float Risk = 1.0f - Snapshot.TractionAuthority;
    if ((Snapshot.bTorqueCut || Snapshot.BrakeAssist > 0.0f) && State.InterventionCooldown <= 0.0f)
    {
        State.InterventionCooldown = InterventionLogCooldownSeconds;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_AXLE_TRACTION_RECOMMENDATION vehicle=%s speed_kmh=%.1f risk=%.2f torque_cut=%s brake=%.2f contacts=%d suspension_ready=%s"),
            *VehicleId.ToString(), SpeedKmh, Risk, Snapshot.bTorqueCut ? TEXT("YES") : TEXT("NO"), Snapshot.BrakeAssist,
            Snapshot.ContactWheels, Snapshot.bSuspensionRuntimeReady ? TEXT("YES") : TEXT("NO"));
    }

    if (State.EvidenceSeconds >= EvidenceIntervalSeconds)
    {
        State.EvidenceSeconds = 0.0f;
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_AXLE_TRACTION_EVIDENCE vehicle=%s valid=%d contacts=%d front=%d rear=%d left=%d right=%d front_slip=%.2f rear_slip=%.2f imbalance=%.2f authority=%.2f torque_cut=%s brake=%.2f suspension_ready=%s suspension_samples=%d suspension_range=%.2f..%.2f tire=%.2f tire_level=%d"),
            *VehicleId.ToString(), Snapshot.ValidWheels, Snapshot.ContactWheels, Snapshot.FrontContacts, Snapshot.RearContacts,
            Snapshot.LeftContacts, Snapshot.RightContacts, Snapshot.FrontSlipRisk, Snapshot.RearSlipRisk, Snapshot.AxleImbalance,
            Snapshot.TractionAuthority, Snapshot.bTorqueCut ? TEXT("YES") : TEXT("NO"), Snapshot.BrakeAssist,
            Snapshot.bSuspensionRuntimeReady ? TEXT("YES") : TEXT("NO"), Snapshot.SuspensionSamples,
            Snapshot.MinSuspensionTravel, Snapshot.MaxSuspensionTravel, TireIntegrity, TireUpgradeLevel);
    }
}
