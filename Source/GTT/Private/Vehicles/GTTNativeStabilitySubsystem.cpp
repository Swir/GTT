#include "Vehicles/GTTNativeStabilitySubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float ProbeStartLiftCm = 30.0f;
    constexpr float ProbeDepthCm = 155.0f;
    constexpr float LowContactGraceSeconds = 0.28f;
    constexpr float EvidenceIntervalSeconds = 4.0f;
    constexpr float InterventionSpeedKmh = 14.0f;
    constexpr float SevereRollDegrees = 30.0f;
    constexpr float SeverePitchDegrees = 32.0f;
}

void UGTTNativeStabilitySubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        EvaluateFieldmaster(*It, DeltaTime);
    }
}

TStatId UGTTNativeStabilitySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeStabilitySubsystem, STATGROUP_Tickables);
}

bool UGTTNativeStabilitySubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativeStabilitySubsystem::EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn)
    {
        return;
    }

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive() || !NativePawn->IsNativeFieldmasterReady())
    {
        StabilityStates.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive())
    {
        return;
    }

    FStabilityState& State = StabilityStates.FindOrAdd(Key);
    TArray<float> ClearancesCm;
    const int32 Contacts = SampleWheelContacts(NativePawn, ClearancesCm);
    const float SpeedKmh = NativePawn->GetVelocity().Size() * 0.036f;
    const float Risk = ComputeStabilityRisk(NativePawn, Contacts, SpeedKmh);
    const FGTTVehicleMigrationSnapshot VehicleState = NativePawn->GetMigrationSnapshot();
    const int32 TireLevel = FMath::Clamp(VehicleState.TireUpgradeLevel, 0, 3);

    State.LastContacts = Contacts;
    State.LastRisk = Risk;

    if (Contacts <= 2 && SpeedKmh >= InterventionSpeedKmh)
    {
        State.LowContactSeconds += DeltaTime;
    }
    else
    {
        State.LowContactSeconds = 0.0f;
    }

    const FRotator Rotation = NativePawn->GetActorRotation();
    const float AbsRoll = FMath::Abs(FMath::UnwindDegrees(Rotation.Roll));
    const float AbsPitch = FMath::Abs(FMath::UnwindDegrees(Rotation.Pitch));
    const bool bSevereAttitude = AbsRoll >= SevereRollDegrees || AbsPitch >= SeverePitchDegrees;
    const bool bSustainedLowContact = State.LowContactSeconds >= LowContactGraceSeconds;
    const bool bIntervention = SpeedKmh >= InterventionSpeedKmh && (Risk >= 0.58f || bSustainedLowContact || bSevereAttitude);

    float AppliedBrake = 0.0f;
    if (bIntervention)
    {
        const float TireAssist = TireLevel * 0.03f;
        AppliedBrake = FMath::Clamp(0.18f + Risk * 0.42f - TireAssist, 0.16f, 0.65f);
        Movement->SetThrottleInput(0.0f);
        Movement->SetBrakeInput(AppliedBrake);

        if (bSevereAttitude || Contacts <= 1)
        {
            Movement->SetSteeringInput(0.0f);
        }
    }

    State.EvidenceSeconds += DeltaTime;
    if (State.EvidenceSeconds < EvidenceIntervalSeconds)
    {
        return;
    }
    State.EvidenceSeconds = 0.0f;

    while (ClearancesCm.Num() < 4)
    {
        ClearancesCm.Add(-1.0f);
    }

    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_STABILITY_EVIDENCE vehicle=RustyFieldmaster60 speed_kmh=%.1f contacts=%d/4 clearances_cm=[%.1f,%.1f,%.1f,%.1f] roll=%.1f pitch=%.1f risk=%.2f low_contact_s=%.2f intervention=%s brake=%.2f tire_level=%d"),
        SpeedKmh,
        Contacts,
        ClearancesCm[0], ClearancesCm[1], ClearancesCm[2], ClearancesCm[3],
        Rotation.Roll,
        Rotation.Pitch,
        Risk,
        State.LowContactSeconds,
        bIntervention ? TEXT("YES") : TEXT("NO"),
        AppliedBrake,
        TireLevel);
}

int32 UGTTNativeStabilitySubsystem::SampleWheelContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const
{
    OutClearancesCm.Reset();
    if (!NativePawn || !GetWorld() || !NativePawn->GetMesh())
    {
        return 0;
    }

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig))
    {
        return 0;
    }

    const TArray<FName> WheelBones = {
        Rig.FrontLeftWheelBone,
        Rig.FrontRightWheelBone,
        Rig.RearLeftWheelBone,
        Rig.RearRightWheelBone
    };

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTNativeStability), false, NativePawn);
    int32 Contacts = 0;
    for (const FName WheelBone : WheelBones)
    {
        if (NativePawn->GetMesh()->GetBoneIndex(WheelBone) == INDEX_NONE)
        {
            OutClearancesCm.Add(-1.0f);
            continue;
        }

        const FVector BoneLocation = NativePawn->GetMesh()->GetBoneLocation(WheelBone, EBoneSpaces::WorldSpace);
        const FVector Start = BoneLocation + FVector::UpVector * ProbeStartLiftCm;
        const FVector End = BoneLocation - FVector::UpVector * ProbeDepthCm;
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            ++Contacts;
            OutClearancesCm.Add(FMath::Max(0.0f, BoneLocation.Z - Hit.ImpactPoint.Z));
        }
        else
        {
            OutClearancesCm.Add(-1.0f);
        }
    }

    return Contacts;
}

float UGTTNativeStabilitySubsystem::ComputeStabilityRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float SpeedKmh) const
{
    if (!NativePawn)
    {
        return 1.0f;
    }

    const FRotator Rotation = NativePawn->GetActorRotation();
    const float RollRisk = FMath::Clamp(FMath::Abs(FMath::UnwindDegrees(Rotation.Roll)) / 38.0f, 0.0f, 1.0f);
    const float PitchRisk = FMath::Clamp(FMath::Abs(FMath::UnwindDegrees(Rotation.Pitch)) / 42.0f, 0.0f, 1.0f);
    const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(Contacts)) / 3.0f, 0.0f, 1.0f);
    const float SpeedRisk = FMath::Clamp((SpeedKmh - 10.0f) / 32.0f, 0.0f, 1.0f);

    return FMath::Clamp(FMath::Max3(RollRisk, PitchRisk, ContactRisk * (0.55f + 0.45f * SpeedRisk)), 0.0f, 1.0f);
}
