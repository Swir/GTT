#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTAuthoredTrailerPresentationSubsystem.generated.h"

class AGTTFarmTrailer;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class USkeletalMesh;

/**
 * Runtime presentation bridge for the authored farm-trailer rig.
 *
 * The physical AGTTFarmTrailer remains authoritative for collision, hitch load,
 * suspension, wheel loss, cargo integrity and roadside repair. When the accepted
 * project-owned skeletal asset exists at the canonical content path and exposes
 * the required bones/sockets/PhysicsAsset, this subsystem replaces only the
 * placeholder BasicShapes presentation and drives the authored wheel bones from
 * the existing physical wheel bodies.
 */
UCLASS()
class GTT_API UGTTAuthoredTrailerPresentationSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

private:
    struct FRuntimeTrailerVisual
    {
        TWeakObjectPtr<AGTTFarmTrailer> Trailer;
        TWeakObjectPtr<UPoseableMeshComponent> AuthoredVisual;
        TWeakObjectPtr<UStaticMeshComponent> LeftWheel;
        TWeakObjectPtr<UStaticMeshComponent> RightWheel;
        FTransform LeftPhysicalReference = FTransform::Identity;
        FTransform RightPhysicalReference = FTransform::Identity;
        FTransform LeftBoneReference = FTransform::Identity;
        FTransform RightBoneReference = FTransform::Identity;
    };

    void ScanForEligibleTrailers();
    bool TryActivateAuthoredPresentation(AGTTFarmTrailer* Trailer);
    void UpdateWheelPose(FRuntimeTrailerVisual& Runtime) const;
    static bool ValidateAuthoredAsset(USkeletalMesh* Mesh, FString& OutReason);
    static UStaticMeshComponent* FindStaticMeshComponent(AGTTFarmTrailer* Trailer, FName ComponentName);
    static void HidePlaceholderPresentation(AGTTFarmTrailer* Trailer);

    TArray<FRuntimeTrailerVisual> RuntimeVisuals;
    TWeakObjectPtr<USkeletalMesh> CachedAuthoredMesh;
    float ScanAccumulatorSeconds = 0.0f;
};
