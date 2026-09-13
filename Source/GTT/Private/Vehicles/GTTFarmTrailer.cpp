#include "Vehicles/GTTFarmTrailer.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTChaosVehicleBridgeComponent.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
    constexpr float TrailerImpactCooldownSeconds = 0.35f;
    constexpr float TrailerDamageThresholdKmh = 18.0f;
    constexpr float TrailerSevereImpactKmh = 42.0f;
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
    TrailerBody->SetRelativeScale3D(FVector(2.9f, 1.25f, 0.28f));
    TrailerBody->SetSimulatePhysics(true);
    TrailerBody->SetNotifyRigidBodyCollision(true);
    TrailerBody->SetMassOverrideInKg(NAME_None, 980.0f, true);
    TrailerBody->SetLinearDamping(0.45f);
    TrailerBody->SetAngularDamping(1.4f);

    LeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWheel"));
    LeftWheel->SetupAttachment(TrailerBody);
    LeftWheel->SetStaticMesh(Cylinder);
    LeftWheel->SetRelativeLocation(FVector(70.0f, -145.0f, -62.0f));
    LeftWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftWheel->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.32f));
    LeftWheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    LeftWheel->SetSimulatePhysics(true);
    LeftWheel->SetMassOverrideInKg(NAME_None, 74.0f, true);
    LeftWheel->SetLinearDamping(0.18f);
    LeftWheel->SetAngularDamping(0.10f);

    RightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWheel"));
    RightWheel->SetupAttachment(TrailerBody);
    RightWheel->SetStaticMesh(Cylinder);
    RightWheel->SetRelativeLocation(FVector(70.0f, 145.0f, -62.0f));
    RightWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightWheel->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.32f));
    RightWheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RightWheel->SetSimulatePhysics(true);
    RightWheel->SetMassOverrideInKg(NAME_None, 74.0f, true);
    RightWheel->SetLinearDamping(0.18f);
    RightWheel->SetAngularDamping(0.10f);

    LeftWheelConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("LeftWheelConstraint"));
    LeftWheelConstraint->SetupAttachment(TrailerBody);
    LeftWheelConstraint->SetRelativeLocation(FVector(70.0f, -145.0f, -62.0f));

    RightWheelConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("RightWheelConstraint"));
    RightWheelConstraint->SetupAttachment(TrailerBody);
    RightWheelConstraint->SetRelativeLocation(FVector(70.0f, 145.0f, -62.0f));

    CargoBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoBlock"));
    CargoBlock->SetupAttachment(TrailerBody);
    CargoBlock->SetStaticMesh(Cube);
    CargoBlock->SetRelativeLocation(FVector(20.0f, 0.0f, 105.0f));
    CargoBlock->SetRelativeScale3D(FVector(2.25f, 0.95f, 0.65f));
    CargoBlock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CargoBlock->SetVisibility(false, true);

    HitchConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("HitchConstraint"));
    HitchConstraint->SetupAttachment(TrailerBody);
    HitchConstraint->SetDisableCollision(true);
    HitchConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, 90.0f);
    HitchConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, 70.0f);
    HitchConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, 65.0f);
    HitchConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 42.0f);
    HitchConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 24.0f);
    HitchConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 18.0f);
}

void AGTTFarmTrailer::BeginPlay()
{
    Super::BeginPlay();
    ConfigureWheelAxle(LeftWheelConstraint, LeftWheel);
    ConfigureWheelAxle(RightWheelConstraint, RightWheel);
}

void AGTTFarmTrailer::ConfigureWheelAxle(UPhysicsConstraintComponent* Constraint, UStaticMeshComponent* Wheel)
{
    if (!Constraint || !Wheel || !TrailerBody)
    {
        return;
    }

    Constraint->SetDisableCollision(true);
    Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
    Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
    Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.0f);
    Constraint->SetLinearBreakable(true, WheelBreakForce);
    Constraint->SetAngularBreakable(true, WheelBreakTorque);
    Constraint->SetConstrainedComponents(TrailerBody, NAME_None, Wheel, NAME_None);
}

AActor* AGTTFarmTrailer::GetTowActor() const
{
    if (NativeTowVehicle)
    {
        return NativeTowVehicle;
    }
    return TowVehicle;
}

bool AGTTFarmTrailer::HasIntactAxle() const
{
    return !bLeftWheelLost && !bRightWheelLost;
}

void AGTTFarmTrailer::RefreshAxleState()
{
    if (LeftWheelConstraint && LeftWheelConstraint->IsBroken())
    {
        bLeftWheelLost = true;
    }
    if (RightWheelConstraint && RightWheelConstraint->IsBroken())
    {
        bRightWheelLost = true;
    }

    const int32 LostWheelCount = (bLeftWheelLost ? 1 : 0) + (bRightWheelLost ? 1 : 0);
    if (LostWheelCount > 0)
    {
        TrailerIntegrity = FMath::Min(TrailerIntegrity, LostWheelCount == 2 ? 0.25f : 0.55f);
        if (bCargoLoaded)
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
}

void AGTTFarmTrailer::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

    if (!GetWorld() || Other == this || !TrailerBody)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastImpactDamageTimeSeconds < TrailerImpactCooldownSeconds)
    {
        return;
    }

    const float SpeedKmh = GetVelocity().Size() * 0.036f;
    const float MassKg = FMath::Max(TrailerBody->GetMass(), 1.0f);
    const float ImpulseEquivalentKmh = (NormalImpulse.Size() / MassKg) * 0.036f;
    const float ImpactKmh = FMath::Max(SpeedKmh, ImpulseEquivalentKmh);
    if (ImpactKmh < TrailerDamageThresholdKmh)
    {
        return;
    }

    LastImpactDamageTimeSeconds = Now;
    const float Severity = FMath::Clamp((ImpactKmh - TrailerDamageThresholdKmh) / 55.0f, 0.0f, 1.5f);
    TrailerIntegrity = FMath::Max(0.0f, TrailerIntegrity - Severity * 0.12f);

    if (bCargoLoaded && ImpactKmh >= TrailerSevereImpactKmh)
    {
        CargoIntegrity = FMath::Max(0.0f, CargoIntegrity - Severity * 0.08f);
    }
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
        if (ChaosBridge->TryGetNativeHitchTransform(NativeHitchTransform))
        {
            HitchLocation = NativeHitchTransform.GetLocation();
        }
    }

    const float HitchDistance = FVector::Distance(HitchLocation, GetActorLocation());
    if (HitchDistance > 620.0f) return false;

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
    if (CargoBlock) CargoBlock->SetVisibility(bCargoLoaded, true);
    if (TrailerBody) TrailerBody->SetMassOverrideInKg(NAME_None, bCargoLoaded ? 1680.0f : 980.0f, true);
}

void AGTTFarmTrailer::ResetTrailer(const FTransform& Transform)
{
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
    if (LeftWheel)
    {
        LeftWheel->SetPhysicsLinearVelocity(FVector::ZeroVector);
        LeftWheel->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
    if (RightWheel)
    {
        RightWheel->SetPhysicsLinearVelocity(FVector::ZeroVector);
        RightWheel->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
    SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
    ConfigureWheelAxle(LeftWheelConstraint, LeftWheel);
    ConfigureWheelAxle(RightWheelConstraint, RightWheel);
}