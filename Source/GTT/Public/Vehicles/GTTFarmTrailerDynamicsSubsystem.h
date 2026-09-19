#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmTrailerDynamicsSubsystem.generated.h"

class AGTTFarmTrailer;
class UPhysicsConstraintComponent;

USTRUCT(BlueprintType)
struct GTT_API FGTTFarmTrailerDynamicsSnapshot
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bConfigured = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bAttached = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bCargoLoaded = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bLeftSuspensionActive = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bRightSuspensionActive = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHitchBreakable = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SuspensionTravelCm = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float SpringStrength = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DampingStrength = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float HitchBreakForce = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float HitchBreakTorque = 0.0f;
};

/**
 * Runtime physical-dynamics layer for the articulated farm trailer.
 *
 * The trailer actor owns cargo/economy/damage state. This subsystem owns only the
 * physical suspension and hitch-break tuning so towing remains connected to the
 * existing gameplay loop without creating a second cargo or payout authority.
 */
UCLASS()
class GTT_API UGTTFarmTrailerDynamicsSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Dynamics")
    FGTTFarmTrailerDynamicsSnapshot GetSnapshot(const AGTTFarmTrailer* Trailer) const;

private:
    struct FRuntimeState
    {
        FGTTFarmTrailerDynamicsSnapshot Snapshot;
        float RefreshSeconds = 0.0f;
        float EvidenceSeconds = 0.0f;
    };

    void EvaluateTrailer(AGTTFarmTrailer* Trailer, float DeltaSeconds);
    UPhysicsConstraintComponent* FindConstraintByName(AGTTFarmTrailer* Trailer, FName ConstraintName) const;
    bool ConfigureSuspensionConstraint(UPhysicsConstraintComponent* Constraint, float SpringStrength, float DampingStrength) const;

    TMap<TWeakObjectPtr<AGTTFarmTrailer>, FRuntimeState> RuntimeByTrailer;
};
