#include "Vehicles/GTTAuthoredTrailerPresentationSubsystem.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/PrimitiveComponent.h"
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
    constexpr float RuntimeEvidenceIntervalSeconds = 0.50f;
    constexpr float WheelRadiusCm = 64.0f;
    constexpr float WheelGroundProbeExtraDepthCm = 80.0f;
    constexpr float WheelContactToleranceCm = 12.0f;

    // Keep these mirrored from UGTTTrailerRoadFeedbackSubsystem and verify them
    // deterministically in Scripts/verify_v0_1_60_authored_trailer_runtime_bridge.py.
    constexpr float StabilityStartSpeedKmh = 25.0f;
    constexpr float StabilityFullSpeedKmh = 70.0f;
    constexpr float MaximumStabilityAuthority = 0.45f;
    constexpr float JackknifeWarningAngleDegrees = 32.0f;
    constexpr float JackknifeCriticalAngleDegrees = 62.0f;
    constexpr float JackknifeStartSpeedKmh = 28.0f;
    constexpr float JackknifeFullSpeedKmh = 58.0f;
    constexpr float JackknifeWarningRiskThreshold = 0.35f;
    constexpr float MaximumJackknifeAssistMultiplier = 1.35f;

    constexpr float ScenarioMinimumSpeedKmh = 4.0f;
    constexpr float ScenarioMinimumDistanceCm = 900.0f;
    constexpr int32 ScenarioMinimumSafeSamples = 8;
    constexpr float ScenarioSafeHitchErrorCm = 80.0f;
    constexpr float ScenarioHardHitchErrorCm = 110.0f;
    constexpr float ScenarioStoppedSpeedKmh = 1.5f;
    constexpr float ScenarioMaximumStepDistanceCm = 1500.0f;

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
    const FName HitchCouplerComponentName(TEXT("HitchCoupler"));

    struct FWheelGroundProbe
    {
        bool bContact = false;
        float ClearanceCm = WheelGroundProbeExtraDepthCm;
    };

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

    FWheelGroundProbe ProbeWheelGround(
        const UWorld* World,
        const AGTTFarmTrailer* Trailer,
        const UStaticMeshComponent* Wheel)
    {
        FWheelGroundProbe Probe;
        if (!World || !Trailer || !Wheel)
        {
            return Probe;
        }

        const FVector WheelLocation = Wheel->GetComponentLocation();
        const FVector TraceStart = WheelLocation + FVector(0.0f, 0.0f, 10.0f);
        const FVector TraceEnd = WheelLocation - FVector(0.0f, 0.0f, WheelRadiusCm + WheelGroundProbeExtraDepthCm);

        FCollisionQueryParams QueryParams;
        QueryParams.bTraceComplex = false;
        QueryParams.AddIgnoredActor(Trailer);
        if (const AActor* TowActor = Trailer->GetTowActor())
        {
            QueryParams.AddIgnoredActor(TowActor);
        }

        FHitResult Hit;
        if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
        {
            return Probe;
        }

        Probe.ClearanceCm = FVector::Distance(WheelLocation, Hit.ImpactPoint) - WheelRadiusCm;
        Probe.bContact = Probe.ClearanceCm <= WheelContactToleranceCm;
        return Probe;
    }

    float ComputeArticulationDegrees(const AGTTFarmTrailer* Trailer)
    {
        if (!Trailer || !Trailer->IsAttached())
        {
            return 0.0f;
        }

        const AActor* TowActor = Trailer->GetTowActor();
        if (!TowActor)
        {
            return 0.0f;
        }

        const FVector TowForward = TowActor->GetActorForwardVector().GetSafeNormal2D();
        const FVector TrailerForward = Trailer->GetActorForwardVector().GetSafeNormal2D();
        if (TowForward.IsNearlyZero() || TrailerForward.IsNearlyZero())
        {
            return 0.0f;
        }

        const float Alignment = FMath::Clamp(FVector::DotProduct(TowForward, TrailerForward), -1.0f, 1.0f);
        return FMath::RadiansToDegrees(FMath::Acos(Alignment));
    }

    float ComputeJackknifeRiskForEvidence(const AGTTFarmTrailer* Trailer, const float SpeedKmh, const float ArticulationDegrees)
    {
        if (!Trailer || !Trailer->IsAttached() || !Trailer->HasCargo() || Trailer->GetLostWheelCount() > 0)
        {
            return 0.0f;
        }

        const float AngleRisk = FMath::Clamp(
            (ArticulationDegrees - JackknifeWarningAngleDegrees) /
                (JackknifeCriticalAngleDegrees - JackknifeWarningAngleDegrees),
            0.0f,
            1.0f);
        const float SpeedRisk = FMath::Clamp(
            (SpeedKmh - JackknifeStartSpeedKmh) /
                (JackknifeFullSpeedKmh - JackknifeStartSpeedKmh),
            0.0f,
            1.0f);
        const float LoadRisk = FMath::Clamp(Trailer->GetTowLoadFactor(), 0.35f, 1.0f);
        return FMath::Clamp(AngleRisk * SpeedRisk * LoadRisk, 0.0f, 1.0f);
    }

    float ComputeStabilityAuthorityForEvidence(const AGTTFarmTrailer* Trailer, const float SpeedKmh, const float JackknifeRisk)
    {
        if (!Trailer || !Trailer->IsAttached() || !Trailer->HasCargo() || Trailer->GetLostWheelCount() > 0)
        {
            return 0.0f;
        }

        const float SpeedAuthority = FMath::Clamp(
            (SpeedKmh - StabilityStartSpeedKmh) / (StabilityFullSpeedKmh - StabilityStartSpeedKmh),
            0.0f,
            1.0f);
        const float LoadAuthority = FMath::Clamp(Trailer->GetTowLoadFactor(), 0.0f, 1.0f);
        const float HitchReserve = 1.0f - FMath::Clamp(Trailer->GetHitchLoad(), 0.0f, 1.0f);
        const float Integrity = FMath::Min(Trailer->GetTrailerIntegrity(), HitchReserve);
        const float IntegrityAuthority = FMath::Clamp((Integrity - 0.20f) / 0.80f, 0.15f, 1.0f);
        const float JackknifeBoost = FMath::Lerp(
            1.0f,
            MaximumJackknifeAssistMultiplier,
            FMath::Clamp(JackknifeRisk, 0.0f, 1.0f));
        return FMath::Min(
            SpeedAuthority * LoadAuthority * IntegrityAuthority * MaximumStabilityAuthority * JackknifeBoost,
            MaximumStabilityAuthority);
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
        EmitRuntimeEvidence(Runtime, DeltaTime);
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
            GTT_LOG( Warning, TEXT("AUTHORED_TRAILER_PRESENTATION event=REJECTED actor=%s reason=%s"), *GetNameSafe(Trailer), *ValidationFailure);
        }
        return false;
    }

    UStaticMeshComponent* LeftWheel = FindStaticMeshComponent(Trailer, LeftWheelComponentName);
    UStaticMeshComponent* RightWheel = FindStaticMeshComponent(Trailer, RightWheelComponentName);
    UStaticMeshComponent* HitchCoupler = FindStaticMeshComponent(Trailer, HitchCouplerComponentName);
    if (!LeftWheel || !RightWheel || !HitchCoupler) return false;

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
    Runtime.HitchCoupler = HitchCoupler;
    const FTransform RootTransform = Trailer->GetRootComponent()->GetComponentTransform();
    Runtime.LeftPhysicalReference = LeftWheel->GetComponentTransform().GetRelativeTransform(RootTransform);
    Runtime.RightPhysicalReference = RightWheel->GetComponentTransform().GetRelativeTransform(RootTransform);
    Runtime.LeftBoneReference = AuthoredVisual->GetBoneTransformByName(LeftWheelBoneName, EBoneSpaces::ComponentSpace);
    Runtime.RightBoneReference = AuthoredVisual->GetBoneTransformByName(RightWheelBoneName, EBoneSpaces::ComponentSpace);

    HidePlaceholderPresentation(Trailer);
    RuntimeVisuals.Add(Runtime);
    UpdateWheelPose(RuntimeVisuals.Last());

    GTT_LOG( Display, TEXT("AUTHORED_TRAILER_PRESENTATION event=ACTIVATED actor=%s asset=%s bones=body,wheel_l,wheel_r sockets=socket_hitch,socket_cargo,socket_axle_l,socket_axle_r physics_asset=YES"), *GetNameSafe(Trailer), AuthoredTrailerAssetPath);
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

void UGTTAuthoredTrailerPresentationSubsystem::EmitRuntimeEvidence(FRuntimeTrailerVisual& Runtime, float DeltaTime)
{
    Runtime.EvidenceAccumulatorSeconds += FMath::Max(DeltaTime, 0.0f);
    if (Runtime.EvidenceAccumulatorSeconds < RuntimeEvidenceIntervalSeconds)
    {
        return;
    }
    Runtime.EvidenceAccumulatorSeconds = 0.0f;

    UWorld* World = GetWorld();
    AGTTFarmTrailer* Trailer = Runtime.Trailer.Get();
    UPoseableMeshComponent* AuthoredVisual = Runtime.AuthoredVisual.Get();
    UStaticMeshComponent* LeftWheel = Runtime.LeftWheel.Get();
    UStaticMeshComponent* RightWheel = Runtime.RightWheel.Get();
    UStaticMeshComponent* HitchCoupler = Runtime.HitchCoupler.Get();
    if (!World || !Trailer || !AuthoredVisual || !LeftWheel || !RightWheel || !HitchCoupler)
    {
        return;
    }

    const FWheelGroundProbe LeftProbe = ProbeWheelGround(World, Trailer, LeftWheel);
    const FWheelGroundProbe RightProbe = ProbeWheelGround(World, Trailer, RightWheel);
    const float ContactFraction = 0.5f * static_cast<float>((LeftProbe.bContact ? 1 : 0) + (RightProbe.bContact ? 1 : 0));

    const FVector LeftLocation = LeftWheel->GetComponentLocation();
    const FVector RightLocation = RightWheel->GetComponentLocation();
    const float TrackDistanceCm = FMath::Max(FVector::Dist2D(LeftLocation, RightLocation), 1.0f);
    const float AxleTiltDegrees = FMath::RadiansToDegrees(FMath::Atan2(
        FMath::Abs(LeftLocation.Z - RightLocation.Z),
        TrackDistanceCm));

    const FVector AuthoredHitchLocation = AuthoredVisual->GetSocketTransform(HitchSocketName, RTS_World).GetLocation();
    const float HitchErrorCm = FVector::Distance(AuthoredHitchLocation, HitchCoupler->GetComponentLocation());

    const float SpeedKmh = Trailer->GetVelocity().Size() * 0.036f;
    const float ArticulationDegrees = ComputeArticulationDegrees(Trailer);
    const float JackknifeRisk = ComputeJackknifeRiskForEvidence(Trailer, SpeedKmh, ArticulationDegrees);
    const float StabilityAuthority = ComputeStabilityAuthorityForEvidence(Trailer, SpeedKmh, JackknifeRisk);
    const bool bWarning = JackknifeRisk >= JackknifeWarningRiskThreshold;
    const bool bNativeTow = Trailer->IsAttachedToNativeFieldmaster();

    UE_LOG(
        LogGTT,
        Display,
        TEXT("AUTHORED_TRAILER_RUNTIME_EVIDENCE trailer=%s active=1 nativeTow=%d contacts=%.3f left=%d right=%d clearL=%.2f clearR=%.2f axleTilt=%.2f hitchError=%.2f articulation=%.2f stabilization=%.3f warning=%d"),
        *GetNameSafe(Trailer),
        bNativeTow ? 1 : 0,
        ContactFraction,
        LeftProbe.bContact ? 1 : 0,
        RightProbe.bContact ? 1 : 0,
        LeftProbe.ClearanceCm,
        RightProbe.ClearanceCm,
        AxleTiltDegrees,
        HitchErrorCm,
        ArticulationDegrees,
        StabilityAuthority,
        bWarning ? 1 : 0);

    const bool bScenarioEligible =
        bNativeTow &&
        Trailer->IsAttached() &&
        Trailer->HasCargo() &&
        Trailer->GetLostWheelCount() == 0;

    if (!bScenarioEligible)
    {
        if (!Runtime.bScenarioCompleteEmitted)
        {
            Runtime.bScenarioTracking = false;
            Runtime.LastScenarioLocation = FVector::ZeroVector;
            Runtime.ScenarioDistanceCm = 0.0f;
            Runtime.ScenarioMaxSpeedKmh = 0.0f;
            Runtime.ScenarioMaxHitchErrorCm = 0.0f;
            Runtime.ScenarioMaxArticulationDeg = 0.0f;
            Runtime.ScenarioMinCargoIntegrity = 1.0f;
            Runtime.ScenarioDualContactSamples = 0;
            Runtime.ScenarioSafeSamples = 0;
        }
        return;
    }

    if (!Runtime.bScenarioTracking)
    {
        Runtime.bScenarioTracking = true;
        Runtime.LastScenarioLocation = Trailer->GetActorLocation();
        Runtime.ScenarioDistanceCm = 0.0f;
        Runtime.ScenarioMaxSpeedKmh = 0.0f;
        Runtime.ScenarioMaxHitchErrorCm = 0.0f;
        Runtime.ScenarioMaxArticulationDeg = 0.0f;
        Runtime.ScenarioMinCargoIntegrity = Trailer->GetCargoIntegrity();
        Runtime.ScenarioDualContactSamples = 0;
        Runtime.ScenarioSafeSamples = 0;
    }
    else
    {
        const FVector CurrentLocation = Trailer->GetActorLocation();
        const float StepDistanceCm = FVector::Dist2D(CurrentLocation, Runtime.LastScenarioLocation);
        if (StepDistanceCm <= ScenarioMaximumStepDistanceCm)
        {
            Runtime.ScenarioDistanceCm += StepDistanceCm;
        }
        Runtime.LastScenarioLocation = CurrentLocation;
    }

    Runtime.ScenarioMaxSpeedKmh = FMath::Max(Runtime.ScenarioMaxSpeedKmh, SpeedKmh);
    Runtime.ScenarioMaxHitchErrorCm = FMath::Max(Runtime.ScenarioMaxHitchErrorCm, HitchErrorCm);
    Runtime.ScenarioMaxArticulationDeg = FMath::Max(Runtime.ScenarioMaxArticulationDeg, ArticulationDegrees);
    Runtime.ScenarioMinCargoIntegrity = FMath::Min(Runtime.ScenarioMinCargoIntegrity, Trailer->GetCargoIntegrity());

    const bool bDualContact = LeftProbe.bContact && RightProbe.bContact;
    if (bDualContact)
    {
        ++Runtime.ScenarioDualContactSamples;
        if (!bWarning && HitchErrorCm <= ScenarioSafeHitchErrorCm)
        {
            ++Runtime.ScenarioSafeSamples;
        }
    }

    UE_LOG(
        LogGTT,
        Display,
        TEXT("NATIVE_TRAILER_SCENARIO_SAMPLE speed_kmh=%.2f distance_cm=%.1f loaded=1 attached=1 active=1 contacts=%.3f left=%d right=%d hitch_error_cm=%.2f articulation_deg=%.2f cargo_integrity=%.3f trailer_integrity=%.3f hitch_load=%.3f"),
        SpeedKmh,
        Runtime.ScenarioDistanceCm,
        ContactFraction,
        LeftProbe.bContact ? 1 : 0,
        RightProbe.bContact ? 1 : 0,
        HitchErrorCm,
        ArticulationDegrees,
        Trailer->GetCargoIntegrity(),
        Trailer->GetTrailerIntegrity(),
        Trailer->GetHitchLoad());

    if (HitchErrorCm > ScenarioHardHitchErrorCm)
    {
        UE_LOG(
            LogGTT,
            Warning,
            TEXT("NATIVE_TRAILER_SCENARIO phase=DIAGNOSTIC result=FAIL reason=HITCH_ENVELOPE hitch_error_cm=%.2f hard_limit_cm=%.2f"),
            HitchErrorCm,
            ScenarioHardHitchErrorCm);
    }

    const bool bHasMotionProof =
        Runtime.ScenarioMaxSpeedKmh >= ScenarioMinimumSpeedKmh &&
        Runtime.ScenarioDistanceCm >= ScenarioMinimumDistanceCm &&
        Runtime.ScenarioDualContactSamples >= ScenarioMinimumSafeSamples &&
        Runtime.ScenarioSafeSamples >= ScenarioMinimumSafeSamples &&
        Runtime.ScenarioMaxHitchErrorCm <= ScenarioHardHitchErrorCm;
    const bool bControlledStop = SpeedKmh <= ScenarioStoppedSpeedKmh;

    if (bHasMotionProof && bControlledStop && !Runtime.bScenarioCompleteEmitted)
    {
        Runtime.bScenarioCompleteEmitted = true;
        UE_LOG(
            LogGTT,
            Display,
            TEXT("NATIVE_TRAILER_SCENARIO_COMPLETE result=PASS route=loaded-authored-tow attachment=1 authored=1 loaded=1 stopped=1 max_speed_kmh=%.2f distance_cm=%.1f dual_contact_samples=%d safe_samples=%d max_hitch_error_cm=%.2f max_articulation_deg=%.2f min_cargo_integrity=%.3f final_speed_kmh=%.2f"),
            Runtime.ScenarioMaxSpeedKmh,
            Runtime.ScenarioDistanceCm,
            Runtime.ScenarioDualContactSamples,
            Runtime.ScenarioSafeSamples,
            Runtime.ScenarioMaxHitchErrorCm,
            Runtime.ScenarioMaxArticulationDeg,
            Runtime.ScenarioMinCargoIntegrity,
            SpeedKmh);
    }
}
