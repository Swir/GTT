#include "Vehicles/GTTTrailerNativeAcceptanceSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

namespace
{
    const FName AuthoredTrailerTag(TEXT("GTT.AuthoredTrailerRig"));
    const FName RequiredRootBone(TEXT("root"));
    const FName RequiredLeftWheelBone(TEXT("wheel_l"));
    const FName RequiredRightWheelBone(TEXT("wheel_r"));
    const FName RequiredTowEyeSocket(TEXT("tow_eye"));
    const FName RequiredAxleSocket(TEXT("axle_center"));
    constexpr float EvidenceIntervalSeconds = 2.0f;
}

TStatId UGTTTrailerNativeAcceptanceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTTrailerNativeAcceptanceSubsystem, STATGROUP_Tickables);
}

void UGTTTrailerNativeAcceptanceSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    ScanAccumulator += DeltaSeconds;
    if (ScanAccumulator < ScanIntervalSeconds) return;
    const float Step = ScanAccumulator;
    ScanAccumulator = 0.0f;

    for (auto& Pair : RuntimeByTrailer)
    {
        Pair.Value.EvidenceCooldown = FMath::Max(0.0f, Pair.Value.EvidenceCooldown - Step);
    }

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        EvaluateTrailer(*It, Step);
    }
}

USkeletalMeshComponent* UGTTTrailerNativeAcceptanceSubsystem::FindAuthoredRig(AGTTFarmTrailer* Trailer) const
{
    if (!Trailer) return nullptr;
    TArray<USkeletalMeshComponent*> Meshes;
    Trailer->GetComponents<USkeletalMeshComponent>(Meshes);
    for (USkeletalMeshComponent* Mesh : Meshes)
    {
        if (Mesh && (Mesh->ComponentHasTag(AuthoredTrailerTag) || Mesh->GetFName() == TEXT("AuthoredTrailerMesh")))
        {
            return Mesh;
        }
    }
    return nullptr;
}

bool UGTTTrailerNativeAcceptanceSubsystem::ValidateAuthoredRig(USkeletalMeshComponent* Rig, FString& OutReason) const
{
    if (!Rig)
    {
        OutReason = TEXT("NO_AUTHORED_RIG");
        return false;
    }
    if (!Rig->GetPhysicsAsset())
    {
        OutReason = TEXT("MISSING_PHYSICS_ASSET");
        return false;
    }

    const bool bBones = Rig->GetBoneIndex(RequiredRootBone) != INDEX_NONE &&
        Rig->GetBoneIndex(RequiredLeftWheelBone) != INDEX_NONE &&
        Rig->GetBoneIndex(RequiredRightWheelBone) != INDEX_NONE;
    if (!bBones)
    {
        OutReason = TEXT("MISSING_ROOT_OR_WHEEL_BONES");
        return false;
    }

    const bool bSockets = Rig->DoesSocketExist(RequiredTowEyeSocket) && Rig->DoesSocketExist(RequiredAxleSocket);
    if (!bSockets)
    {
        OutReason = TEXT("MISSING_TOW_EYE_OR_AXLE_SOCKET");
        return false;
    }

    OutReason = TEXT("AUTHORED_RIG_READY");
    return true;
}

bool UGTTTrailerNativeAcceptanceSubsystem::EvaluateNativeHitch(AGTTFarmTrailer* Trailer, AGTTFieldmasterNativePawn* NativeTow, FGTTTrailerNativeAcceptanceState& State, FString& OutReason) const
{
    if (!Trailer || !NativeTow || !Trailer->IsAttachedToNativeFieldmaster())
    {
        OutReason = TEXT("LEGACY_OR_DETACHED");
        return false;
    }
    if (!NativeTow->IsNativeFieldmasterReady() || !NativeTow->IsLegacyTakeoverActive())
    {
        OutReason = TEXT("NATIVE_TOW_NOT_ACCEPTED");
        return false;
    }

    FTransform RearHitch;
    if (!NativeTow->TryGetRearHitchTransform(RearHitch))
    {
        OutReason = TEXT("NATIVE_REAR_HITCH_MISSING");
        return false;
    }

    const FVector TrailerCoupler = Trailer->GetActorTransform().TransformPosition(FVector(-495.0f, 0.0f, -2.0f));
    State.HitchErrorCm = FVector::Distance(RearHitch.GetLocation(), TrailerCoupler);
    State.bHitchAligned = State.HitchErrorCm <= MaxHitchAlignmentErrorCm;

    const FVector TowForward = NativeTow->GetActorForwardVector().GetSafeNormal2D();
    const FVector TrailerForward = Trailer->GetActorForwardVector().GetSafeNormal2D();
    const float Dot = FMath::Clamp(FVector::DotProduct(TowForward, TrailerForward), -1.0f, 1.0f);
    State.ArticulationYawDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

    if (!State.bHitchAligned)
    {
        OutReason = TEXT("HITCH_ALIGNMENT_ERROR");
        return false;
    }
    if (State.ArticulationYawDeg >= JackknifeDetachYawDeg)
    {
        OutReason = TEXT("JACKKNIFE_LIMIT");
        return false;
    }

    OutReason = State.ArticulationYawDeg >= JackknifeWarningYawDeg ? TEXT("JACKKNIFE_WARNING") : TEXT("NATIVE_HITCH_ACCEPTED");
    return true;
}

void UGTTTrailerNativeAcceptanceSubsystem::EvaluateTrailer(AGTTFarmTrailer* Trailer, float DeltaSeconds)
{
    if (!Trailer) return;
    FGTTTrailerNativeAcceptanceState& State = RuntimeByTrailer.FindOrAdd(Trailer);
    USkeletalMeshComponent* Rig = FindAuthoredRig(Trailer);
    State.bAuthoredRigPresent = Rig != nullptr;
    State.bPhysicsAssetReady = Rig && Rig->GetPhysicsAsset();
    State.bRequiredBonesReady = Rig && Rig->GetBoneIndex(RequiredRootBone) != INDEX_NONE && Rig->GetBoneIndex(RequiredLeftWheelBone) != INDEX_NONE && Rig->GetBoneIndex(RequiredRightWheelBone) != INDEX_NONE;
    State.bRequiredSocketsReady = Rig && Rig->DoesSocketExist(RequiredTowEyeSocket) && Rig->DoesSocketExist(RequiredAxleSocket);

    FString RigReason;
    const bool bRigAccepted = ValidateAuthoredRig(Rig, RigReason);

    AGTTFieldmasterNativePawn* NativeTow = Cast<AGTTFieldmasterNativePawn>(Trailer->GetTowActor());
    FString HitchReason;
    State.bNativeTowReady = EvaluateNativeHitch(Trailer, NativeTow, State, HitchReason);
    State.bRuntimeAccepted = bRigAccepted && State.bNativeTowReady && Trailer->HasIntactAxle();

    if (Trailer->IsAttachedToNativeFieldmaster() && State.bAuthoredRigPresent && !State.bRuntimeAccepted)
    {
        State.InvalidSeconds += DeltaSeconds;
        if (State.InvalidSeconds >= InvalidGraceSeconds)
        {
            UE_LOG(LogTemp, Warning, TEXT("NATIVE_TRAILER_FAILSAFE trailer=%s rig=%s hitch=%s yaw=%.1f error=%.1f action=DETACH"), *GetNameSafe(Trailer), *RigReason, *HitchReason, State.ArticulationYawDeg, State.HitchErrorCm);
            Trailer->DetachTrailer();
            State.InvalidSeconds = 0.0f;
        }
    }
    else
    {
        State.InvalidSeconds = 0.0f;
    }

    if (State.EvidenceCooldown <= 0.0f)
    {
        EmitEvidence(Trailer, State, State.bRuntimeAccepted ? TEXT("ACCEPTED") : FString::Printf(TEXT("%s/%s"), *RigReason, *HitchReason));
        State.EvidenceCooldown = EvidenceIntervalSeconds;
    }
}

void UGTTTrailerNativeAcceptanceSubsystem::EmitEvidence(AGTTFarmTrailer* Trailer, const FGTTTrailerNativeAcceptanceState& State, const FString& Reason) const
{
    UE_LOG(LogTemp, Display, TEXT("NATIVE_TRAILER_ACCEPTANCE_EVIDENCE trailer=%s authored=%d physics=%d bones=%d sockets=%d nativeTow=%d aligned=%d axle=%d accepted=%d yaw=%.1f hitchError=%.1f reason=%s"),
        *GetNameSafe(Trailer), State.bAuthoredRigPresent ? 1 : 0, State.bPhysicsAssetReady ? 1 : 0, State.bRequiredBonesReady ? 1 : 0,
        State.bRequiredSocketsReady ? 1 : 0, State.bNativeTowReady ? 1 : 0, State.bHitchAligned ? 1 : 0,
        Trailer && Trailer->HasIntactAxle() ? 1 : 0, State.bRuntimeAccepted ? 1 : 0, State.ArticulationYawDeg, State.HitchErrorCm, *Reason);

    if (Trailer && Trailer->IsAttachedToNativeFieldmaster() && State.ArticulationYawDeg >= JackknifeWarningYawDeg && State.ArticulationYawDeg < JackknifeDetachYawDeg)
    {
        UE_LOG(LogTemp, Warning, TEXT("NATIVE_TRAILER_JACKKNIFE_WARNING trailer=%s yaw=%.1f limit=%.1f"), *GetNameSafe(Trailer), State.ArticulationYawDeg, JackknifeDetachYawDeg);
    }
}
