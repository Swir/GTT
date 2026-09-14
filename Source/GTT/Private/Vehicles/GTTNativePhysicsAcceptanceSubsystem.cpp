#include "Vehicles/GTTNativePhysicsAcceptanceSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float EvidenceIntervalSeconds = 4.0f;
    constexpr float InvalidFallbackSeconds = 1.5f;
    constexpr float MinimumWheelPairSeparationCm = 60.0f;
    constexpr float MinimumAxleSeparationCm = 90.0f;
    constexpr float ProbeStartLiftCm = 25.0f;
    constexpr float ProbeDepthCm = 145.0f;
}

void UGTTNativePhysicsAcceptanceSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        EvaluateNativeFieldmaster(*It, DeltaTime);
    }
}

TStatId UGTTNativePhysicsAcceptanceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTNativePhysicsAcceptanceSubsystem, STATGROUP_Tickables);
}

bool UGTTNativePhysicsAcceptanceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTNativePhysicsAcceptanceSubsystem::EvaluateNativeFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime)
{
    if (!NativePawn)
    {
        return;
    }

    const TWeakObjectPtr<AGTTFieldmasterNativePawn> Key(NativePawn);
    if (!NativePawn->IsLegacyTakeoverActive())
    {
        AcceptanceStates.Remove(Key);
        return;
    }

    FAcceptanceState& State = AcceptanceStates.FindOrAdd(Key);
    FString ValidationReason;
    const bool bAuthoredPhysicsValid = ValidateAuthoredPhysics(NativePawn, ValidationReason);

    if (bAuthoredPhysicsValid)
    {
        State.InvalidSeconds = 0.0f;
    }
    else
    {
        State.InvalidSeconds += DeltaTime;
        if (State.InvalidSeconds >= InvalidFallbackSeconds)
        {
            UE_LOG(LogGTT, Error,
                TEXT("NATIVE_PHYSICS_FALLBACK vehicle=RustyFieldmaster60 invalid_seconds=%.2f reason=\"%s\""),
                State.InvalidSeconds,
                *ValidationReason);
            NativePawn->DeactivateLegacyTakeover();
            AcceptanceStates.Remove(Key);
            return;
        }
    }

    TArray<float> ClearancesCm;
    State.LastGroundContacts = SampleGroundContacts(NativePawn, ClearancesCm);
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

    const UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    const USkeletalMeshComponent* Mesh = NativePawn->GetMesh();
    const UPhysicsAsset* PhysicsAsset = Mesh ? Mesh->GetPhysicsAsset() : nullptr;

    UE_LOG(LogGTT, Log,
        TEXT("NATIVE_PHYSICS_EVIDENCE vehicle=RustyFieldmaster60 accepted=%s contacts=%d/4 clearances_cm=[%.1f,%.1f,%.1f,%.1f] physics_asset=%s body_count=%d movement=%s collision=%d reason=\"%s\""),
        bAuthoredPhysicsValid ? TEXT("YES") : TEXT("NO"),
        State.LastGroundContacts,
        ClearancesCm[0], ClearancesCm[1], ClearancesCm[2], ClearancesCm[3],
        PhysicsAsset ? *PhysicsAsset->GetName() : TEXT("NONE"),
        PhysicsAsset ? PhysicsAsset->SkeletalBodySetups.Num() : 0,
        Movement && Movement->IsActive() ? TEXT("ACTIVE") : TEXT("INACTIVE"),
        Mesh ? static_cast<int32>(Mesh->GetCollisionEnabled()) : -1,
        *ValidationReason);
}

bool UGTTNativePhysicsAcceptanceSubsystem::ValidateAuthoredPhysics(AGTTFieldmasterNativePawn* NativePawn, FString& OutReason) const
{
    if (!NativePawn)
    {
        OutReason = TEXT("native pawn missing");
        return false;
    }

    USkeletalMeshComponent* Mesh = NativePawn->GetMesh();
    if (!Mesh)
    {
        OutReason = TEXT("skeletal mesh component missing");
        return false;
    }

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativePawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive())
    {
        OutReason = TEXT("Chaos wheeled movement inactive");
        return false;
    }

    UPhysicsAsset* PhysicsAsset = Mesh->GetPhysicsAsset();
    if (!PhysicsAsset || PhysicsAsset->SkeletalBodySetups.Num() == 0)
    {
        OutReason = TEXT("Physics Asset missing or contains no rigid bodies");
        return false;
    }

    if (Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
    {
        OutReason = TEXT("vehicle mesh collision disabled");
        return false;
    }

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig))
    {
        OutReason = TEXT("Fieldmaster rig contract unavailable");
        return false;
    }

    if (PhysicsAsset->FindBodyIndex(Rig.RootBone) == INDEX_NONE)
    {
        OutReason = FString::Printf(TEXT("Physics Asset has no root body for %s"), *Rig.RootBone.ToString());
        return false;
    }

    const TArray<FName> WheelBones = {
        Rig.FrontLeftWheelBone,
        Rig.FrontRightWheelBone,
        Rig.RearLeftWheelBone,
        Rig.RearRightWheelBone
    };

    TArray<FVector> WheelLocations;
    WheelLocations.Reserve(4);
    for (const FName WheelBone : WheelBones)
    {
        if (WheelBone.IsNone() || Mesh->GetBoneIndex(WheelBone) == INDEX_NONE)
        {
            OutReason = FString::Printf(TEXT("required wheel bone missing: %s"), *WheelBone.ToString());
            return false;
        }
        WheelLocations.Add(Mesh->GetBoneLocation(WheelBone, EBoneSpaces::WorldSpace));
    }

    const float FrontTrack = FVector::Distance(WheelLocations[0], WheelLocations[1]);
    const float RearTrack = FVector::Distance(WheelLocations[2], WheelLocations[3]);
    const FVector FrontCenter = (WheelLocations[0] + WheelLocations[1]) * 0.5f;
    const FVector RearCenter = (WheelLocations[2] + WheelLocations[3]) * 0.5f;
    const float Wheelbase = FVector::Distance(FrontCenter, RearCenter);

    if (FrontTrack < MinimumWheelPairSeparationCm || RearTrack < MinimumWheelPairSeparationCm || Wheelbase < MinimumAxleSeparationCm)
    {
        OutReason = FString::Printf(TEXT("degenerate wheel geometry front=%.1f rear=%.1f wheelbase=%.1f"), FrontTrack, RearTrack, Wheelbase);
        return false;
    }

    OutReason = FString::Printf(TEXT("authored physics valid bodies=%d front_track=%.1f rear_track=%.1f wheelbase=%.1f"),
        PhysicsAsset->SkeletalBodySetups.Num(), FrontTrack, RearTrack, Wheelbase);
    return true;
}

int32 UGTTNativePhysicsAcceptanceSubsystem::SampleGroundContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const
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

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GTTNativeWheelGroundProbe), false, NativePawn);
    int32 ContactCount = 0;

    for (const FName WheelBone : WheelBones)
    {
        const FVector WheelLocation = NativePawn->GetMesh()->GetBoneLocation(WheelBone, EBoneSpaces::WorldSpace);
        const FVector Start = WheelLocation + FVector(0.0f, 0.0f, ProbeStartLiftCm);
        const FVector End = WheelLocation - FVector(0.0f, 0.0f, ProbeDepthCm);
        FHitResult Hit;
        const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
        if (bHit)
        {
            ++ContactCount;
            OutClearancesCm.Add(FMath::Max(0.0f, WheelLocation.Z - Hit.ImpactPoint.Z));
        }
        else
        {
            OutClearancesCm.Add(-1.0f);
        }
    }

    return ContactCount;
}
