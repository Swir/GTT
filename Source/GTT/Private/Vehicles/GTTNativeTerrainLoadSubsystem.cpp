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
    constexpr float RollbackTriggerKmh = -1.2f;
    constexpr float RollbackGraceSeconds = 0.12f;
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
    const float MudSeverity = NativePawn->GetNativeMudSeverity();
    const float TerrainGrip = NativePawn->GetNativeTerrainGripFactor();
    const float RequestedThrottle = FMath::Abs(NativePawn->GetRequestedThrottleInput());

    AGTTFarmTrailer* Trailer = FindAttachedTrailer(NativePawn);
    const float TowLoad = Trailer ? Trailer->GetTowLoadFactor() : 0.0f;

    float FrontClearanceCm = 0.0f;
    float RearClearanceCm = 0.0f;
    const bool bAxleEvidence = SampleAxleClearance(NativePawn, FrontClearanceCm, RearClearanceCm);
    const float ClearanceBias = bAxleEvidence
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
    }

    State.LastThrottleLimit = ThrottleLimit;
    State.LastRearLoadBias = RearLoadBias;
    State.LastLaunchGrip = LaunchGrip;
    State.bLastRollbackControl = bRollbackControl;

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
}

bool UGTTNativeTerrainLoadSubsystem::SampleAxleClearance(const AGTTFieldmasterNativePawn* NativePawn, float& OutFrontCm, float& OutRearCm) const
{
    OutFrontCm = 0.0f;
    OutRearCm = 0.0f;
    if (!NativePawn || !GetWorld() || !NativePawn->GetMesh()) return false;

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig)) return false;

    const TArray<FName> WheelBones = {
        Rig.FrontLeftWheelBone,
        Rig.FrontRightWheelBone,
        Rig.RearLeftWheelBone,
        Rig.RearRightWheelBone
    };

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTNativeTerrainLoad), false, NativePawn);
    TArray<float> Clearances;
    Clearances.Reserve(4);

    for (const FName WheelBone : WheelBones)
    {
        if (NativePawn->GetMesh()->GetBoneIndex(WheelBone) == INDEX_NONE) return false;
        const FVector BoneLocation = NativePawn->GetMesh()->GetBoneLocation(WheelBone, EBoneSpaces::WorldSpace);
        FHitResult Hit;
        const FVector Start = BoneLocation + FVector::UpVector * ProbeStartLiftCm;
        const FVector End = BoneLocation - FVector::UpVector * ProbeDepthCm;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) return false;
        Clearances.Add(FMath::Max(0.0f, BoneLocation.Z - Hit.ImpactPoint.Z));
    }

    OutFrontCm = 0.5f * (Clearances[0] + Clearances[1]);
    OutRearCm = 0.5f * (Clearances[2] + Clearances[3]);
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
