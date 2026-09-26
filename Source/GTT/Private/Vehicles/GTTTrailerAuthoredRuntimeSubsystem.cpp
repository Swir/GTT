#include "Vehicles/GTTTrailerAuthoredRuntimeSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
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

    bool IsLegacyPresentationComponent(const UStaticMeshComponent* Component)
    {
        if (!Component) return false;
        static const TSet<FName> LegacyNames = {
            TEXT("TrailerBody"), TEXT("LeftWheel"), TEXT("RightWheel"), TEXT("CargoBlock"),
            TEXT("Drawbar"), TEXT("HitchCoupler"), TEXT("FrontRail"), TEXT("LeftRail"),
            TEXT("RightRail"), TEXT("Tailgate"), TEXT("LeftFender"), TEXT("RightFender"),
            TEXT("RearReflectorBar"), TEXT("CargoLogA"), TEXT("CargoLogB"), TEXT("CargoLogC"), TEXT("CargoLogD")
        };
        return LegacyNames.Contains(Component->GetFName());
    }
}

TStatId UGTTTrailerAuthoredRuntimeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTTrailerAuthoredRuntimeSubsystem, STATGROUP_Tickables);
}

void UGTTTrailerAuthoredRuntimeSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    ScanAccumulator += DeltaSeconds;
    if (ScanAccumulator < ScanIntervalSeconds) return;
    const float Step = ScanAccumulator;
    ScanAccumulator = 0.0f;

    for (auto It = RuntimeByTrailer.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid()) It.RemoveCurrent();
        else It.Value().EvidenceCooldown = FMath::Max(0.0f, It.Value().EvidenceCooldown - Step);
    }

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        EvaluateTrailer(*It, Step);
    }
}

USkeletalMeshComponent* UGTTTrailerAuthoredRuntimeSubsystem::FindAuthoredRig(AGTTFarmTrailer* Trailer) const
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

bool UGTTTrailerAuthoredRuntimeSubsystem::ValidateRig(USkeletalMeshComponent* Rig) const
{
    if (!Rig || !Rig->GetPhysicsAsset()) return false;
    const bool bBones = Rig->GetBoneIndex(RequiredRootBone) != INDEX_NONE &&
        Rig->GetBoneIndex(RequiredLeftWheelBone) != INDEX_NONE &&
        Rig->GetBoneIndex(RequiredRightWheelBone) != INDEX_NONE;
    const bool bSockets = Rig->DoesSocketExist(RequiredTowEyeSocket) && Rig->DoesSocketExist(RequiredAxleSocket);
    return bBones && bSockets;
}

void UGTTTrailerAuthoredRuntimeSubsystem::SetAuthoredPresentation(AGTTFarmTrailer* Trailer, USkeletalMeshComponent* Rig, bool bActive, FRuntimeState& State) const
{
    if (!Trailer || State.bPresentationTakeover == bActive) return;

    TArray<UStaticMeshComponent*> StaticMeshes;
    Trailer->GetComponents<UStaticMeshComponent>(StaticMeshes);
    for (UStaticMeshComponent* Mesh : StaticMeshes)
    {
        if (!IsLegacyPresentationComponent(Mesh)) continue;
        if (bActive)
        {
            Mesh->SetVisibility(false, true);
        }
        else
        {
            const FName Name = Mesh->GetFName();
            if (Name == TEXT("CargoBlock")) Mesh->SetVisibility(false, true);
            else if (Name == TEXT("CargoLogA") || Name == TEXT("CargoLogB") || Name == TEXT("CargoLogC") || Name == TEXT("CargoLogD"))
                Mesh->SetVisibility(Trailer->HasCargo(), true);
            else Mesh->SetVisibility(true, true);
        }
    }

    if (Rig)
    {
        Rig->SetVisibility(bActive, true);
        Rig->SetHiddenInGame(!bActive, true);
    }
    State.bPresentationTakeover = bActive;
}

bool UGTTTrailerAuthoredRuntimeSubsystem::TraceWheelContact(AGTTFarmTrailer* Trailer, const FVector& WheelWorld, float& OutGroundClearanceCm) const
{
    OutGroundClearanceCm = GroundTraceDistanceCm;
    UWorld* World = GetWorld();
    if (!World || !Trailer) return false;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTAuthoredTrailerWheelContact), false, Trailer);
    const FVector Start = WheelWorld + FVector::UpVector * 16.0f;
    const FVector End = WheelWorld - FVector::UpVector * GroundTraceDistanceCm;
    if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) return false;

    OutGroundClearanceCm = FVector::Distance(WheelWorld, Hit.ImpactPoint);
    return OutGroundClearanceCm <= GroundContactSlackCm;
}

void UGTTTrailerAuthoredRuntimeSubsystem::ApplyAuthoredDynamics(AGTTFarmTrailer* Trailer, USkeletalMeshComponent* Rig, FRuntimeState& State, float DeltaSeconds) const
{
    if (!Trailer || !Rig || !Trailer->IsAttachedToNativeFieldmaster()) return;

    UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Trailer->GetRootComponent());
    if (!Body || !Body->IsSimulatingPhysics()) return;

    const FVector LeftWheelWorld = Rig->GetSocketTransform(RequiredLeftWheelBone, RTS_World).GetLocation();
    const FVector RightWheelWorld = Rig->GetSocketTransform(RequiredRightWheelBone, RTS_World).GetLocation();
    State.Snapshot.bLeftWheelContact = TraceWheelContact(Trailer, LeftWheelWorld, State.Snapshot.LeftGroundClearanceCm);
    State.Snapshot.bRightWheelContact = TraceWheelContact(Trailer, RightWheelWorld, State.Snapshot.RightGroundClearanceCm);
    State.Snapshot.ContactRatio = 0.5f * (static_cast<float>(State.Snapshot.bLeftWheelContact) + static_cast<float>(State.Snapshot.bRightWheelContact));

    const float LateralSpan = FMath::Max(1.0f, FVector::Distance(FVector(LeftWheelWorld.X, LeftWheelWorld.Y, 0.0f), FVector(RightWheelWorld.X, RightWheelWorld.Y, 0.0f)));
    State.Snapshot.AxleTiltDeg = FMath::RadiansToDegrees(FMath::Atan2(RightWheelWorld.Z - LeftWheelWorld.Z, LateralSpan));

    const FVector AxleWorld = Rig->GetSocketTransform(RequiredAxleSocket, RTS_World).GetLocation();
    const FVector VelocityAtAxle = Body->GetPhysicsLinearVelocityAtPoint(AxleWorld);
    const FVector Right = Trailer->GetActorRightVector().GetSafeNormal();
    const float LateralSpeed = FVector::DotProduct(VelocityAtAxle, Right);
    const float Load = Trailer->GetTowLoadFactor();
    const float ContactAuthority = FMath::Lerp(0.25f, 1.0f, State.Snapshot.ContactRatio);
    const float MassKg = FMath::Max(Body->GetMass(), 1.0f);
    const float Damping = LateralDampingPerSecond * (1.0f + Load * 0.75f) * ContactAuthority;
    const FVector LateralForce = -Right * LateralSpeed * MassKg * Damping;
    Body->AddForceAtLocation(LateralForce, AxleWorld, NAME_None);

    const FVector AngularVelocity = Body->GetPhysicsAngularVelocityInRadians();
    const float ArticulationBoost = 1.0f + FMath::Clamp(State.Snapshot.ArticulationYawDeg / 55.0f, 0.0f, 1.0f) * 0.75f;
    const float YawTorque = -AngularVelocity.Z * YawDampingStrength * MassKg * ContactAuthority * ArticulationBoost;
    Body->AddTorqueInRadians(FVector(0.0f, 0.0f, YawTorque), NAME_None, false);

    const float TiltCorrection = FMath::Clamp(State.Snapshot.AxleTiltDeg / 18.0f, -1.0f, 1.0f);
    Body->AddTorqueInRadians(Trailer->GetActorForwardVector() * (-TiltCorrection * YawDampingStrength * 0.35f * MassKg * ContactAuthority), NAME_None, false);

    State.Snapshot.StabilizationLoad = FMath::Clamp(FMath::Abs(LateralSpeed) / 900.0f + FMath::Abs(AngularVelocity.Z) * 0.22f + Load * 0.35f, 0.0f, 1.0f);
    (void)DeltaSeconds;
}

void UGTTTrailerAuthoredRuntimeSubsystem::EvaluateTrailer(AGTTFarmTrailer* Trailer, float DeltaSeconds)
{
    if (!Trailer) return;
    FRuntimeState& State = RuntimeByTrailer.FindOrAdd(Trailer);
    USkeletalMeshComponent* Rig = FindAuthoredRig(Trailer);
    const bool bRigValid = ValidateRig(Rig);
    State.Rig = Rig;
    State.Snapshot = FGTTAuthoredTrailerRuntimeSnapshot{};
    State.Snapshot.bAuthoredRigValid = bRigValid;

    SetAuthoredPresentation(Trailer, Rig, bRigValid, State);
    State.Snapshot.bAuthoredPresentationActive = State.bPresentationTakeover;

    if (!bRigValid)
    {
        if (State.EvidenceCooldown <= 0.0f)
        {
            GTT_LOG( Verbose, TEXT("AUTHORED_TRAILER_RUNTIME_EVIDENCE trailer=%s active=0 reason=NO_VALID_AUTHORED_RIG"), *GetNameSafe(Trailer));
            State.EvidenceCooldown = EvidenceIntervalSeconds;
        }
        return;
    }

    if (Trailer->IsAttachedToNativeFieldmaster())
    {
        AGTTFieldmasterNativePawn* NativeTow = Cast<AGTTFieldmasterNativePawn>(Trailer->GetTowActor());
        if (NativeTow)
        {
            FTransform RearHitch;
            if (NativeTow->TryGetRearHitchTransform(RearHitch))
            {
                const FVector TowEye = Rig->GetSocketTransform(RequiredTowEyeSocket, RTS_World).GetLocation();
                State.Snapshot.HitchAlignmentErrorCm = FVector::Distance(RearHitch.GetLocation(), TowEye);
                const float Dot = FMath::Clamp(FVector::DotProduct(NativeTow->GetActorForwardVector().GetSafeNormal2D(), Trailer->GetActorForwardVector().GetSafeNormal2D()), -1.0f, 1.0f);
                State.Snapshot.ArticulationYawDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
            }
        }
        ApplyAuthoredDynamics(Trailer, Rig, State, DeltaSeconds);
    }
    else
    {
        const FVector LeftWheelWorld = Rig->GetSocketTransform(RequiredLeftWheelBone, RTS_World).GetLocation();
        const FVector RightWheelWorld = Rig->GetSocketTransform(RequiredRightWheelBone, RTS_World).GetLocation();
        State.Snapshot.bLeftWheelContact = TraceWheelContact(Trailer, LeftWheelWorld, State.Snapshot.LeftGroundClearanceCm);
        State.Snapshot.bRightWheelContact = TraceWheelContact(Trailer, RightWheelWorld, State.Snapshot.RightGroundClearanceCm);
        State.Snapshot.ContactRatio = 0.5f * (static_cast<float>(State.Snapshot.bLeftWheelContact) + static_cast<float>(State.Snapshot.bRightWheelContact));
    }

    if (State.EvidenceCooldown <= 0.0f)
    {
        GTT_LOG( Log, TEXT("AUTHORED_TRAILER_RUNTIME_EVIDENCE trailer=%s active=%d nativeTow=%d contacts=%.1f left=%d right=%d clearL=%.1f clearR=%.1f axleTilt=%.1f hitchError=%.1f articulation=%.1f stabilization=%.2f warning=%d"),
            *GetNameSafe(Trailer), State.bPresentationTakeover ? 1 : 0, Trailer->IsAttachedToNativeFieldmaster() ? 1 : 0,
            State.Snapshot.ContactRatio, State.Snapshot.bLeftWheelContact ? 1 : 0, State.Snapshot.bRightWheelContact ? 1 : 0,
            State.Snapshot.LeftGroundClearanceCm, State.Snapshot.RightGroundClearanceCm, State.Snapshot.AxleTiltDeg,
            State.Snapshot.HitchAlignmentErrorCm, State.Snapshot.ArticulationYawDeg, State.Snapshot.StabilizationLoad,
            State.Snapshot.HitchAlignmentErrorCm > HitchWarningErrorCm ? 1 : 0);
        State.EvidenceCooldown = EvidenceIntervalSeconds;
    }
}

bool UGTTTrailerAuthoredRuntimeSubsystem::IsAuthoredRuntimeActive(const AGTTFarmTrailer* Trailer) const
{
    if (!Trailer) return false;
    for (const TPair<TWeakObjectPtr<AGTTFarmTrailer>, FRuntimeState>& Pair : RuntimeByTrailer)
    {
        if (Pair.Key.Get() == Trailer) return Pair.Value.bPresentationTakeover && Pair.Value.Snapshot.bAuthoredRigValid;
    }
    return false;
}

FGTTAuthoredTrailerRuntimeSnapshot UGTTTrailerAuthoredRuntimeSubsystem::GetRuntimeSnapshot(const AGTTFarmTrailer* Trailer) const
{
    if (!Trailer) return FGTTAuthoredTrailerRuntimeSnapshot{};
    for (const TPair<TWeakObjectPtr<AGTTFarmTrailer>, FRuntimeState>& Pair : RuntimeByTrailer)
    {
        if (Pair.Key.Get() == Trailer) return Pair.Value.Snapshot;
    }
    return FGTTAuthoredTrailerRuntimeSnapshot{};
}