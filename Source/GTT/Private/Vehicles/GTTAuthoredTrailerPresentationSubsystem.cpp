#include "Vehicles/GTTAuthoredTrailerPresentationSubsystem.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "GTT.h"

namespace
{
    constexpr float TrailerPresentationScanIntervalSeconds = 1.0f;
    const TCHAR* const AuthoredTrailerAssetPath = TEXT("/Game/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer.SK_GTT_FarmTrailer");

    const FName BodyBoneName(TEXT("body"));
    const FName LeftWheelBoneName(TEXT("wheel_l"));
    const FName RightWheelBoneName(TEXT("wheel_r"));
    const FName HitchSocketName(TEXT("socket_hitch"));
    const FName CargoSocketName(TEXT("socket_cargo"));
    const FName LeftAxleSocketName(TEXT("socket_axle_l"));
    const FName RightAxleSocketName(TEXT("socket_axle_r"));
    const FName LeftWheelComponentName(TEXT("LeftWheel"));
    const FName RightWheelComponentName(TEXT("RightWheel"));

    bool IsPlaceholderPresentationComponent(const FName Name)
    {
        return Name == TEXT("TrailerBody") || Name == TEXT("LeftWheel") || Name == TEXT("RightWheel") ||
            Name == TEXT("Drawbar") || Name == TEXT("HitchCoupler") || Name == TEXT("FrontRail") ||
            Name == TEXT("LeftRail") || Name == TEXT("RightRail") || Name == TEXT("Tailgate") ||
            Name == TEXT("LeftFender") || Name == TEXT("RightFender") || Name == TEXT("RearReflectorBar");
    }

    FTransform BuildDrivenWheelBoneTransform(
        const FTransform& PhysicalReference,
        const FTransform& PhysicalCurrent,
        const FTransform& BoneReference)
    {
        FTransform Result = BoneReference;
        Result.SetTranslation(BoneReference.GetTranslation() + (PhysicalCurrent.GetTranslation() - PhysicalReference.GetTranslation()));

        FQuat RotationDelta = PhysicalCurrent.GetRotation() * PhysicalReference.GetRotation().Inverse();
        RotationDelta.Normalize();
        FQuat DrivenRotation = RotationDelta * BoneReference.GetRotation();
        DrivenRotation.Normalize();
        Result.SetRotation(DrivenRotation);
        return Result;
    }
}

TStatId UGTTAuthoredTrailerPresentationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTAuthoredTrailerPresentationSubsystem, STATGROUP_Tickables);
}

void UGTTAuthoredTrailerPresentationSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)) return;

    for (int32 Index = RuntimeVisuals.Num() - 1; Index >= 0; --Index)
    {
        FRuntimeTrailerVisual& Runtime = RuntimeVisuals[Index];
        if (!Runtime.Trailer.IsValid() || !Runtime.AuthoredVisual.IsValid())
        {
            RuntimeVisuals.RemoveAtSwap(Index);
            continue;
        }
        UpdateWheelPose(Runtime);
    }

    ScanAccumulatorSeconds += DeltaTime;
    if (ScanAccumulatorSeconds < TrailerPresentationScanIntervalSeconds) return;
    ScanAccumulatorSeconds = 0.0f;
    ScanForEligibleTrailers();
}

void UGTTAuthoredTrailerPresentationSubsystem::ScanForEligibleTrailers()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        AGTTFarmTrailer* Trailer = *It;
        if (!IsValid(Trailer)) continue;

        const bool bAlreadyActive = RuntimeVisuals.ContainsByPredicate(
            [Trailer](const FRuntimeTrailerVisual& Runtime)
            {
                return Runtime.Trailer.Get() == Trailer;
            });
        if (!bAlreadyActive) TryActivateAuthoredPresentation(Trailer);
    }
}

bool UGTTAuthoredTrailerPresentationSubsystem::ValidateAuthoredAsset(USkeletalMesh* Mesh, FString& OutReason)
{
    if (!Mesh)
    {
        OutReason = TEXT("skeletal asset missing");
        return false;
    }

    const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
    for (const FName BoneName : {BodyBoneName, LeftWheelBoneName, RightWheelBoneName})
    {
        if (RefSkeleton.FindBoneIndex(BoneName) == INDEX_NONE)
        {
            OutReason = FString::Printf(TEXT("required bone missing: %s"), *BoneName.ToString());
            return false;
        }
    }

    for (const FName SocketName : {HitchSocketName, CargoSocketName, LeftAxleSocketName, RightAxleSocketName})
    {
        if (!Mesh->FindSocket(SocketName))
        {
            OutReason = FString::Printf(TEXT("required socket missing: %s"), *SocketName.ToString());
            return false;
        }
    }

    if (!Mesh->GetPhysicsAsset())
    {
        OutReason = TEXT("PhysicsAsset missing");
        return false;
    }

    OutReason.Reset();
    return true;
}

UStaticMeshComponent* UGTTAuthoredTrailerPresentationSubsystem::FindStaticMeshComponent(AGTTFarmTrailer* Trailer, FName ComponentName)
{
    if (!Trailer) return nullptr;
    TArray<UStaticMeshComponent*> Components;
    Trailer->GetComponents<UStaticMeshComponent>(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (Component && Component->GetFName() == ComponentName) return Component;
    }
    return nullptr;
}

void UGTTAuthoredTrailerPresentationSubsystem::HidePlaceholderPresentation(AGTTFarmTrailer* Trailer)
{
    if (!Trailer) return;
    TArray<UStaticMeshComponent*> Components;
    Trailer->GetComponents<UStaticMeshComponent>(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (!Component || !IsPlaceholderPresentationComponent(Component->GetFName())) continue;
        Component->SetVisibility(false, false);
        Component->SetHiddenInGame(true, false);
    }
}

bool UGTTAuthoredTrailerPresentationSubsystem::TryActivateAuthoredPresentation(AGTTFarmTrailer* Trailer)
{
    if (!Trailer || !Trailer->GetRootComponent()) return false;

    USkeletalMesh* AuthoredMesh = CachedAuthoredMesh.Get();
    if (!AuthoredMesh)
    {
        AuthoredMesh = LoadObject<USkeletalMesh>(nullptr, AuthoredTrailerAssetPath);
        CachedAuthoredMesh = AuthoredMesh;
    }

    FString ValidationFailure;
    if (!ValidateAuthoredAsset(AuthoredMesh, ValidationFailure))
    {
        if (AuthoredMesh)
        {
            UE_LOG(LogGTT, Warning, TEXT("AUTHORED_TRAILER_PRESENTATION event=REJECTED actor=%s reason=%s"), *GetNameSafe(Trailer), *ValidationFailure);
        }
        return false;
    }

    UStaticMeshComponent* LeftWheel = FindStaticMeshComponent(Trailer, LeftWheelComponentName);
    UStaticMeshComponent* RightWheel = FindStaticMeshComponent(Trailer, RightWheelComponentName);
    if (!LeftWheel || !RightWheel) return false;

    UPoseableMeshComponent* AuthoredVisual = NewObject<UPoseableMeshComponent>(Trailer);
    if (!AuthoredVisual) return false;
    AuthoredVisual->SetupAttachment(Trailer->GetRootComponent());
    AuthoredVisual->SetSkeletalMesh(AuthoredMesh);
    AuthoredVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AuthoredVisual->SetGenerateOverlapEvents(false);
    AuthoredVisual->SetCastShadow(true);
    AuthoredVisual->RegisterComponent();
    AuthoredVisual->SetRelativeTransform(FTransform::Identity);

    FRuntimeTrailerVisual Runtime;
    Runtime.Trailer = Trailer;
    Runtime.AuthoredVisual = AuthoredVisual;
    Runtime.LeftWheel = LeftWheel;
    Runtime.RightWheel = RightWheel;
    const FTransform RootTransform = Trailer->GetRootComponent()->GetComponentTransform();
    Runtime.LeftPhysicalReference = LeftWheel->GetComponentTransform().GetRelativeTransform(RootTransform);
    Runtime.RightPhysicalReference = RightWheel->GetComponentTransform().GetRelativeTransform(RootTransform);
    Runtime.LeftBoneReference = AuthoredVisual->GetBoneTransformByName(LeftWheelBoneName, EBoneSpaces::ComponentSpace);
    Runtime.RightBoneReference = AuthoredVisual->GetBoneTransformByName(RightWheelBoneName, EBoneSpaces::ComponentSpace);

    HidePlaceholderPresentation(Trailer);
    RuntimeVisuals.Add(Runtime);
    UpdateWheelPose(RuntimeVisuals.Last());

    UE_LOG(LogGTT, Display, TEXT("AUTHORED_TRAILER_PRESENTATION event=ACTIVATED actor=%s asset=%s bones=body,wheel_l,wheel_r sockets=socket_hitch,socket_cargo,socket_axle_l,socket_axle_r physics_asset=YES"), *GetNameSafe(Trailer), AuthoredTrailerAssetPath);
    return true;
}

void UGTTAuthoredTrailerPresentationSubsystem::UpdateWheelPose(FRuntimeTrailerVisual& Runtime) const
{
    AGTTFarmTrailer* Trailer = Runtime.Trailer.Get();
    UPoseableMeshComponent* AuthoredVisual = Runtime.AuthoredVisual.Get();
    UStaticMeshComponent* LeftWheel = Runtime.LeftWheel.Get();
    UStaticMeshComponent* RightWheel = Runtime.RightWheel.Get();
    if (!Trailer || !AuthoredVisual || !LeftWheel || !RightWheel || !Trailer->GetRootComponent()) return;

    const FTransform RootTransform = Trailer->GetRootComponent()->GetComponentTransform();
    const FTransform LeftCurrent = LeftWheel->GetComponentTransform().GetRelativeTransform(RootTransform);
    const FTransform RightCurrent = RightWheel->GetComponentTransform().GetRelativeTransform(RootTransform);

    AuthoredVisual->SetBoneTransformByName(
        LeftWheelBoneName,
        BuildDrivenWheelBoneTransform(Runtime.LeftPhysicalReference, LeftCurrent, Runtime.LeftBoneReference),
        EBoneSpaces::ComponentSpace);
    AuthoredVisual->SetBoneTransformByName(
        RightWheelBoneName,
        BuildDrivenWheelBoneTransform(Runtime.RightPhysicalReference, RightCurrent, Runtime.RightBoneReference),
        EBoneSpaces::ComponentSpace);
}
