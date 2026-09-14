#include "Vehicles/GTTNativeTerrainLoadSubsystem.h"

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
    constexpr float ProbeStartLiftCm = 28.0f;
    constexpr float ProbeDepthCm = 150.0f;
    constexpr float MinimumGradeDegrees = 5.0f;
    constexpr float LowSpeedControlKmh = 12.0f;
    constexpr float WheelLoadControlMaxKmh = 26.0f;
    constexpr float RollbackTriggerKmh = -1.2f;
    constexpr float RollbackGraceSeconds = 0.12f;
    constexpr float CrossAxleGraceSeconds = 0.22f;
    constexpr float EvidenceIntervalSeconds = 4.0f;
}

void UGTTNativeTerrainLoadSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        EvaluateFieldmaster(*It, DeltaTime);
    }
}

TStatId UGTTNativeTerrainLoadSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativeTerrainLoadSubsystem, STATGROUP_Tickables);
}

bool UGTTNativeTerrainLoadSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativeTerrainLoadSubsystem::EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn) return;

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive() || !NativePawn->IsNativeFieldmasterReady() || !NativePawn->IsOccupied())
    {
        TerrainLoadStates.Remove(Key);
        return;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive()) return;

    FTerrainLoadState& State = TerrainLoadStates.FindOrAdd(Key);
    const FVector Velocity = NativePawn->GetVelocity();
    const float SignedForwardKmh = FVector::DotProduct(Velocity, NativePawn->GetActorForwardVector()) * 0.036f;
    const float SpeedKmh = Velocity.Size() * 0.036f;
    const float GradeDegrees = FMath::Abs(FMath::UnwindDegrees(NativePawn->GetActorRotation().Pitch));
    const float RollDegrees = FMath::Abs(FMath::UnwindDegrees(NativePawn->GetActorRotation().Roll));
    const float MudSeverity = NativePawn->GetNativeMudSeverity();
    const float TerrainGrip = NativePawn->GetNativeTerrainGripFactor();
    const float RequestedThrottle = FMath::Abs(NativePawn->GetRequestedThrottleInput());
    const FGTTVehicleMigrationSnapshot Snapshot = NativePawn->GetMigrationSnapshot();

    AGTTFarmTrailer* Trailer = FindAttachedTrailer(NativePawn);
    const float TowLoad = Trailer ? Trailer->GetTowLoadFactor() : 0.0f;

    FWheelLoadEvidence WheelEvidence;
    const bool bWheelEvidence = SampleWheelLoadEvidence(NativePawn, WheelEvidence);
    const float FrontClearanceCm = bWheelEvidence ? 0.5f * (WheelEvidence.FrontLeftClearanceCm + WheelEvidence.FrontRightClearanceCm) : 0.0f;
    const float RearClearanceCm = bWheelEvidence ? 0.5f * (WheelEvidence.RearLeftClearanceCm + WheelEvidence.RearRightClearanceCm) : 0.0f;
    const float ClearanceBias = bWheelEvidence
        ? FMath::Clamp((FrontClearanceCm - RearClearanceCm) / 42.0f, -1.0f, 1.0f)
        : 0.0f;

    const float RearLoadBias = FMath::Clamp(FMath::Max(0.0f, ClearanceBias) + TowLoad * 0.42f, 0.0f, 1.0f);
    const float FrontUnloadPenalty = RearLoadBias * (0.28f + TowLoad * 0.12f);
    const float MudPenalty = MudSeverity * 0.44f;
    const float LaunchGrip = FMath::Clamp(TerrainGrip - FrontUnloadPenalty - MudPenalty * 0.25f, 0.22f, 1.0f);

    const bool bTerrainLoaded = MudSeverity >= 0.08f || TowLoad >= 0.35f;
    const bool bOnMeaningfulGrade = GradeDegrees >= MinimumGradeDegrees;
    const bool bLowSpeed = SpeedKmh <= LowSpeedControlKmh;
    const bool bLaunchControl = bTerrainLoaded && bOnMeaningfulGrade && bLowSpeed && RequestedThrottle > 0.05f;

    float ThrottleLimit = 1.0f;
    float InterventionBrake = 0.0f;
    if (bLaunchControl)
    {
        const float GradePenalty = FMath::Clamp((GradeDegrees - MinimumGradeDegrees) / 20.0f, 0.0f, 1.0f) * 0.22f;
        const float TowPenalty = TowLoad * 0.16f;
        ThrottleLimit = FMath::Clamp(LaunchGrip - GradePenalty - TowPenalty + 0.18f, 0.24f, 0.88f);
        Movement->SetThrottleInput(RequestedThrottle * ThrottleLimit);
    }

    const bool bRollingBackward = SignedForwardKmh <= RollbackTriggerKmh;
    State.RollbackSeconds = (bLaunchControl && bRollingBackward) ? State.RollbackSeconds + DeltaTime : 0.0f;
    const bool bRollbackControl = State.RollbackSeconds >= RollbackGraceSeconds;

    if (bRollbackControl)
    {
        const float RollbackSeverity = FMath::Clamp(FMath::Abs(SignedForwardKmh) / 8.0f, 0.0f, 1.0f);
        const float HoldBrake = FMath::Clamp(0.34f + RollbackSeverity * 0.30f + TowLoad * 0.18f + MudSeverity * 0.10f, 0.34f, 0.82f);
        Movement->SetThrottleInput(0.0f);
        Movement->SetBrakeInput(HoldBrake);
        ThrottleLimit = 0.0f;
        InterventionBrake = HoldBrake;
    }

    float CrossAxleRisk = 0.0f;
    float FrontAxleGrip = TerrainGrip;
    float RearAxleGrip = TerrainGrip;
    bool bCrossAxleControl = false;

    if (bWheelEvidence)
    {
        const float TireHealth = FMath::Clamp(Snapshot.TireIntegrity, 0.0f, 1.0f);
        const float TireUpgradeBonus = FMath::Clamp(static_cast<float>(Snapshot.TireUpgradeLevel), 0.0f, 3.0f) * 0.04f;
        const float TireFactor = FMath::Clamp(0.45f + TireHealth * 0.55f + TireUpgradeBonus, 0.35f, 1.08f);
        FrontAxleGrip = FMath::Clamp(TerrainGrip * WheelEvidence.FrontLoadFactor * TireFactor * (1.0f - RearLoadBias * 0.18f), 0.18f, 1.10f);
        RearAxleGrip = FMath::Clamp(TerrainGrip * WheelEvidence.RearLoadFactor * TireFactor * (1.0f + TowLoad * 0.05f), 0.18f, 1.10f);

        const float AxleSplitRisk = FMath::Max(WheelEvidence.FrontCrossAxleImbalance, WheelEvidence.RearCrossAxleImbalance);
        const float SideUnloadRisk = WheelEvidence.SideLoadImbalance;
        const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(WheelEvidence.GroundedWheelCount)) / 3.0f, 0.0f, 1.0f);
        const float RollRisk = FMath::Clamp((RollDegrees - 7.0f) / 18.0f, 0.0f, 1.0f);
        CrossAxleRisk = FMath::Clamp(
            AxleSplitRisk * 0.48f + SideUnloadRisk * 0.24f + ContactRisk * 0.20f + RollRisk * 0.18f + MudSeverity * 0.10f,
            0.0f, 1.0f);

        const bool bEligibleSpeed = SpeedKmh >= 2.0f && SpeedKmh <= WheelLoadControlMaxKmh;
        const bool bRiskPresent = CrossAxleRisk >= 0.24f && RequestedThrottle > 0.05f;
        State.CrossAxleRiskSeconds = (bEligibleSpeed && bRiskPresent) ? State.CrossAxleRiskSeconds + DeltaTime : FMath::Max(0.0f, State.CrossAxleRiskSeconds - DeltaTime * 2.0f);
        bCrossAxleControl = !bRollbackControl && State.CrossAxleRiskSeconds >= CrossAxleGraceSeconds;

        if (bCrossAxleControl)
        {
            const float GripFloor = FMath::Min(FrontAxleGrip, RearAxleGrip);
            const float RiskThrottleCap = FMath::Clamp(0.92f - CrossAxleRisk * 0.58f, 0.28f, 0.82f);
            const float GripThrottleCap = FMath::Clamp(GripFloor + 0.18f, 0.28f, 0.92f);
            const float CrossAxleThrottleLimit = FMath::Min(RiskThrottleCap, GripThrottleCap);
            ThrottleLimit = FMath::Min(ThrottleLimit, CrossAxleThrottleLimit);
            Movement->SetThrottleInput(RequestedThrottle * ThrottleLimit);

            InterventionBrake = FMath::Max(InterventionBrake, FMath::Clamp((CrossAxleRisk - 0.24f) * 0.34f, 0.0f, 0.24f));
            if (InterventionBrake > KINDA_SMALL_NUMBER)
            {
                Movement->SetBrakeInput(InterventionBrake);
            }

            if (CrossAxleRisk >= 0.72f || WheelEvidence.GroundedWheelCount <= 2)
            {
                Movement->SetSteeringInput(0.0f);
            }
        }
    }
    else
    {
        State.CrossAxleRiskSeconds = 0.0f;
    }

    State.LastThrottleLimit = ThrottleLimit;
    State.LastRearLoadBias = RearLoadBias;
    State.LastLaunchGrip = LaunchGrip;
    State.LastCrossAxleRisk = CrossAxleRisk;
    State.LastFrontAxleGrip = FrontAxleGrip;
    State.LastRearAxleGrip = RearAxleGrip;
    State.bLastRollbackControl = bRollbackControl;
    State.bLastCrossAxleControl = bCrossAxleControl;

    State.EvidenceSeconds += DeltaTime;
    if (State.EvidenceSeconds < EvidenceIntervalSeconds) return;
    State.EvidenceSeconds = 0.0f;

    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_TERRAIN_LOAD_EVIDENCE vehicle=RustyFieldmaster60 speed_kmh=%.1f signed_forward_kmh=%.1f grade_deg=%.1f mud=%.2f terrain_grip=%.2f tow_load=%.2f front_clearance_cm=%.1f rear_clearance_cm=%.1f rear_load_bias=%.2f launch_grip=%.2f throttle_limit=%.2f rollback_s=%.2f rollback_control=%s trailer=%s"),
        SpeedKmh,
        SignedForwardKmh,
        GradeDegrees,
        MudSeverity,
        TerrainGrip,
        TowLoad,
        FrontClearanceCm,
        RearClearanceCm,
        RearLoadBias,
        LaunchGrip,
        ThrottleLimit,
        State.RollbackSeconds,
        bRollbackControl ? TEXT("YES") : TEXT("NO"),
        Trailer ? TEXT("ATTACHED") : TEXT("NONE"));

    if (bWheelEvidence)
    {
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_WHEEL_LOAD_EVIDENCE vehicle=RustyFieldmaster60 contacts=%d/4 fl_cm=%.1f fr_cm=%.1f rl_cm=%.1f rr_cm=%.1f fl_load=%.2f fr_load=%.2f rl_load=%.2f rr_load=%.2f front_grip=%.2f rear_grip=%.2f front_split=%.2f rear_split=%.2f side_imbalance=%.2f cross_axle_risk=%.2f risk_s=%.2f control=%s brake=%.2f"),
            WheelEvidence.GroundedWheelCount,
            WheelEvidence.FrontLeftClearanceCm,
            WheelEvidence.FrontRightClearanceCm,
            WheelEvidence.RearLeftClearanceCm,
            WheelEvidence.RearRightClearanceCm,
            WheelEvidence.FrontLeftLoad,
            WheelEvidence.FrontRightLoad,
            WheelEvidence.RearLeftLoad,
            WheelEvidence.RearRightLoad,
            FrontAxleGrip,
            RearAxleGrip,
            WheelEvidence.FrontCrossAxleImbalance,
            WheelEvidence.RearCrossAxleImbalance,
            WheelEvidence.SideLoadImbalance,
            CrossAxleRisk,
            State.CrossAxleRiskSeconds,
            bCrossAxleControl ? TEXT("YES") : TEXT("NO"),
            InterventionBrake);
    }
}

bool UGTTNativeTerrainLoadSubsystem::SampleWheelLoadEvidence(const AGTTFieldmasterNativePawn* NativePawn, FWheelLoadEvidence& OutEvidence) const
{
    OutEvidence = FWheelLoadEvidence();
    if (!NativePawn || !GetWorld() || !NativePawn->GetMesh()) return false;

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig)) return false;

    const TArray<FName> WheelBones = {
        Rig.FrontLeftWheelBone,
        Rig.FrontRightWheelBone,
        Rig.RearLeftWheelBone,
        Rig.RearRightWheelBone
    };

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTNativeWheelLoad), false, NativePawn);
    TArray<float> Clearances;
    Clearances.Reserve(4);
    OutEvidence.GroundedWheelCount = 0;

    for (const FName WheelBone : WheelBones)
    {
        if (NativePawn->GetMesh()->GetBoneIndex(WheelBone) == INDEX_NONE) return false;
        const FVector BoneLocation = NativePawn->GetMesh()->GetBoneLocation(WheelBone, EBoneSpaces::WorldSpace);
        FHitResult Hit;
        const FVector Start = BoneLocation + FVector::UpVector * ProbeStartLiftCm;
        const FVector End = BoneLocation - FVector::UpVector * ProbeDepthCm;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
        if (!bHit)
        {
            Clearances.Add(ProbeDepthCm);
            continue;
        }

        const float Clearance = FMath::Max(0.0f, BoneLocation.Z - Hit.ImpactPoint.Z);
        Clearances.Add(Clearance);
        if (Clearance <= ProbeDepthCm * 0.72f)
        {
            ++OutEvidence.GroundedWheelCount;
        }
    }

    if (Clearances.Num() != 4) return false;
    OutEvidence.FrontLeftClearanceCm = Clearances[0];
    OutEvidence.FrontRightClearanceCm = Clearances[1];
    OutEvidence.RearLeftClearanceCm = Clearances[2];
    OutEvidence.RearRightClearanceCm = Clearances[3];

    const float AverageClearance = FMath::Max(1.0f, 0.25f * (Clearances[0] + Clearances[1] + Clearances[2] + Clearances[3]));
    TArray<float> RawLoads;
    RawLoads.Reserve(4);
    float RawTotal = 0.0f;
    for (const float Clearance : Clearances)
    {
        const float Load = FMath::Clamp(AverageClearance / FMath::Max(8.0f, Clearance), 0.35f, 1.80f);
        RawLoads.Add(Load);
        RawTotal += Load;
    }
    if (RawTotal <= KINDA_SMALL_NUMBER) return false;

    const float Scale = 4.0f / RawTotal;
    OutEvidence.FrontLeftLoad = RawLoads[0] * Scale;
    OutEvidence.FrontRightLoad = RawLoads[1] * Scale;
    OutEvidence.RearLeftLoad = RawLoads[2] * Scale;
    OutEvidence.RearRightLoad = RawLoads[3] * Scale;

    OutEvidence.FrontLoadFactor = FMath::Clamp(0.5f * (OutEvidence.FrontLeftLoad + OutEvidence.FrontRightLoad), 0.35f, 1.45f);
    OutEvidence.RearLoadFactor = FMath::Clamp(0.5f * (OutEvidence.RearLeftLoad + OutEvidence.RearRightLoad), 0.35f, 1.45f);
    OutEvidence.FrontCrossAxleImbalance = FMath::Clamp(FMath::Abs(OutEvidence.FrontLeftLoad - OutEvidence.FrontRightLoad) / 1.2f, 0.0f, 1.0f);
    OutEvidence.RearCrossAxleImbalance = FMath::Clamp(FMath::Abs(OutEvidence.RearLeftLoad - OutEvidence.RearRightLoad) / 1.2f, 0.0f, 1.0f);
    const float LeftLoad = OutEvidence.FrontLeftLoad + OutEvidence.RearLeftLoad;
    const float RightLoad = OutEvidence.FrontRightLoad + OutEvidence.RearRightLoad;
    OutEvidence.SideLoadImbalance = FMath::Clamp(FMath::Abs(LeftLoad - RightLoad) / 2.2f, 0.0f, 1.0f);
    return true;
}

AGTTFarmTrailer* UGTTNativeTerrainLoadSubsystem::FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const
{
    UWorld* World = GetWorld();
    if (!World || !NativePawn) return nullptr;

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
