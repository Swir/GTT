#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTTrailerAuthoredRuntimeSubsystem.generated.h"

class AGTTFarmTrailer;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FGTTAuthoredTrailerRuntimeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bAuthoredRigValid = false;
    UPROPERTY(BlueprintReadOnly) bool bAuthoredPresentationActive = false;
    UPROPERTY(BlueprintReadOnly) bool bLeftWheelContact = false;
    UPROPERTY(BlueprintReadOnly) bool bRightWheelContact = false;
    UPROPERTY(BlueprintReadOnly) float ContactRatio = 0.0f;
    UPROPERTY(BlueprintReadOnly) float LeftGroundClearanceCm = 0.0f;
    UPROPERTY(BlueprintReadOnly) float RightGroundClearanceCm = 0.0f;
    UPROPERTY(BlueprintReadOnly) float AxleTiltDeg = 0.0f;
    UPROPERTY(BlueprintReadOnly) float HitchAlignmentErrorCm = 0.0f;
    UPROPERTY(BlueprintReadOnly) float ArticulationYawDeg = 0.0f;
    UPROPERTY(BlueprintReadOnly) float StabilizationLoad = 0.0f;
};

UCLASS()
class GTT_API UGTTTrailerAuthoredRuntimeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return !IsTemplate(); }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Native")
    bool IsAuthoredRuntimeActive(const AGTTFarmTrailer* Trailer) const;

    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Native")
    FGTTAuthoredTrailerRuntimeSnapshot GetRuntimeSnapshot(const AGTTFarmTrailer* Trailer) const;

private:
    struct FRuntimeState
    {
        TWeakObjectPtr<USkeletalMeshComponent> Rig;
        FGTTAuthoredTrailerRuntimeSnapshot Snapshot;
        float EvidenceCooldown = 0.0f;
        bool bPresentationTakeover = false;
    };

    void EvaluateTrailer(AGTTFarmTrailer* Trailer, float DeltaSeconds);
    USkeletalMeshComponent* FindAuthoredRig(AGTTFarmTrailer* Trailer) const;
    bool ValidateRig(USkeletalMeshComponent* Rig) const;
    void SetAuthoredPresentation(AGTTFarmTrailer* Trailer, USkeletalMeshComponent* Rig, bool bActive, FRuntimeState& State) const;
    bool TraceWheelContact(AGTTFarmTrailer* Trailer, const FVector& WheelWorld, float& OutGroundClearanceCm) const;
    void ApplyAuthoredDynamics(AGTTFarmTrailer* Trailer, USkeletalMeshComponent* Rig, FRuntimeState& State, float DeltaSeconds) const;

    TMap<TWeakObjectPtr<AGTTFarmTrailer>, FRuntimeState> RuntimeByTrailer;
    float ScanAccumulator = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float ScanIntervalSeconds = 0.05f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float GroundTraceDistanceCm = 105.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float GroundContactSlackCm = 78.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float HitchWarningErrorCm = 80.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float LateralDampingPerSecond = 2.7f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float YawDampingStrength = 68000.0f;
};