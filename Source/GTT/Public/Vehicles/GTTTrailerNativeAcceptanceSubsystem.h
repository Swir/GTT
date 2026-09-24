#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTTrailerNativeAcceptanceSubsystem.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;
class USkeletalMeshComponent;

USTRUCT()
struct FGTTTrailerNativeAcceptanceState
{
    GENERATED_BODY()

    bool bAuthoredRigPresent = false;
    bool bPhysicsAssetReady = false;
    bool bRequiredBonesReady = false;
    bool bRequiredSocketsReady = false;
    bool bNativeTowReady = false;
    bool bHitchAligned = false;
    bool bRuntimeAccepted = false;
    float ArticulationYawDeg = 0.0f;
    float HitchErrorCm = 0.0f;
    float EvidenceCooldown = 0.0f;
    float InvalidSeconds = 0.0f;
};

UCLASS()
class GTT_API UGTTTrailerNativeAcceptanceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }

private:
    void EvaluateTrailer(AGTTFarmTrailer* Trailer, float DeltaSeconds);
    USkeletalMeshComponent* FindAuthoredRig(AGTTFarmTrailer* Trailer) const;
    bool ValidateAuthoredRig(USkeletalMeshComponent* Rig, FString& OutReason) const;
    bool EvaluateNativeHitch(AGTTFarmTrailer* Trailer, AGTTFieldmasterNativePawn* NativeTow, FGTTTrailerNativeAcceptanceState& State, FString& OutReason) const;
    void EmitEvidence(AGTTFarmTrailer* Trailer, const FGTTTrailerNativeAcceptanceState& State, const FString& Reason) const;

    TMap<TWeakObjectPtr<AGTTFarmTrailer>, FGTTTrailerNativeAcceptanceState> RuntimeByTrailer;
    float ScanAccumulator = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float ScanIntervalSeconds = 0.25f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float MaxHitchAlignmentErrorCm = 85.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float JackknifeWarningYawDeg = 58.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float JackknifeDetachYawDeg = 76.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Native") float InvalidGraceSeconds = 0.75f;
};
