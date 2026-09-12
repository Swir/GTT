#include "Vehicles/GTTVehicleDynamicsComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UGTTVehicleDynamicsComponent::UGTTVehicleDynamicsComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UGTTVehicleDynamicsComponent::BeginPlay()
{
    Super::BeginPlay();
    ResolveChassis();
}

void UGTTVehicleDynamicsComponent::ConfigureProfile(const FGTTVehicleDynamicsProfile& InProfile)
{
    Profile = InProfile;
    if (Profile.ForwardGearTopSpeedsKmh.Num() == 0) Profile.ForwardGearTopSpeedsKmh = {Profile.MaxSpeedKmh};
}

void UGTTVehicleDynamicsComponent::SetDriverInputs(float Throttle, float Steering)
{
    ThrottleInput = FMath::Clamp(Throttle, -1.0f, 1.0f);
    SteeringInput = FMath::Clamp(Steering, -1.0f, 1.0f);
}

void UGTTVehicleDynamicsComponent::SetPowerMultipliers(float EnginePower, float TireGrip)
{
    EnginePowerMultiplier = FMath::Clamp(EnginePower, 0.0f, 2.0f);
    TireGripMultiplier = FMath::Clamp(TireGrip, 0.05f, 2.0f);
}

void UGTTVehicleDynamicsComponent::ApplyTerrainModifier(float GripMultiplier, float InExtraRollingResistance, float DurationSeconds)
{
    const float RawGrip = FMath::Clamp(GripMultiplier, 0.1f, 1.0f);
    const float ProfileAdjustedGrip = FMath::Lerp(RawGrip, 1.0f, FMath::Clamp(Profile.OffroadGripBias, 0.0f, 0.8f));
    SurfaceGripMultiplier = FMath::Min(SurfaceGripMultiplier, ProfileAdjustedGrip);
    ExtraRollingResistance = FMath::Max(ExtraRollingResistance, FMath::Max(0.0f, InExtraRollingResistance) * (1.0f - Profile.OffroadGripBias * 0.45f));
    TerrainModifierTimeRemaining = FMath::Max(TerrainModifierTimeRemaining, FMath::Max(0.05f, DurationSeconds));
}

void UGTTVehicleDynamicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!Chassis || !Chassis->IsSimulatingPhysics())
    {
        ResolveChassis();
        return;
    }

    if (TerrainModifierTimeRemaining > 0.0f) TerrainModifierTimeRemaining = FMath::Max(0.0f, TerrainModifierTimeRemaining - DeltaTime);
    else
    {
        SurfaceGripMultiplier = FMath::FInterpTo(SurfaceGripMultiplier, 1.0f, DeltaTime, 7.0f);
        ExtraRollingResistance = FMath::FInterpTo(ExtraRollingResistance, 0.0f, DeltaTime, 7.0f);
    }

    const float SpeedKmh = Chassis->GetPhysicsLinearVelocity().Size() * 0.036f;
    UpdateGear(SpeedKmh);
    ApplySuspensionAndGrip(DeltaTime);
    ApplyDrivetrain(DeltaTime);
}

void UGTTVehicleDynamicsComponent::ResolveChassis()
{
    Chassis = GetOwner() ? Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()) : nullptr;
}

void UGTTVehicleDynamicsComponent::UpdateGear(float SpeedKmh)
{
    const float ForwardSpeed = FMath::Abs(SpeedKmh);
    CurrentGear = 1;
    for (int32 Index = 0; Index < Profile.ForwardGearTopSpeedsKmh.Num(); ++Index)
    {
        CurrentGear = Index + 1;
        if (ForwardSpeed <= Profile.ForwardGearTopSpeedsKmh[Index]) break;
    }
}

void UGTTVehicleDynamicsComponent::ApplySuspensionAndGrip(float DeltaTime)
{
    if (!GetWorld() || !GetOwner() || !Chassis) return;

    GroundContactCount = 0;
    AverageSuspensionCompression = 0.0f;
    const FVector Forward = GetOwner()->GetActorForwardVector();
    const FVector Right = GetOwner()->GetActorRightVector();
    const FVector Up = GetOwner()->GetActorUpVector();
    const FVector Origin = GetOwner()->GetActorLocation();
    const float HalfWheelBase = Profile.WheelBaseCm * 0.5f;
    const float HalfTrack = Profile.TrackWidthCm * 0.5f;
    const float TraceLength = Profile.SuspensionRestLengthCm + Profile.WheelRadiusCm;
    const float Mass = FMath::Max(1.0f, Chassis->GetMass());
    const float Grip = Profile.LateralGrip * TireGripMultiplier * SurfaceGripMultiplier;

    const FVector WheelOffsets[4] = {
        Forward * HalfWheelBase - Right * HalfTrack,
        Forward * HalfWheelBase + Right * HalfTrack,
        -Forward * HalfWheelBase - Right * HalfTrack,
        -Forward * HalfWheelBase + Right * HalfTrack
    };

    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTVehicleSuspension), false, GetOwner());
    for (const FVector& Offset : WheelOffsets)
    {
        const FVector Start = Origin + Offset + Up * 12.0f;
        const FVector End = Start - Up * TraceLength;
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params)) continue;

        ++GroundContactCount;
        const float SuspensionDistance = FMath::Max(0.0f, Hit.Distance - Profile.WheelRadiusCm);
        const float Compression = FMath::Clamp(1.0f - SuspensionDistance / FMath::Max(1.0f, Profile.SuspensionRestLengthCm), 0.0f, 1.0f);
        AverageSuspensionCompression += Compression;

        const FVector PointVelocity = Chassis->GetPhysicsLinearVelocityAtPoint(Hit.ImpactPoint);
        const float VerticalSpeed = FVector::DotProduct(PointVelocity, Up);
        const float SpringForce = Compression * Profile.SpringStrength * Mass;
        const float DamperForce = -VerticalSpeed * Profile.DamperStrength * Mass * 0.01f;
        Chassis->AddForceAtLocation(Up * FMath::Max(0.0f, SpringForce + DamperForce), Hit.ImpactPoint);

        const float LateralSpeed = FVector::DotProduct(PointVelocity, Right);
        Chassis->AddForceAtLocation(-Right * LateralSpeed * Grip * Mass * 0.11f, Hit.ImpactPoint);
    }

    if (GroundContactCount > 0) AverageSuspensionCompression /= static_cast<float>(GroundContactCount);
}

void UGTTVehicleDynamicsComponent::ApplyDrivetrain(float DeltaTime)
{
    if (!GetOwner() || !Chassis || GroundContactCount <= 0) return;

    const FVector Forward = GetOwner()->GetActorForwardVector();
    const FVector Up = GetOwner()->GetActorUpVector();
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    const float SpeedKmh = Velocity.Size2D() * 0.036f;
    const float MaxSpeed = FMath::Max(8.0f, Profile.MaxSpeedKmh);
    const float SpeedRatio = FMath::Clamp(SpeedKmh / MaxSpeed, 0.0f, 1.2f);
    const float GearRatio = 1.0f / FMath::Max(1.0f, 0.72f + static_cast<float>(CurrentGear) * 0.28f);
    const float GroundRatio = FMath::Clamp(static_cast<float>(GroundContactCount) / 4.0f, 0.25f, 1.0f);

    float DriveScale = FMath::Clamp(1.0f - FMath::Square(FMath::Min(SpeedRatio, 1.0f)), 0.05f, 1.0f);
    if (ThrottleInput < 0.0f && FVector::DotProduct(Velocity, Forward) > 80.0f) DriveScale *= Profile.BrakeStrength;

    const float DriveForce = ThrottleInput * Profile.MaxDriveForce * EnginePowerMultiplier * GearRatio * DriveScale * GroundRatio;
    Chassis->AddForce(Forward * DriveForce, NAME_None, true);

    const float SteeringSpeedFactor = FMath::Clamp(SpeedKmh / 15.0f, 0.15f, 1.0f) * FMath::Clamp(1.15f - SpeedRatio * 0.55f, 0.45f, 1.0f);
    const float SteeringGrip = TireGripMultiplier * SurfaceGripMultiplier;
    Chassis->AddTorqueInRadians(Up * SteeringInput * Profile.MaxSteerTorque * SteeringSpeedFactor * SteeringGrip, NAME_None, true);

    const float Rolling = Profile.RollingResistance + ExtraRollingResistance;
    if (Rolling > 0.0f && !Velocity.IsNearlyZero(4.0f))
    {
        Chassis->AddForce(-Velocity.GetSafeNormal() * Velocity.Size() * Rolling * Chassis->GetMass() * 0.012f);
    }
}

FString UGTTVehicleDynamicsComponent::GetDynamicsSummary() const
{
    return FString::Printf(TEXT("GEAR %d/%d | CONTACT %d/4 | SUSP %.0f%% | GRIP %.0f%%"),
        CurrentGear,
        FMath::Max(1, Profile.ForwardGearTopSpeedsKmh.Num()),
        GroundContactCount,
        AverageSuspensionCompression * 100.0f,
        SurfaceGripMultiplier * TireGripMultiplier * 100.0f);
}
