#include "Vehicles/GTTNativeStabilitySubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float ProbeStartLiftCm = 30.0f;
    constexpr float ProbeDepthCm = 155.0f;
    constexpr float LowContactGraceSeconds = 0.28f;
    constexpr float TrailerSwayGraceSeconds = 0.35f;
    constexpr float EvidenceIntervalSeconds = 4.0f;
    constexpr float InterventionSpeedKmh = 14.0f;
    constexpr float LoadedInterventionSpeedKmh = 11.0f;
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
    const AGTTFarmTrailer* Trailer = FindAttachedTrailer(NativePawn);
    const float TowLoadFactor = Trailer ? Trailer->GetTowLoadFactor() : 0.0f;
    const float SwayRisk = Trailer ? ComputeTrailerSwayRisk(NativePawn, Trailer, TowLoadFactor) : 0.0f;
    const float Risk = ComputeStabilityRisk(NativePawn, Contacts, SpeedKmh, TowLoadFactor, SwayRisk);
    const FGTTVehicleMigrationSnapshot VehicleState = NativePawn->GetMigrationSnapshot();
    const int32 TireLevel = FMath::Clamp(VehicleState.TireUpgradeLevel, 0, 3);

    State.LastContacts = Contacts;
    State.LastRisk = Risk;
    State.LastTowLoad = TowLoadFactor;
    State.LastSwayRisk = SwayRisk;

    const float ActiveInterventionSpeed = TowLoadFactor >= 0.50f ? LoadedInterventionSpeedKmh : InterventionSpeedKmh;
    if (Contacts <= 2 && SpeedKmh >= ActiveInterventionSpeed)
    {
        State.LowContactSeconds += DeltaTime;
    }
    else
    {
        State.LowContactSeconds = 0.0f;
    }

    if (Trailer && SwayRisk >= 0.50f && SpeedKmh >= LoadedInterventionSpeedKmh)
    {
        State.TrailerSwaySeconds += DeltaTime;
    }
    else
    {
        State.TrailerSwaySeconds = 0.0f;
    }

    const FRotator Rotation = NativePawn->GetActorRotation();
    const float AbsRoll = FMath::Abs(FMath::UnwindDegrees(Rotation.Roll));
    const float AbsPitch = FMath::Abs(FMath::UnwindDegrees(Rotation.Pitch));
    const bool bSevereAttitude = AbsRoll >= SevereRollDegrees || AbsPitch >= SeverePitchDegrees;
    const bool bSustainedLowContact = State.LowContactSeconds >= LowContactGraceSeconds;
    const bool bSustainedTrailerSway = State.TrailerSwaySeconds >= TrailerSwayGraceSeconds;
    const bool bIntervention = SpeedKmh >= ActiveInterventionSpeed && (Risk >= 0.58f || bSustainedLowContact || bSevereAttitude || bSustainedTrailerSway);

    float AppliedBrake = 0.0f;
    if (bIntervention)
    {
        const float TireAssist = TireLevel * 0.03f;
        AppliedBrake = FMath::Clamp(0.18f + Risk * 0.36f + TowLoadFactor * 0.13f + SwayRisk * 0.16f - TireAssist, 0.16f, 0.78f);
        Movement->SetThrottleInput(0.0f);
        Movement->SetBrakeInput(AppliedBrake);

        if (bSevereAttitude || Contacts <= 1 || (bSustainedTrailerSway && SwayRisk >= 0.72f))
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
        TEXT("NATIVE_STABILITY_EVIDENCE vehicle=RustyFieldmaster60 speed_kmh=%.1f contacts=%d/4 clearances_cm=[%.1f,%.1f,%.1f,%.1f] roll=%.1f pitch=%.1f risk=%.2f tow_load=%.2f sway_risk=%.2f sway_s=%.2f low_contact_s=%.2f intervention=%s brake=%.2f tire_level=%d trailer=%s"),
        SpeedKmh,
        Contacts,
        ClearancesCm[0], ClearancesCm[1], ClearancesCm[2], ClearancesCm[3],
        Rotation.Roll,
        Rotation.Pitch,
        Risk,
        TowLoadFactor,
        SwayRisk,
        State.TrailerSwaySeconds,
        State.LowContactSeconds,
        bIntervention ? TEXT("YES") : TEXT("NO"),
        AppliedBrake,
        TireLevel,
        Trailer ? TEXT("ATTACHED") : TEXT("NONE"));
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

AGTTFarmTrailer* UGTTNativeStabilitySubsystem::FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const
{
    UWorld* World = GetWorld();
    if (!World || !NativePawn)
    {
        return nullptr;
    }

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        AGTTFarmTrailer* Trailer = *It;
        if (Trailer && Trailer->IsAttachedToNativeFieldmaster() && Trailer->GetTowActor() == NativePawn)
        {
            return Trailer;
        }
    }
    return nullptr;
}

float UGTTNativeStabilitySubsystem::ComputeTrailerSwayRisk(const AGTTFieldmasterNativePawn* NativePawn, const AGTTFarmTrailer* Trailer, float TowLoadFactor) const
{
    if (!NativePawn || !Trailer || TowLoadFactor <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(NativePawn->GetActorRotation().Yaw, Trailer->GetActorRotation().Yaw));
    const FVector RelativeVelocity = Trailer->GetVelocity() - NativePawn->GetVelocity();
    const float LateralRelativeKmh = FMath::Abs(FVector::DotProduct(RelativeVelocity, NativePawn->GetActorRightVector())) * 0.036f;
    const float YawRisk = FMath::Clamp((YawDelta - 7.0f) / 34.0f, 0.0f, 1.0f);
    const float LateralRisk = FMath::Clamp((LateralRelativeKmh - 2.0f) / 18.0f, 0.0f, 1.0f);
    const float LoadAmplifier = FMath::Lerp(0.55f, 1.25f, FMath::Clamp(TowLoadFactor, 0.0f, 1.0f));
    return FMath::Clamp(FMath::Max(YawRisk, LateralRisk) * LoadAmplifier, 0.0f, 1.0f);
}

float UGTTNativeStabilitySubsystem::ComputeStabilityRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float SpeedKmh, float TowLoadFactor, float SwayRisk) const
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
    const float GroundRisk = FMath::Max3(RollRisk, PitchRisk, ContactRisk * (0.55f + 0.45f * SpeedRisk));
    const float HeavyHaulRisk = TowLoadFactor * (0.10f + 0.10f * SpeedRisk) + SwayRisk * (0.30f + 0.18f * SpeedRisk);

    return FMath::Clamp(FMath::Max(GroundRisk, HeavyHaulRisk), 0.0f, 1.0f);
}