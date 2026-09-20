#include "Vehicles/GTTFarmTrailer.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTChaosVehicleBridgeComponent.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace
{
    constexpr float TrailerImpactCooldownSeconds = 0.35f;
    constexpr float TrailerDamageThresholdKmh = 18.0f;
    constexpr float TrailerSevereImpactKmh = 42.0f;
    constexpr float TrailerSuspensionTravelCm = 24.0f;
    constexpr float TrailerSuspensionSpring = 56000.0f;
    constexpr float TrailerLoadedSuspensionSpring = 76000.0f;
    constexpr float TrailerSuspensionDamping = 7600.0f;
    constexpr float TrailerLoadedSuspensionDamping = 9800.0f;
    constexpr int32 TrailerFieldRepairBaseQuote = 35;
    constexpr int32 TrailerFieldRepairWheelQuote = 45;
    constexpr int32 TrailerFieldRepairCargoSurcharge = 20;
    constexpr float TrailerFieldRepairBaseDuration = 6.0f;
    const FVector LeftWheelHome(70.0f, -145.0f, -62.0f);
    const FVector RightWheelHome(70.0f, 145.0f, -62.0f);

    void ConfigureVisual(UStaticMeshComponent* Component, UStaticMesh* Mesh, USceneComponent* Parent, const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
    {
        if (!Component) return;
        Component->SetupAttachment(Parent);
        Component->SetStaticMesh(Mesh);
        Component->SetRelativeLocation(Location);
        Component->SetRelativeScale3D(Scale);
        Component->SetRelativeRotation(Rotation);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCastShadow(true);
    }
}

AGTTFarmTrailer::AGTTFarmTrailer()
{
    PrimaryActorTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Cube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    UStaticMesh* Cylinder = CylinderFinder.Succeeded() ? CylinderFinder.Object : Cube;

    TrailerBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrailerBody"));
    SetRootComponent(TrailerBody);
    TrailerBody->SetStaticMesh(Cube);
    TrailerBody->SetRelativeScale3D(FVector(2.9f, 1.25f, 0.20f));
    TrailerBody->SetSimulatePhysics(true);
    TrailerBody->SetNotifyRigidBodyCollision(true);
    TrailerBody->SetMassOverrideInKg(NAME_None, 980.0f, true);
    TrailerBody->SetLinearDamping(0.45f);
    TrailerBody->SetAngularDamping(1.4f);

    LeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWheel"));
    LeftWheel->SetupAttachment(TrailerBody);
    LeftWheel->SetStaticMesh(Cylinder);
    LeftWheel->SetRelativeLocation(LeftWheelHome);
    LeftWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftWheel->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.34f));
    LeftWheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    LeftWheel->SetSimulatePhysics(true);
    LeftWheel->SetMassOverrideInKg(NAME_None, 74.0f, true);
    LeftWheel->SetLinearDamping(0.18f);
    LeftWheel->SetAngularDamping(0.10f);

    RightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWheel"));
    RightWheel->SetupAttachment(TrailerBody);
    RightWheel->SetStaticMesh(Cylinder);
    RightWheel->SetRelativeLocation(RightWheelHome);
    RightWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightWheel->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.34f));
    RightWheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RightWheel->SetSimulatePhysics(true);
    RightWheel->SetMassOverrideInKg(NAME_None, 74.0f, true);
    RightWheel->SetLinearDamping(0.18f);
    RightWheel->SetAngularDamping(0.10f);

    LeftWheelConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LeftWheelConstraint"));
    LeftWheelConstraint->SetupAttachment(TrailerBody);
    LeftWheelConstraint->SetRelativeLocation(LeftWheelHome);

    RightWheelConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("RightWheelConstraint"));
    RightWheelConstraint->SetupAttachment(TrailerBody);
    RightWheelConstraint->SetRelativeLocation(RightWheelHome);

    // Keep the old block as a hidden state/mass carrier so older logic remains compatible,
    // but replace its visible presentation with a farm-trailer silhouette and timber stack.
    CargoBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoBlock"));
    ConfigureVisual(CargoBlock, Cube, TrailerBody, FVector(20.0f, 0.0f, 70.0f), FVector(2.25f, 0.95f, 0.20f));
    CargoBlock->SetVisibility(false, true);

    Drawbar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Drawbar"));
    ConfigureVisual(Drawbar, Cube, TrailerBody, FVector(-355.0f, 0.0f, -2.0f), FVector(1.35f, 0.18f, 0.12f));

    HitchCoupler = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HitchCoupler"));
    ConfigureVisual(HitchCoupler, Cylinder, TrailerBody, FVector(-495.0f, 0.0f, -2.0f), FVector(0.16f, 0.16f, 0.12f), FRotator(0.0f, 90.0f, 0.0f));

    FrontRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRail"));
    ConfigureVisual(FrontRail, Cube, TrailerBody, FVector(-230.0f, 0.0f, 72.0f), FVector(0.12f, 1.18f, 0.72f));

    LeftRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRail"));
    ConfigureVisual(LeftRail, Cube, TrailerBody, FVector(15.0f, -118.0f, 58.0f), FVector(2.45f, 0.08f, 0.42f));

    RightRail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRail"));
    ConfigureVisual(RightRail, Cube, TrailerBody, FVector(15.0f, 118.0f, 58.0f), FVector(2.45f, 0.08f, 0.42f));

    Tailgate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tailgate"));
    ConfigureVisual(Tailgate, Cube, TrailerBody, FVector(265.0f, 0.0f, 58.0f), FVector(0.10f, 1.15f, 0.42f));

    LeftFender = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFender"));
    ConfigureVisual(LeftFender, Cube, TrailerBody, FVector(70.0f, -143.0f, -8.0f), FVector(0.72f, 0.12f, 0.10f));

    RightFender = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFender"));
    ConfigureVisual(RightFender, Cube, TrailerBody, FVector(70.0f, 143.0f, -8.0f), FVector(0.72f, 0.12f, 0.10f));

    RearReflectorBar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearReflectorBar"));
    ConfigureVisual(RearReflectorBar, Cube, TrailerBody, FVector(280.0f, 0.0f, 12.0f), FVector(0.08f, 1.00f, 0.08f));

    CargoLogA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoLogA"));
    ConfigureVisual(CargoLogA, Cylinder, TrailerBody, FVector(25.0f, -62.0f, 70.0f), FVector(0.30f, 0.30f, 2.20f), FRotator(0.0f, 90.0f, 0.0f));
    CargoLogB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoLogB"));
    ConfigureVisual(CargoLogB, Cylinder, TrailerBody, FVector(25.0f, 0.0f, 72.0f), FVector(0.32f, 0.32f, 2.18f), FRotator(0.0f, 90.0f, 0.0f));
    CargoLogC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoLogC"));
    ConfigureVisual(CargoLogC, Cylinder, TrailerBody, FVector(25.0f, 62.0f, 70.0f), FVector(0.29f, 0.29f, 2.22f), FRotator(0.0f, 90.0f, 0.0f));
    CargoLogD = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoLogD"));
    ConfigureVisual(CargoLogD, Cylinder, TrailerBody, FVector(10.0f, -4.0f, 122.0f), FVector(0.27f, 0.27f, 2.05f), FRotator(0.0f, 90.0f, 0.0f));
    SetCargoVisualsVisible(false);

    HitchConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("HitchConstraint"));
    HitchConstraint->SetupAttachment(TrailerBody);
    ConfigureHitchConstraint();
}

void AGTTFarmTrailer::BeginPlay()
{
    Super::BeginPlay();
    ConfigureWheelAxle(LeftWheelConstraint, LeftWheel);
    ConfigureWheelAxle(RightWheelConstraint, RightWheel);
    ConfigureHitchConstraint();
    RefreshPresentation();
}

void AGTTFarmTrailer::ConfigureWheelAxle(UPhysicsConstraintComponent* Constraint, UStaticMeshComponent* Wheel)
{
    if (!Constraint || !Wheel || !TrailerBody) return;
    Constraint->SetDisableCollision(true);
    Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, TrailerSuspensionTravelCm);
    Constraint->SetLinearPositionDrive(false, false, true);
    Constraint->SetLinearVelocityDrive(false, false, true);
    Constraint->SetLinearPositionTarget(FVector::ZeroVector);
    Constraint->SetLinearVelocityTarget(FVector::ZeroVector);
    Constraint->SetLinearDriveParams(
        bCargoLoaded ? TrailerLoadedSuspensionSpring : TrailerSuspensionSpring,
        bCargoLoaded ? TrailerLoadedSuspensionDamping : TrailerSuspensionDamping,
        0.0f);
    Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.0f);
    Constraint->SetLinearBreakable(true, WheelBreakForce);
    Constraint->SetAngularBreakable(true, WheelBreakTorque);
    Constraint->SetConstrainedComponents(TrailerBody, NAME_None, Wheel, NAME_None);
}

void AGTTFarmTrailer::ConfigureHitchConstraint()
{
    if (!HitchConstraint) return;
    HitchConstraint->SetDisableCollision(true);
    HitchConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, 90.0f);
    HitchConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, 70.0f);
    HitchConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, 65.0f);
    HitchConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 42.0f);
    HitchConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 24.0f);
    HitchConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 18.0f);
}

void AGTTFarmTrailer::RestoreWheel(UStaticMeshComponent* Wheel, UPhysicsConstraintComponent* Constraint, const FVector& RelativeLocation)
{
    if (!Wheel || !Constraint || !TrailerBody) return;
    Wheel->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Wheel->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    const FTransform BodyTransform = TrailerBody->GetComponentTransform();
    const FVector WorldLocation = BodyTransform.TransformPosition(RelativeLocation);
    const FQuat WorldRotation = BodyTransform.GetRotation() * FRotator(90.0f, 0.0f, 0.0f).Quaternion();
    Wheel->SetWorldLocationAndRotation(WorldLocation, WorldRotation, false, nullptr, ETeleportType::TeleportPhysics);
    Wheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Wheel->SetSimulatePhysics(true);
    ConfigureWheelAxle(Constraint, Wheel);
}

void AGTTFarmTrailer::SetCargoVisualsVisible(bool bVisible)
{
    if (CargoLogA) CargoLogA->SetVisibility(bVisible, true);
    if (CargoLogB) CargoLogB->SetVisibility(bVisible, true);
    if (CargoLogC) CargoLogC->SetVisibility(bVisible, true);
    if (CargoLogD) CargoLogD->SetVisibility(bVisible, true);
}

void AGTTFarmTrailer::RefreshPresentation()
{
    if (LeftFender) LeftFender->SetVisibility(!bLeftWheelLost, true);
    if (RightFender) RightFender->SetVisibility(!bRightWheelLost, true);

    const float Damage01 = 1.0f - FMath::Clamp(TrailerIntegrity, 0.0f, 1.0f);
    if (Tailgate)
    {
        const float TailgateSag = FMath::Lerp(0.0f, -13.0f, FMath::Clamp(Damage01 * 1.35f, 0.0f, 1.0f));
        Tailgate->SetRelativeRotation(FRotator(0.0f, TailgateSag, 0.0f));
    }
    if (RearReflectorBar)
    {
        const float ReflectorSag = FMath::Lerp(0.0f, 8.0f, FMath::Clamp(Damage01 * 1.6f, 0.0f, 1.0f));
        RearReflectorBar->SetRelativeRotation(FRotator(ReflectorSag, 0.0f, 0.0f));
    }

    SetCargoVisualsVisible(bCargoLoaded);
    if (CargoLogD)
    {
        const float CargoShift = bCargoLoaded ? (1.0f - FMath::Clamp(CargoIntegrity, 0.0f, 1.0f)) * 42.0f : 0.0f;
        CargoLogD->SetRelativeLocation(FVector(10.0f + CargoShift, -4.0f, 122.0f - CargoShift * 0.28f));
        CargoLogD->SetRelativeRotation(FRotator(0.0f, 90.0f + CargoShift * 0.18f, CargoShift * 0.10f));
    }
}

AActor* AGTTFarmTrailer::GetTowActor() const
{
    if (NativeTowVehicle) return NativeTowVehicle;
    return TowVehicle;
}

bool AGTTFarmTrailer::HasIntactAxle() const
{
    return !bLeftWheelLost && !bRightWheelLost;
}

bool AGTTFarmTrailer::NeedsRoadsideRepair() const
{
    return TrailerIntegrity < 0.98f || !HasIntactAxle();
}

int32 AGTTFarmTrailer::GetRoadsideRepairQuote() const
{
    const float Damage01 = 1.0f - FMath::Clamp(TrailerIntegrity, 0.0f, 1.0f);
    const int32 DamageQuote = FMath::CeilToInt(Damage01 * 90.0f);
    return TrailerFieldRepairBaseQuote + GetLostWheelCount() * TrailerFieldRepairWheelQuote + DamageQuote + (bCargoLoaded ? TrailerFieldRepairCargoSurcharge : 0);
}

float AGTTFarmTrailer::GetRoadsideRepairDuration() const
{
    const float Damage01 = 1.0f - FMath::Clamp(TrailerIntegrity, 0.0f, 1.0f);
    return FMath::Clamp(TrailerFieldRepairBaseDuration + GetLostWheelCount() * 3.5f + Damage01 * 8.0f + (bCargoLoaded ? 2.0f : 0.0f), 6.0f, 20.0f);
}

bool AGTTFarmTrailer::CanBeginRoadsideRepair(APawn* RepairPawn, FString& OutReason) const
{
    if (!RepairPawn)
    {
        OutReason = TEXT("TRAILER SERVICE: no player operator available.");
        return false;
    }
    if (!NeedsRoadsideRepair())
    {
        OutReason = TEXT("TRAILER CHECK: axle and structure are roadworthy; no field repair is needed.");
        return false;
    }
    if (FVector::DistSquared(RepairPawn->GetActorLocation(), GetActorLocation()) > FMath::Square(RoadsideRepairMaxDistance))
    {
        OutReason = TEXT("TRAILER SERVICE: move closer before starting the field repair.");
        return false;
    }
    const float SpeedKmh = GetVelocity().Size() * 0.036f;
    if (SpeedKmh > RoadsideRepairMaxSpeedKmh)
    {
        OutReason = TEXT("TRAILER SERVICE: stop the trailer before working on the axle or hitch.");
        return false;
    }
    if (bAttached && HitchLoad > RoadsideRepairMaxHitchLoad)
    {
        OutReason = TEXT("TRAILER SERVICE: hitch is under tension. Stop and relieve the tow load first.");
        return false;
    }
    const UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(RepairPawn);
    if (!Economy)
    {
        OutReason = TEXT("TRAILER SERVICE: player economy is unavailable.");
        return false;
    }
    const int32 Quote = GetRoadsideRepairQuote();
    if (Economy->GetCash() < Quote)
    {
        OutReason = FString::Printf(TEXT("TRAILER SERVICE: field repair costs $%d; insufficient cash."), Quote);
        return false;
    }
    return true;
}

void AGTTFarmTrailer::BeginRoadsideRepair(APawn* RepairPawn)
{
    if (!RepairPawn || bRoadsideRepairPending) return;
    FString Reason;
    if (!CanBeginRoadsideRepair(RepairPawn, Reason))
    {
        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(RepairPawn)) Economy->PushMessage(Reason, 5.0f);
        return;
    }

    bRoadsideRepairPending = true;
    RoadsideRepairPlayer = RepairPawn;
    LockedRoadsideRepairQuote = GetRoadsideRepairQuote();
    LockedRoadsideRepairDuration = GetRoadsideRepairDuration();
    RoadsideRepairTimeRemaining = LockedRoadsideRepairDuration;

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(RepairPawn))
    {
        Economy->PushMessage(FString::Printf(TEXT("TRAILER FIELD REPAIR STARTED: $%d locked | %.0fs. Stay close and keep the trailer still."), LockedRoadsideRepairQuote, LockedRoadsideRepairDuration), 6.0f);
    }
    UE_LOG(LogGTT, Display, TEXT("TRAILER_ROADSIDE_RECOVERY event=START quote=%d duration=%.1f wheels_lost=%d cargo=%s integrity=%.3f"), LockedRoadsideRepairQuote, LockedRoadsideRepairDuration, GetLostWheelCount(), bCargoLoaded ? TEXT("YES") : TEXT("NO"), TrailerIntegrity);
}

void AGTTFarmTrailer::CancelRoadsideRepair(const FString& Reason)
{
    APawn* RepairPawn = RoadsideRepairPlayer.Get();
    if (RepairPawn)
    {
        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(RepairPawn)) Economy->PushMessage(Reason, 4.5f);
    }
    UE_LOG(LogGTT, Display, TEXT("TRAILER_ROADSIDE_RECOVERY event=CANCEL quote=%d remaining=%.2f reason=%s"), LockedRoadsideRepairQuote, RoadsideRepairTimeRemaining, *Reason);
    bRoadsideRepairPending = false;
    RoadsideRepairPlayer.Reset();
    LockedRoadsideRepairQuote = 0;
    LockedRoadsideRepairDuration = 0.0f;
    RoadsideRepairTimeRemaining = 0.0f;
}

void AGTTFarmTrailer::CompleteRoadsideRepair()
{
    APawn* RepairPawn = RoadsideRepairPlayer.Get();
    UGTTPlayerEconomyComponent* Economy = RepairPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(RepairPawn) : nullptr;
    if (!RepairPawn || !Economy)
    {
        CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: operator unavailable; no charge."));
        return;
    }
    if (Economy->GetCash() < LockedRoadsideRepairQuote)
    {
        CancelRoadsideRepair(FString::Printf(TEXT("TRAILER SERVICE STOPPED: $%d locked quote is no longer available; no repair or charge."), LockedRoadsideRepairQuote));
        return;
    }

    const int32 CompletedQuote = LockedRoadsideRepairQuote;
    const int32 LostWheelsBefore = GetLostWheelCount();
    const float IntegrityBefore = TrailerIntegrity;
    const float CargoIntegrityBefore = CargoIntegrity;
    if (!PerformRoadsideRepair(0.40f))
    {
        CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: repair target changed before completion; no charge."));
        return;
    }
    if (!Economy->SpendCash(CompletedQuote, TEXT("Trailer roadside field repair")))
    {
        // This should only be reachable if another synchronous economy mutation occurred after the pre-check.
        // The physical repair remains safer than intentionally re-breaking an axle; make the anomaly explicit.
        Economy->PushMessage(TEXT("TRAILER SERVICE WARNING: repair completed but checkout changed unexpectedly. Inspect economy state."), 6.0f);
        UE_LOG(LogGTT, Error, TEXT("TRAILER_ROADSIDE_RECOVERY event=CHECKOUT_FAIL quote=%d integrity_before=%.3f integrity_after=%.3f"), CompletedQuote, IntegrityBefore, TrailerIntegrity);
    }
    else
    {
        Economy->PushMessage(FString::Printf(TEXT("TRAILER FIELD REPAIR COMPLETE: $%d paid | wheels %d->%d | structure %.0f%%->%.0f%% | cargo remains %.0f%%."), CompletedQuote, LostWheelsBefore, GetLostWheelCount(), IntegrityBefore * 100.0f, TrailerIntegrity * 100.0f, CargoIntegrity * 100.0f), 7.0f);
        UE_LOG(LogGTT, Display, TEXT("TRAILER_ROADSIDE_RECOVERY event=COMPLETE quote=%d wheels_before=%d wheels_after=%d integrity_before=%.3f integrity_after=%.3f cargo_before=%.3f cargo_after=%.3f"), CompletedQuote, LostWheelsBefore, GetLostWheelCount(), IntegrityBefore, TrailerIntegrity, CargoIntegrityBefore, CargoIntegrity);
    }

    bRoadsideRepairPending = false;
    RoadsideRepairPlayer.Reset();
    LockedRoadsideRepairQuote = 0;
    LockedRoadsideRepairDuration = 0.0f;
    RoadsideRepairTimeRemaining = 0.0f;
}

void AGTTFarmTrailer::Interact_Implementation(AActor* Interactor)
{
    APawn* RepairPawn = Cast<APawn>(Interactor);
    if (!RepairPawn) return;

    if (bRoadsideRepairPending)
    {
        if (RoadsideRepairPlayer.Get() == RepairPawn)
        {
            CancelRoadsideRepair(TEXT("TRAILER FIELD REPAIR CANCELLED: no charge."));
        }
        else if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(RepairPawn))
        {
            Economy->PushMessage(TEXT("TRAILER SERVICE BUSY: another operator is already repairing this trailer."), 4.0f);
        }
        return;
    }
    BeginRoadsideRepair(RepairPawn);
}

FText AGTTFarmTrailer::GetInteractionText_Implementation() const
{
    if (bRoadsideRepairPending)
    {
        return FText::FromString(FString::Printf(TEXT("Cancel trailer field repair (%.0fs remaining)"), FMath::Max(0.0f, RoadsideRepairTimeRemaining)));
    }
    if (!NeedsRoadsideRepair()) return FText::FromString(TEXT("Inspect trailer — roadworthy"));
    if (bAttached && HitchLoad > RoadsideRepairMaxHitchLoad) return FText::FromString(TEXT("Trailer hitch under tension — stop tow vehicle"));
    return FText::FromString(FString::Printf(TEXT("Field repair trailer — $%d / %.0fs"), GetRoadsideRepairQuote(), GetRoadsideRepairDuration()));
}

void AGTTFarmTrailer::RefreshAxleState()
{
    if (LeftWheelConstraint && LeftWheelConstraint->IsBroken()) bLeftWheelLost = true;
    if (RightWheelConstraint && RightWheelConstraint->IsBroken()) bRightWheelLost = true;

    const int32 LostWheelCount = GetLostWheelCount();
    if (LostWheelCount > 0)
    {
        TrailerIntegrity = FMath::Min(TrailerIntegrity, LostWheelCount == 2 ? 0.25f : 0.55f);
        if (bCargoLoaded && GetWorld())
        {
            CargoIntegrity = FMath::Max(0.0f, CargoIntegrity - 0.045f * LostWheelCount * GetWorld()->GetDeltaSeconds());
        }
    }
}

void AGTTFarmTrailer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshAxleState();

    if (bAttached)
    {
        AActor* ActiveTowActor = GetTowActor();
        if (!ActiveTowActor)
        {
            DetachTrailer();
        }
        else
        {
            const float Distance = FVector::Distance(ActiveTowActor->GetActorLocation(), GetActorLocation());
            HitchLoad = FMath::Clamp((Distance - SafeHitchDistance) / FMath::Max(1.0f, BreakHitchDistance - SafeHitchDistance), 0.0f, 1.0f);
            if (Distance > BreakHitchDistance)
            {
                TrailerIntegrity = FMath::Max(0.0f, TrailerIntegrity - 0.16f);
                DetachTrailer();
            }
        }
    }

    if (bCargoLoaded)
    {
        const float SpeedKmh = GetVelocity().Size() * 0.036f;
        const float Roll = FMath::Abs(GetActorRotation().Roll);
        const float Pitch = FMath::Abs(GetActorRotation().Pitch);
        const float Instability = FMath::Max(Roll / 35.0f, Pitch / 28.0f);
        const float AxlePenalty = HasIntactAxle() ? 0.0f : 0.45f;
        if (Instability + AxlePenalty > 0.65f || SpeedKmh > 68.0f)
        {
            const float Stress = FMath::Max(Instability + AxlePenalty - 0.65f, (SpeedKmh - 68.0f) / 55.0f);
            CargoIntegrity = FMath::Max(0.0f, CargoIntegrity - Stress * 0.055f * DeltaSeconds);
            TrailerIntegrity = FMath::Max(0.0f, TrailerIntegrity - Stress * 0.012f * DeltaSeconds);
        }
    }

    if (bRoadsideRepairPending)
    {
        APawn* RepairPawn = RoadsideRepairPlayer.Get();
        if (!RepairPawn)
        {
            CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: operator unavailable; no charge."));
        }
        else if (FVector::DistSquared(RepairPawn->GetActorLocation(), GetActorLocation()) > FMath::Square(RoadsideRepairMaxDistance))
        {
            CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: you moved too far away; no charge."));
        }
        else if (GetVelocity().Size() * 0.036f > RoadsideRepairMaxSpeedKmh)
        {
            CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: trailer moved during repair; no charge."));
        }
        else if (bAttached && HitchLoad > RoadsideRepairMaxHitchLoad)
        {
            CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: hitch tension became unsafe; no charge."));
        }
        else
        {
            RoadsideRepairTimeRemaining = FMath::Max(0.0f, RoadsideRepairTimeRemaining - DeltaSeconds);
            if (RoadsideRepairTimeRemaining <= KINDA_SMALL_NUMBER) CompleteRoadsideRepair();
        }
    }

    RefreshPresentation();
}

void AGTTFarmTrailer::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
    if (!GetWorld() || Other == this || !TrailerBody) return;

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastImpactDamageTimeSeconds < TrailerImpactCooldownSeconds) return;

    const float SpeedKmh = GetVelocity().Size() * 0.036f;
    const float MassKg = FMath::Max(TrailerBody->GetMass(), 1.0f);
    const float ImpulseEquivalentKmh = (NormalImpulse.Size() / MassKg) * 0.036f;
    const float ImpactKmh = FMath::Max(SpeedKmh, ImpulseEquivalentKmh);
    if (ImpactKmh < TrailerDamageThresholdKmh) return;

    LastImpactDamageTimeSeconds = Now;
    const float Severity = FMath::Clamp((ImpactKmh - TrailerDamageThresholdKmh) / 55.0f, 0.0f, 1.5f);
    TrailerIntegrity = FMath::Max(0.0f, TrailerIntegrity - Severity * 0.12f);
    if (bCargoLoaded && ImpactKmh >= TrailerSevereImpactKmh)
    {
        CargoIntegrity = FMath::Max(0.0f, CargoIntegrity - Severity * 0.08f);
    }
    if (bRoadsideRepairPending) CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: impact interrupted the repair; no charge."));
    RefreshPresentation();
}

bool AGTTFarmTrailer::AttachToVehicle(AGTTVehicleBase* Vehicle)
{
    if (!Vehicle || !TrailerBody || !HitchConstraint || bAttached) return false;
    UPrimitiveComponent* VehicleRoot = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent());
    if (!VehicleRoot || !VehicleRoot->IsSimulatingPhysics()) return false;

    FVector HitchLocation = Vehicle->GetActorLocation() - Vehicle->GetActorForwardVector() * 285.0f;
    if (const UGTTChaosVehicleBridgeComponent* ChaosBridge = Vehicle->FindComponentByClass<UGTTChaosVehicleBridgeComponent>())
    {
        FTransform NativeHitchTransform;
        if (ChaosBridge->TryGetNativeHitchTransform(NativeHitchTransform)) HitchLocation = NativeHitchTransform.GetLocation();
    }
    if (FVector::Distance(HitchLocation, GetActorLocation()) > 620.0f) return false;

    ConfigureHitchConstraint();
    TowVehicle = Vehicle;
    NativeTowVehicle = nullptr;
    HitchConstraint->SetWorldLocation(HitchLocation);
    HitchConstraint->SetConstrainedComponents(VehicleRoot, NAME_None, TrailerBody, NAME_None);
    bAttached = true;
    HitchLoad = 0.0f;
    return true;
}

bool AGTTFarmTrailer::AttachToNativeFieldmaster(AGTTFieldmasterNativePawn* Vehicle)
{
    if (!Vehicle || !TrailerBody || !HitchConstraint || bAttached) return false;
    if (!Vehicle->IsNativeFieldmasterReady() || !Vehicle->IsLegacyTakeoverActive()) return false;

    USkeletalMeshComponent* VehicleMesh = Vehicle->GetMesh();
    if (!VehicleMesh || !VehicleMesh->IsSimulatingPhysics()) return false;

    FTransform HitchTransform;
    if (!Vehicle->TryGetRearHitchTransform(HitchTransform)) return false;
    if (FVector::Distance(HitchTransform.GetLocation(), GetActorLocation()) > 620.0f) return false;

    ConfigureHitchConstraint();
    TowVehicle = nullptr;
    NativeTowVehicle = Vehicle;
    HitchConstraint->SetWorldLocation(HitchTransform.GetLocation());
    HitchConstraint->SetWorldRotation(HitchTransform.Rotator());
    HitchConstraint->SetConstrainedComponents(VehicleMesh, NAME_None, TrailerBody, NAME_None);
    bAttached = true;
    HitchLoad = 0.0f;
    return true;
}

void AGTTFarmTrailer::DetachTrailer()
{
    if (HitchConstraint) HitchConstraint->BreakConstraint();
    TowVehicle = nullptr;
    NativeTowVehicle = nullptr;
    bAttached = false;
    HitchLoad = 0.0f;
}

void AGTTFarmTrailer::SetCargoLoaded(bool bLoaded)
{
    bCargoLoaded = bLoaded;
    CargoIntegrity = bCargoLoaded ? 1.0f : CargoIntegrity;
    if (CargoBlock) CargoBlock->SetVisibility(false, true);
    SetCargoVisualsVisible(bCargoLoaded);
    if (TrailerBody) TrailerBody->SetMassOverrideInKg(NAME_None, bCargoLoaded ? 1680.0f : 980.0f, true);
    if (LeftWheelConstraint && LeftWheel && !bLeftWheelLost) ConfigureWheelAxle(LeftWheelConstraint, LeftWheel);
    if (RightWheelConstraint && RightWheel && !bRightWheelLost) ConfigureWheelAxle(RightWheelConstraint, RightWheel);
    RefreshPresentation();
}

bool AGTTFarmTrailer::PerformRoadsideRepair(float IntegrityRestore)
{
    RefreshAxleState();
    const bool bNeedsRepair = NeedsRoadsideRepair();
    if (!bNeedsRepair || !TrailerBody) return false;

    const bool bWasAttached = bAttached;
    AActor* PreviousTowActor = GetTowActor();
    if (bWasAttached) DetachTrailer();

    if (bLeftWheelLost) RestoreWheel(LeftWheel, LeftWheelConstraint, LeftWheelHome);
    if (bRightWheelLost) RestoreWheel(RightWheel, RightWheelConstraint, RightWheelHome);
    bLeftWheelLost = false;
    bRightWheelLost = false;
    TrailerIntegrity = FMath::Clamp(TrailerIntegrity + FMath::Max(0.05f, IntegrityRestore), 0.0f, 0.92f);
    ConfigureHitchConstraint();
    LastImpactDamageTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : -100.0f;

    if (bWasAttached && PreviousTowActor)
    {
        if (AGTTFieldmasterNativePawn* Native = Cast<AGTTFieldmasterNativePawn>(PreviousTowActor)) AttachToNativeFieldmaster(Native);
        else if (AGTTVehicleBase* Legacy = Cast<AGTTVehicleBase>(PreviousTowActor)) AttachToVehicle(Legacy);
    }
    RefreshPresentation();
    return true;
}

void AGTTFarmTrailer::ResetTrailer(const FTransform& Transform)
{
    if (bRoadsideRepairPending) CancelRoadsideRepair(TEXT("TRAILER SERVICE CANCELLED: trailer reset; no charge."));
    DetachTrailer();
    SetCargoLoaded(false);
    CargoIntegrity = 1.0f;
    TrailerIntegrity = 1.0f;
    bLeftWheelLost = false;
    bRightWheelLost = false;
    LastImpactDamageTimeSeconds = -100.0f;
    if (TrailerBody)
    {
        TrailerBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
        TrailerBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
    RestoreWheel(LeftWheel, LeftWheelConstraint, LeftWheelHome);
    RestoreWheel(RightWheel, RightWheelConstraint, RightWheelHome);
    ConfigureHitchConstraint();
    SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
    RefreshPresentation();
}