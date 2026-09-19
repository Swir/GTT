#include "Vehicles/GTTFarmTrailerDynamicsSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "GTT.h"

namespace
{
    const FName LeftWheelConstraintName(TEXT("LeftWheelConstraint"));
    const FName RightWheelConstraintName(TEXT("RightWheelConstraint"));
    const FName HitchConstraintName(TEXT("HitchConstraint"));

    constexpr float DynamicsRefreshIntervalSeconds = 0.25f;
    constexpr float EvidenceIntervalSeconds = 4.0f;
    constexpr float SuspensionTravelCm = 20.0f;
    constexpr float EmptySpringStrength = 52000.0f;
    constexpr float LoadedSpringMultiplier = 1.45f;
    constexpr float EmptyDampingStrength = 6400.0f;
    constexpr float LoadedDampingMultiplier = 1.30f;
    constexpr float SuspensionForceLimit = 280000.0f;
    constexpr float EmptyHitchBreakForce = 620000.0f;
    constexpr float EmptyHitchBreakTorque = 340000.0f;
    constexpr float LoadedHitchStrengthMultiplier = 0.88f;
    constexpr float MinimumIntegrityStrengthFactor = 0.55f;
    constexpr float MaximumDynamicHitchWeakening = 0.28f;
}

TStatId UGTTFarmTrailerDynamicsSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmTrailerDynamicsSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmTrailerDynamicsSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

FGTTFarmTrailerDynamicsSnapshot UGTTFarmTrailerDynamicsSubsystem::GetSnapshot(const AGTTFarmTrailer* Trailer) const
{
    if (!Trailer) return FGTTFarmTrailerDynamicsSnapshot();
    const TWeakObjectPtr<AGTTFarmTrailer> Key(const_cast<AGTTFarmTrailer*>(Trailer));
    const FRuntimeState* State = RuntimeByTrailer.Find(Key);
    return State ? State->Snapshot : FGTTFarmTrailerDynamicsSnapshot();
}

UPhysicsConstraintComponent* UGTTFarmTrailerDynamicsSubsystem::FindConstraintByName(AGTTFarmTrailer* Trailer, FName ConstraintName) const
{
    if (!Trailer) return nullptr;

    TArray<UPhysicsConstraintComponent*> Constraints;
    Trailer->GetComponents<UPhysicsConstraintComponent>(Constraints);
    for (UPhysicsConstraintComponent* Constraint : Constraints)
    {
        if (Constraint && Constraint->GetFName() == ConstraintName)
        {
            return Constraint;
        }
    }
    return nullptr;
}

bool UGTTFarmTrailerDynamicsSubsystem::ConfigureSuspensionConstraint(
    UPhysicsConstraintComponent* Constraint,
    float SpringStrength,
    float DampingStrength) const
{
    if (!Constraint || Constraint->IsBroken()) return false;

    Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, SuspensionTravelCm);
    Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.0f);

    Constraint->SetLinearPositionDrive(false, false, true);
    Constraint->SetLinearVelocityDrive(false, false, true);
    Constraint->SetLinearDriveAccelerationMode(true);
    Constraint->SetLinearPositionTarget(FVector::ZeroVector);
    Constraint->SetLinearVelocityTarget(FVector::ZeroVector);
    Constraint->SetLinearDriveParams(SpringStrength, DampingStrength, SuspensionForceLimit);
    return true;
}

void UGTTFarmTrailerDynamicsSubsystem::EvaluateTrailer(AGTTFarmTrailer* Trailer, float DeltaSeconds)
{
    if (!Trailer) return;

    const TWeakObjectPtr<AGTTFarmTrailer> TrailerKey(Trailer);
    FRuntimeState& State = RuntimeByTrailer.FindOrAdd(TrailerKey);
    State.RefreshSeconds = FMath::Max(0.0f, State.RefreshSeconds - DeltaSeconds);
    State.EvidenceSeconds += DeltaSeconds;

    UPhysicsConstraintComponent* HitchConstraint = FindConstraintByName(Trailer, HitchConstraintName);
    if (Trailer->IsAttached() && HitchConstraint && HitchConstraint->IsBroken())
    {
        UE_LOG(LogGTT, Warning,
            TEXT("TRAILER_HITCH_PHYSICS_BREAK cargo=%s integrity=%.2f hitch_load=%.2f"),
            Trailer->HasCargo() ? TEXT("LOADED") : TEXT("EMPTY"),
            Trailer->GetTrailerIntegrity(), Trailer->GetHitchLoad());
        Trailer->DetachTrailer();
    }

    if (State.RefreshSeconds <= 0.0f)
    {
        State.RefreshSeconds = DynamicsRefreshIntervalSeconds;

        const bool bLoaded = Trailer->HasCargo();
        const float Integrity01 = FMath::Clamp(Trailer->GetTrailerIntegrity(), 0.0f, 1.0f);
        const float HitchLoad01 = FMath::Clamp(Trailer->GetHitchLoad(), 0.0f, 1.0f);
        const float SpringStrength = EmptySpringStrength * (bLoaded ? LoadedSpringMultiplier : 1.0f);
        const float DampingStrength = EmptyDampingStrength * (bLoaded ? LoadedDampingMultiplier : 1.0f);

        UPhysicsConstraintComponent* LeftConstraint = FindConstraintByName(Trailer, LeftWheelConstraintName);
        UPhysicsConstraintComponent* RightConstraint = FindConstraintByName(Trailer, RightWheelConstraintName);
        const bool bLeftSuspension = ConfigureSuspensionConstraint(LeftConstraint, SpringStrength, DampingStrength);
        const bool bRightSuspension = ConfigureSuspensionConstraint(RightConstraint, SpringStrength, DampingStrength);

        const float IntegrityStrengthFactor = FMath::Lerp(MinimumIntegrityStrengthFactor, 1.0f, Integrity01);
        const float CargoStrengthFactor = bLoaded ? LoadedHitchStrengthMultiplier : 1.0f;
        const float DynamicStressFactor = 1.0f - HitchLoad01 * MaximumDynamicHitchWeakening;
        const float HitchBreakForce = EmptyHitchBreakForce * IntegrityStrengthFactor * CargoStrengthFactor * DynamicStressFactor;
        const float HitchBreakTorque = EmptyHitchBreakTorque * IntegrityStrengthFactor * CargoStrengthFactor * DynamicStressFactor;

        bool bHitchBreakable = false;
        if (HitchConstraint && !HitchConstraint->IsBroken())
        {
            HitchConstraint->SetLinearBreakable(true, HitchBreakForce);
            HitchConstraint->SetAngularBreakable(true, HitchBreakTorque);
            bHitchBreakable = true;
        }

        State.Snapshot.bConfigured = bLeftSuspension || bRightSuspension || bHitchBreakable;
        State.Snapshot.bAttached = Trailer->IsAttached();
        State.Snapshot.bCargoLoaded = bLoaded;
        State.Snapshot.bLeftSuspensionActive = bLeftSuspension;
        State.Snapshot.bRightSuspensionActive = bRightSuspension;
        State.Snapshot.bHitchBreakable = bHitchBreakable;
        State.Snapshot.SuspensionTravelCm = SuspensionTravelCm;
        State.Snapshot.SpringStrength = SpringStrength;
        State.Snapshot.DampingStrength = DampingStrength;
        State.Snapshot.HitchBreakForce = HitchBreakForce;
        State.Snapshot.HitchBreakTorque = HitchBreakTorque;
    }

    if (State.EvidenceSeconds >= EvidenceIntervalSeconds)
    {
        State.EvidenceSeconds = 0.0f;
        const FGTTFarmTrailerDynamicsSnapshot& Snapshot = State.Snapshot;
        UE_LOG(LogGTT, Log,
            TEXT("TRAILER_NATIVE_DYNAMICS attached=%s cargo=%s suspension=%s/%s travel_cm=%.1f spring=%.0f damping=%.0f hitch_break_force=%.0f hitch_break_torque=%.0f integrity=%.2f hitch_load=%.2f"),
            Trailer->IsAttached() ? TEXT("YES") : TEXT("NO"),
            Trailer->HasCargo() ? TEXT("LOADED") : TEXT("EMPTY"),
            Snapshot.bLeftSuspensionActive ? TEXT("ACTIVE") : TEXT("OFF"),
            Snapshot.bRightSuspensionActive ? TEXT("ACTIVE") : TEXT("OFF"),
            Snapshot.SuspensionTravelCm, Snapshot.SpringStrength, Snapshot.DampingStrength,
            Snapshot.HitchBreakForce, Snapshot.HitchBreakTorque,
            Trailer->GetTrailerIntegrity(), Trailer->GetHitchLoad());
    }
}

void UGTTFarmTrailerDynamicsSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld() || DeltaSeconds <= 0.0f) return;

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        EvaluateTrailer(*It, DeltaSeconds);
    }

    for (auto It = RuntimeByTrailer.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid()) It.RemoveCurrent();
    }
}
