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
    constexpr float LoadTransferGraceSeconds = 0.45f;
    constexpr float TractionLossGraceSeconds = 0.25f;
    constexpr float EvidenceIntervalSeconds = 4.0f;
    constexpr float InterventionSpeedKmh = 14.0f;
    constexpr float LoadedInterventionSpeedKmh = 11.0f;
    constexpr float TractionControlSpeedKmh = 9.0f;
    constexpr float SevereRollDegrees = 30.0f;
    constexpr float SeverePitchDegrees = 32.0f;

    struct FGTTRuntimeWheelEvidence
    {
        bool bComplete = false;
        int32 ValidWheels = 0;
        int32 Contacts = 0;
        int32 SlippingWheels = 0;
        int32 SkiddingWheels = 0;
        float MaxSlipMagnitude = 0.0f;
        float MaxSlipAngle = 0.0f;
        TArray<uint8> InContact;
        TArray<uint8> IsSlipping;
        TArray<uint8> IsSkidding;
        TArray<float> SuspensionLength;
        TArray<float> SpringForce;
        TArray<float> DriveTorque;
        TArray<float> BrakeTorque;
    };

    bool SampleChaosWheelRuntime(const UChaosWheeledVehicleMovementComponent* Movement, FGTTRuntimeWheelEvidence& OutEvidence)
    {
        OutEvidence = FGTTRuntimeWheelEvidence();
        OutEvidence.InContact.Init(0, 4);
        OutEvidence.IsSlipping.Init(0, 4);
        OutEvidence.IsSkidding.Init(0, 4);
        OutEvidence.SuspensionLength.Init(-1.0f, 4);
        OutEvidence.SpringForce.Init(0.0f, 4);
        OutEvidence.DriveTorque.Init(0.0f, 4);
        OutEvidence.BrakeTorque.Init(0.0f, 4);

        if (!Movement || Movement->GetNumWheels() != 4)
        {
            return false;
        }

        for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
        {
            const FWheelStatus& WheelState = Movement->GetWheelState(WheelIndex);
            if (!WheelState.bIsValid)
            {
                continue;
            }

            ++OutEvidence.ValidWheels;
            OutEvidence.InContact[WheelIndex] = WheelState.bInContact ? 1 : 0;
            OutEvidence.IsSlipping[WheelIndex] = WheelState.bIsSlipping ? 1 : 0;
            OutEvidence.IsSkidding[WheelIndex] = WheelState.bIsSkidding ? 1 : 0;
            OutEvidence.Contacts += WheelState.bInContact ? 1 : 0;
            OutEvidence.SlippingWheels += WheelState.bIsSlipping ? 1 : 0;
            OutEvidence.SkiddingWheels += WheelState.bIsSkidding ? 1 : 0;
            OutEvidence.MaxSlipMagnitude = FMath::Max(OutEvidence.MaxSlipMagnitude, FMath::Abs(WheelState.SlipMagnitude));
            OutEvidence.MaxSlipAngle = FMath::Max(OutEvidence.MaxSlipAngle, FMath::Abs(WheelState.SlipAngle));
            OutEvidence.SuspensionLength[WheelIndex] = WheelState.NormalizedSuspensionLength;
            OutEvidence.SpringForce[WheelIndex] = WheelState.SpringForce;
            OutEvidence.DriveTorque[WheelIndex] = WheelState.DriveTorque;
            OutEvidence.BrakeTorque[WheelIndex] = WheelState.BrakeTorque;
        }

        OutEvidence.bComplete = OutEvidence.ValidWheels == 4;
        return OutEvidence.bComplete;
    }

    float ComputeChaosSlipRisk(const FGTTRuntimeWheelEvidence& Evidence)
    {
        if (!Evidence.bComplete)
        {
            return 0.0f;
        }

        const float SlipFlagRisk = FMath::Clamp(static_cast<float>(Evidence.SlippingWheels) / 3.0f, 0.0f, 1.0f);
        const float SkidFlagRisk = FMath::Clamp(static_cast<float>(Evidence.SkiddingWheels) / 2.0f, 0.0f, 1.0f);
        const float MagnitudeRisk = FMath::Clamp(Evidence.MaxSlipMagnitude / 650.0f, 0.0f, 1.0f);
        return FMath::Clamp(FMath::Max3(SlipFlagRisk, SkidFlagRisk, MagnitudeRisk), 0.0f, 1.0f);
    }

    float ComputeAxleRuntimeGrip(const FGTTRuntimeWheelEvidence& Evidence, int32 FirstWheel)
    {
        if (!Evidence.bComplete || FirstWheel < 0 || FirstWheel + 1 >= 4)
        {
            return 1.0f;
        }

        float Grip = 1.0f;
        for (int32 WheelIndex = FirstWheel; WheelIndex <= FirstWheel + 1; ++WheelIndex)
        {
            if (Evidence.InContact[WheelIndex] == 0) Grip -= 0.45f;
            if (Evidence.IsSlipping[WheelIndex] != 0) Grip -= 0.22f;
            if (Evidence.IsSkidding[WheelIndex] != 0) Grip -= 0.28f;
        }
        return FMath::Clamp(Grip, 0.10f, 1.0f);
    }
}

void UGTTNativeStabilitySubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;
    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It) EvaluateFieldmaster(*It, DeltaTime);
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
    if (!NativePawn) return;
    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive() || !NativePawn->IsNativeFieldmasterReady())
    {
        StabilityStates.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive()) return;

    FStabilityState& State = StabilityStates.FindOrAdd(Key);
    TArray<float> ClearancesCm;
    const int32 TraceContacts = SampleWheelContacts(NativePawn, ClearancesCm);
    FGTTRuntimeWheelEvidence ChaosWheelEvidence;
    const bool bChaosWheelEvidenceValid = SampleChaosWheelRuntime(Movement, ChaosWheelEvidence);
    const int32 Contacts = bChaosWheelEvidenceValid ? ChaosWheelEvidence.Contacts : TraceContacts;
    const float SpeedKmh = NativePawn->GetVelocity().Size() * 0.036f;
    const AGTTFarmTrailer* Trailer = FindAttachedTrailer(NativePawn);
    const float TowLoadFactor = Trailer ? Trailer->GetTowLoadFactor() : 0.0f;
    const float SwayRisk = Trailer ? ComputeTrailerSwayRisk(NativePawn, Trailer, TowLoadFactor) : 0.0f;
    float FrontRearBias = 0.0f;
    float SideBias = 0.0f;
    const float LoadTransferRisk = ComputeLoadTransferRisk(NativePawn, ClearancesCm, TowLoadFactor, FrontRearBias, SideBias);
    float SlipAngleDegrees = 0.0f;
    float FrontTraction = 1.0f;
    float RearTraction = 1.0f;
    float TractionRisk = ComputeTractionRisk(NativePawn, Contacts, TowLoadFactor, FrontRearBias, SideBias, SlipAngleDegrees, FrontTraction, RearTraction);
    const float ChaosSlipRisk = ComputeChaosSlipRisk(ChaosWheelEvidence);
    if (bChaosWheelEvidenceValid)
    {
        TractionRisk = FMath::Max(TractionRisk, ChaosSlipRisk);
        FrontTraction = FMath::Min(FrontTraction, ComputeAxleRuntimeGrip(ChaosWheelEvidence, 0));
        RearTraction = FMath::Min(RearTraction, ComputeAxleRuntimeGrip(ChaosWheelEvidence, 2));
        TractionRisk = FMath::Max(TractionRisk, 1.0f - FMath::Min(FrontTraction, RearTraction));
    }
    const float Risk = ComputeStabilityRisk(NativePawn, Contacts, SpeedKmh, TowLoadFactor, SwayRisk, LoadTransferRisk, TractionRisk);
    const FGTTVehicleMigrationSnapshot VehicleState = NativePawn->GetMigrationSnapshot();
    const int32 TireLevel = FMath::Clamp(VehicleState.TireUpgradeLevel, 0, 3);

    State.LastContacts = Contacts;
    State.LastRisk = Risk;
    State.LastTowLoad = TowLoadFactor;
    State.LastSwayRisk = SwayRisk;
    State.LastLoadTransferRisk = LoadTransferRisk;
    State.LastTractionRisk = TractionRisk;
    State.LastSlipAngleDegrees = SlipAngleDegrees;
    State.LastFrontTraction = FrontTraction;
    State.LastRearTraction = RearTraction;
    State.LastFrontRearBias = FrontRearBias;
    State.LastSideBias = SideBias;

    const float ActiveInterventionSpeed = TowLoadFactor >= 0.50f ? LoadedInterventionSpeedKmh : InterventionSpeedKmh;
    State.LowContactSeconds = (Contacts <= 2 && SpeedKmh >= ActiveInterventionSpeed) ? State.LowContactSeconds + DeltaTime : 0.0f;
    State.TrailerSwaySeconds = (Trailer && SwayRisk >= 0.50f && SpeedKmh >= LoadedInterventionSpeedKmh) ? State.TrailerSwaySeconds + DeltaTime : 0.0f;
    State.LoadTransferSeconds = (LoadTransferRisk >= 0.52f && SpeedKmh >= LoadedInterventionSpeedKmh) ? State.LoadTransferSeconds + DeltaTime : 0.0f;
    State.TractionLossSeconds = (TractionRisk >= 0.42f && SpeedKmh >= TractionControlSpeedKmh) ? State.TractionLossSeconds + DeltaTime : 0.0f;

    const FRotator Rotation = NativePawn->GetActorRotation();
    const float AbsRoll = FMath::Abs(FMath::UnwindDegrees(Rotation.Roll));
    const float AbsPitch = FMath::Abs(FMath::UnwindDegrees(Rotation.Pitch));
    const bool bSevereAttitude = AbsRoll >= SevereRollDegrees || AbsPitch >= SeverePitchDegrees;
    const bool bSustainedLowContact = State.LowContactSeconds >= LowContactGraceSeconds;
    const bool bSustainedTrailerSway = State.TrailerSwaySeconds >= TrailerSwayGraceSeconds;
    const bool bSustainedLoadTransfer = State.LoadTransferSeconds >= LoadTransferGraceSeconds;
    const bool bSustainedTractionLoss = State.TractionLossSeconds >= TractionLossGraceSeconds;
    const bool bTractionControl = bSustainedTractionLoss && SpeedKmh >= TractionControlSpeedKmh;
    const bool bIntervention = SpeedKmh >= ActiveInterventionSpeed && (Risk >= 0.58f || bSustainedLowContact || bSevereAttitude || bSustainedTrailerSway || bSustainedLoadTransfer || (bSustainedTractionLoss && TractionRisk >= 0.70f));

    float AppliedBrake = 0.0f;
    float ThrottleLimit = 1.0f;
    if (bIntervention)
    {
        const float TireAssist = TireLevel * 0.03f;
        const float LoadTransferBrake = bSustainedLoadTransfer ? LoadTransferRisk * 0.18f : 0.0f;
        const float TractionBrake = bSustainedTractionLoss ? TractionRisk * 0.16f : 0.0f;
        AppliedBrake = FMath::Clamp(0.18f + Risk * 0.32f + TowLoadFactor * 0.12f + SwayRisk * 0.14f + LoadTransferBrake + TractionBrake - TireAssist, 0.16f, 0.84f);
        ThrottleLimit = 0.0f;
        Movement->SetThrottleInput(0.0f);
        Movement->SetBrakeInput(AppliedBrake);

        if (bSevereAttitude || Contacts <= 1 || (bSustainedTrailerSway && SwayRisk >= 0.72f) || (bSustainedLoadTransfer && FMath::Abs(SideBias) >= 0.62f) || (bSustainedTractionLoss && (SlipAngleDegrees >= 32.0f || ChaosSlipRisk >= 0.82f)))
        {
            Movement->SetSteeringInput(0.0f);
        }
    }
    else if (bTractionControl && NativePawn->IsOccupied())
    {
        const float TireAssist = TireLevel * 0.04f;
        ThrottleLimit = FMath::Clamp(0.92f - TractionRisk * 0.62f + TireAssist, 0.28f, 0.82f);
        const float RequestedThrottle = FMath::Abs(NativePawn->GetRequestedThrottleInput());
        Movement->SetThrottleInput(RequestedThrottle * ThrottleLimit);
        AppliedBrake = TractionRisk >= 0.62f ? FMath::Clamp(0.04f + TractionRisk * 0.18f, 0.0f, 0.22f) : 0.0f;
        if (AppliedBrake > 0.0f) Movement->SetBrakeInput(AppliedBrake);
    }

    State.EvidenceSeconds += DeltaTime;
    if (State.EvidenceSeconds < EvidenceIntervalSeconds) return;
    State.EvidenceSeconds = 0.0f;
    while (ClearancesCm.Num() < 4) ClearancesCm.Add(-1.0f);

    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_STABILITY_EVIDENCE vehicle=RustyFieldmaster60 speed_kmh=%.1f contacts=%d/4 contact_source=%s trace_contacts=%d/4 clearances_cm=[%.1f,%.1f,%.1f,%.1f] roll=%.1f pitch=%.1f risk=%.2f tow_load=%.2f sway_risk=%.2f load_transfer=%.2f traction_risk=%.2f chaos_slip_risk=%.2f slip_deg=%.1f front_traction=%.2f rear_traction=%.2f front_rear_bias=%.2f side_bias=%.2f sway_s=%.2f load_transfer_s=%.2f traction_loss_s=%.2f low_contact_s=%.2f traction_control=%s throttle_limit=%.2f intervention=%s brake=%.2f tire_level=%d tire_integrity=%.2f trailer=%s"),
        SpeedKmh, Contacts, bChaosWheelEvidenceValid ? TEXT("CHAOS") : TEXT("TRACE_FALLBACK"), TraceContacts,
        ClearancesCm[0], ClearancesCm[1], ClearancesCm[2], ClearancesCm[3], Rotation.Roll, Rotation.Pitch,
        Risk, TowLoadFactor, SwayRisk, LoadTransferRisk, TractionRisk, ChaosSlipRisk, SlipAngleDegrees, FrontTraction, RearTraction, FrontRearBias, SideBias,
        State.TrailerSwaySeconds, State.LoadTransferSeconds, State.TractionLossSeconds, State.LowContactSeconds,
        bTractionControl ? TEXT("YES") : TEXT("NO"), ThrottleLimit, bIntervention ? TEXT("YES") : TEXT("NO"), AppliedBrake,
        TireLevel, VehicleState.TireIntegrity, Trailer ? TEXT("ATTACHED") : TEXT("NONE"));

    if (bChaosWheelEvidenceValid)
    {
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_CHAOS_WHEEL_STATE_EVIDENCE vehicle=RustyFieldmaster60 valid=%d/4 contacts=%d/4 suspension=[%.3f,%.3f,%.3f,%.3f] spring_force=[%.1f,%.1f,%.1f,%.1f] drive_torque=[%.1f,%.1f,%.1f,%.1f] brake_torque=[%.1f,%.1f,%.1f,%.1f] slipping=%d skidding=%d max_slip_magnitude=%.1f max_slip_angle=%.3f chaos_slip_risk=%.2f"),
            ChaosWheelEvidence.ValidWheels, ChaosWheelEvidence.Contacts,
            ChaosWheelEvidence.SuspensionLength[0], ChaosWheelEvidence.SuspensionLength[1], ChaosWheelEvidence.SuspensionLength[2], ChaosWheelEvidence.SuspensionLength[3],
            ChaosWheelEvidence.SpringForce[0], ChaosWheelEvidence.SpringForce[1], ChaosWheelEvidence.SpringForce[2], ChaosWheelEvidence.SpringForce[3],
            ChaosWheelEvidence.DriveTorque[0], ChaosWheelEvidence.DriveTorque[1], ChaosWheelEvidence.DriveTorque[2], ChaosWheelEvidence.DriveTorque[3],
            ChaosWheelEvidence.BrakeTorque[0], ChaosWheelEvidence.BrakeTorque[1], ChaosWheelEvidence.BrakeTorque[2], ChaosWheelEvidence.BrakeTorque[3],
            ChaosWheelEvidence.SlippingWheels, ChaosWheelEvidence.SkiddingWheels, ChaosWheelEvidence.MaxSlipMagnitude, ChaosWheelEvidence.MaxSlipAngle, ChaosSlipRisk);
    }
}

int32 UGTTNativeStabilitySubsystem::SampleWheelContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const
{
    OutClearancesCm.Reset();
    if (!NativePawn || !GetWorld() || !NativePawn->GetMesh()) return 0;
    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig)) return 0;
    const TArray<FName> WheelBones = { Rig.FrontLeftWheelBone, Rig.FrontRightWheelBone, Rig.RearLeftWheelBone, Rig.RearRightWheelBone };
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTNativeStability), false, NativePawn);
    int32 Contacts = 0;
    for (const FName WheelBone : WheelBones)
    {
        if (NativePawn->GetMesh()->GetBoneIndex(WheelBone) == INDEX_NONE) { OutClearancesCm.Add(-1.0f); continue; }
        const FVector BoneLocation = NativePawn->GetMesh()->GetBoneLocation(WheelBone, EBoneSpaces::WorldSpace);
        const FVector Start = BoneLocation + FVector::UpVector * ProbeStartLiftCm;
        const FVector End = BoneLocation - FVector::UpVector * ProbeDepthCm;
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
        {
            ++Contacts;
            OutClearancesCm.Add(FMath::Max(0.0f, BoneLocation.Z - Hit.ImpactPoint.Z));
        }
        else OutClearancesCm.Add(-1.0f);
    }
    return Contacts;
}

AGTTFarmTrailer* UGTTNativeStabilitySubsystem::FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const
{
    UWorld* World = GetWorld();
    if (!World || !NativePawn) return nullptr;
    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        AGTTFarmTrailer* Trailer = *It;
        if (Trailer && Trailer->IsAttachedToNativeFieldmaster() && Trailer->GetTowActor() == NativePawn) return Trailer;
    }
    return nullptr;
}

float UGTTNativeStabilitySubsystem::ComputeTrailerSwayRisk(const AGTTFieldmasterNativePawn* NativePawn, const AGTTFarmTrailer* Trailer, float TowLoadFactor) const
{
    if (!NativePawn || !Trailer || TowLoadFactor <= KINDA_SMALL_NUMBER) return 0.0f;
    const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(NativePawn->GetActorRotation().Yaw, Trailer->GetActorRotation().Yaw));
    const FVector RelativeVelocity = Trailer->GetVelocity() - NativePawn->GetVelocity();
    const float LateralRelativeKmh = FMath::Abs(FVector::DotProduct(RelativeVelocity, NativePawn->GetActorRightVector())) * 0.036f;
    const float YawRisk = FMath::Clamp((YawDelta - 7.0f) / 34.0f, 0.0f, 1.0f);
    const float LateralRisk = FMath::Clamp((LateralRelativeKmh - 2.0f) / 18.0f, 0.0f, 1.0f);
    const float LoadAmplifier = FMath::Lerp(0.55f, 1.25f, FMath::Clamp(TowLoadFactor, 0.0f, 1.0f));
    return FMath::Clamp(FMath::Max(YawRisk, LateralRisk) * LoadAmplifier, 0.0f, 1.0f);
}

float UGTTNativeStabilitySubsystem::ComputeLoadTransferRisk(const AGTTFieldmasterNativePawn* NativePawn, const TArray<float>& ClearancesCm, float TowLoadFactor, float& OutFrontRearBias, float& OutSideBias) const
{
    OutFrontRearBias = 0.0f;
    OutSideBias = 0.0f;
    if (!NativePawn || ClearancesCm.Num() < 4) return 0.0f;

    auto Valid = [](float Value) { return Value >= 0.0f; };
    if (!Valid(ClearancesCm[0]) || !Valid(ClearancesCm[1]) || !Valid(ClearancesCm[2]) || !Valid(ClearancesCm[3])) return 0.0f;

    const float FrontAvg = 0.5f * (ClearancesCm[0] + ClearancesCm[1]);
    const float RearAvg = 0.5f * (ClearancesCm[2] + ClearancesCm[3]);
    const float LeftAvg = 0.5f * (ClearancesCm[0] + ClearancesCm[2]);
    const float RightAvg = 0.5f * (ClearancesCm[1] + ClearancesCm[3]);

    OutFrontRearBias = FMath::Clamp((FrontAvg - RearAvg) / 42.0f, -1.0f, 1.0f);
    OutSideBias = FMath::Clamp((LeftAvg - RightAvg) / 34.0f, -1.0f, 1.0f);

    const float GradeRisk = FMath::Clamp(FMath::Abs(FMath::UnwindDegrees(NativePawn->GetActorRotation().Pitch)) / 30.0f, 0.0f, 1.0f);
    const float LongitudinalRisk = FMath::Clamp(FMath::Abs(OutFrontRearBias) * (0.65f + TowLoadFactor * 0.45f), 0.0f, 1.0f);
    const float LateralRisk = FMath::Clamp(FMath::Abs(OutSideBias) * 1.15f, 0.0f, 1.0f);
    const float TrailerRearBias = TowLoadFactor * FMath::Clamp(FMath::Max(0.0f, OutFrontRearBias), 0.0f, 1.0f) * 0.35f;
    return FMath::Clamp(FMath::Max3(LongitudinalRisk + TrailerRearBias, LateralRisk, GradeRisk * 0.72f), 0.0f, 1.0f);
}

float UGTTNativeStabilitySubsystem::ComputeTractionRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float TowLoadFactor, float FrontRearBias, float SideBias, float& OutSlipAngleDegrees, float& OutFrontTraction, float& OutRearTraction) const
{
    OutSlipAngleDegrees = 0.0f;
    OutFrontTraction = 1.0f;
    OutRearTraction = 1.0f;
    if (!NativePawn) return 1.0f;

    const FGTTVehicleMigrationSnapshot VehicleState = NativePawn->GetMigrationSnapshot();
    const float TireIntegrity = FMath::Clamp(VehicleState.TireIntegrity, 0.0f, 1.0f);
    const int32 TireLevel = FMath::Clamp(VehicleState.TireUpgradeLevel, 0, 3);
    const FVector Velocity = NativePawn->GetVelocity();
    const float ForwardCms = FMath::Abs(FVector::DotProduct(Velocity, NativePawn->GetActorForwardVector()));
    const float LateralCms = FMath::Abs(FVector::DotProduct(Velocity, NativePawn->GetActorRightVector()));
    const float SpeedKmh = Velocity.Size() * 0.036f;
    OutSlipAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(LateralCms, FMath::Max(ForwardCms, 150.0f)));

    const float TireUpgradeAssist = static_cast<float>(TireLevel) * 0.05f;
    const float SideUnload = FMath::Abs(SideBias);
    const float FrontUnload = FMath::Max(0.0f, FrontRearBias);
    const float RearUnload = FMath::Max(0.0f, -FrontRearBias);
    OutFrontTraction = FMath::Clamp(TireIntegrity * (1.0f - FrontUnload * 0.46f - SideUnload * 0.16f - TowLoadFactor * FrontUnload * 0.12f) + TireUpgradeAssist, 0.05f, 1.0f);
    OutRearTraction = FMath::Clamp(TireIntegrity * (1.0f - RearUnload * 0.40f - SideUnload * 0.13f) + TireUpgradeAssist + TowLoadFactor * 0.08f, 0.05f, 1.0f);

    const float SpeedFactor = FMath::Clamp((SpeedKmh - 6.0f) / 24.0f, 0.0f, 1.0f);
    const float SlipRisk = FMath::Clamp((OutSlipAngleDegrees - 6.0f) / 24.0f, 0.0f, 1.0f) * (0.55f + SpeedFactor * 0.45f);
    const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(Contacts)) / 3.0f, 0.0f, 1.0f) * (0.45f + SpeedFactor * 0.35f);
    const float AxleGripRisk = 1.0f - FMath::Min(OutFrontTraction, OutRearTraction);
    return FMath::Clamp(FMath::Max3(SlipRisk, ContactRisk, AxleGripRisk), 0.0f, 1.0f);
}

float UGTTNativeStabilitySubsystem::ComputeStabilityRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float SpeedKmh, float TowLoadFactor, float SwayRisk, float LoadTransferRisk, float TractionRisk) const
{
    if (!NativePawn) return 1.0f;
    const FRotator Rotation = NativePawn->GetActorRotation();
    const float RollRisk = FMath::Clamp(FMath::Abs(FMath::UnwindDegrees(Rotation.Roll)) / 38.0f, 0.0f, 1.0f);
    const float PitchRisk = FMath::Clamp(FMath::Abs(FMath::UnwindDegrees(Rotation.Pitch)) / 42.0f, 0.0f, 1.0f);
    const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(Contacts)) / 3.0f, 0.0f, 1.0f);
    const float SpeedRisk = FMath::Clamp((SpeedKmh - 10.0f) / 32.0f, 0.0f, 1.0f);
    const float GroundRisk = FMath::Max3(RollRisk, PitchRisk, ContactRisk * (0.55f + 0.45f * SpeedRisk));
    const float HeavyHaulRisk = TowLoadFactor * (0.10f + 0.10f * SpeedRisk) + SwayRisk * (0.30f + 0.18f * SpeedRisk);
    const float ChassisRisk = LoadTransferRisk * (0.34f + 0.24f * SpeedRisk + 0.12f * TowLoadFactor);
    const float GripRisk = TractionRisk * (0.36f + 0.28f * SpeedRisk + 0.10f * TowLoadFactor);
    return FMath::Clamp(FMath::Max(FMath::Max3(GroundRisk, HeavyHaulRisk, ChassisRisk), GripRisk), 0.0f, 1.0f);
}
