#include "Vehicles/GTTTrailerRoadFeedbackSubsystem.h"

#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTFarmTrailer.h"

namespace GTTTrailerRoadFeedback
{
    constexpr FVector TailLeftLocation(286.0f, -88.0f, 24.0f);
    constexpr FVector TailRightLocation(286.0f, 88.0f, 24.0f);
    constexpr FVector ReverseLeftLocation(287.0f, -48.0f, 20.0f);
    constexpr FVector ReverseRightLocation(287.0f, 48.0f, 20.0f);
    constexpr FVector HazardLeftLocation(282.0f, -116.0f, 34.0f);
    constexpr FVector HazardRightLocation(282.0f, 116.0f, 34.0f);

    UPointLightComponent* CreateRoadLight(
        AGTTFarmTrailer* Trailer,
        const FName Name,
        const FVector& RelativeLocation,
        const FLinearColor& Color,
        const float AttenuationRadius)
    {
        if (!Trailer || !Trailer->GetRootComponent())
        {
            return nullptr;
        }

        UPointLightComponent* Light = NewObject<UPointLightComponent>(Trailer, Name, RF_Transient);
        if (!Light)
        {
            return nullptr;
        }

        Trailer->AddInstanceComponent(Light);
        Light->SetupAttachment(Trailer->GetRootComponent());
        Light->SetRelativeLocation(RelativeLocation);
        Light->SetLightColor(Color);
        Light->SetAttenuationRadius(AttenuationRadius);
        Light->SetCastShadows(false);
        Light->SetIntensity(0.0f);
        Light->RegisterComponent();
        return Light;
    }

    void SetLampIntensity(const TWeakObjectPtr<UPointLightComponent>& Light, const float Intensity)
    {
        if (UPointLightComponent* Component = Light.Get())
        {
            Component->SetIntensity(FMath::Max(0.0f, Intensity));
            Component->SetVisibility(Intensity > KINDA_SMALL_NUMBER, true);
        }
    }
}

TStatId UGTTTrailerRoadFeedbackSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTTrailerRoadFeedbackSubsystem, STATGROUP_Tickables);
}

void UGTTTrailerRoadFeedbackSubsystem::Tick(const float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE))
    {
        return;
    }

    const float SafeDeltaTime = FMath::Max(DeltaTime, 0.001f);
    TSet<TWeakObjectPtr<AGTTFarmTrailer>> SeenTrailers;

    for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
    {
        AGTTFarmTrailer* Trailer = *It;
        if (!IsValid(Trailer))
        {
            continue;
        }

        TWeakObjectPtr<AGTTFarmTrailer> TrailerKey(Trailer);
        SeenTrailers.Add(TrailerKey);
        FGTTTrailerRoadFeedbackRuntime& Runtime = RuntimeByTrailer.FindOrAdd(TrailerKey);
        EnsureRoadLights(Trailer, Runtime);

        UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Trailer->GetRootComponent());
        const FVector Velocity = Body ? Body->GetPhysicsLinearVelocity() : Trailer->GetVelocity();
        const float LongitudinalSpeedKmh = FVector::DotProduct(Velocity, Trailer->GetActorForwardVector()) * 0.036f;
        const float PreviousAbsSpeed = FMath::Abs(Runtime.LastLongitudinalSpeedKmh);
        const float CurrentAbsSpeed = FMath::Abs(LongitudinalSpeedKmh);
        const float DecelerationKmhPerSecond = Runtime.bHasVelocitySample
            ? FMath::Max(0.0f, (PreviousAbsSpeed - CurrentAbsSpeed) / SafeDeltaTime)
            : 0.0f;

        UpdateRoadLights(Trailer, Runtime, LongitudinalSpeedKmh, DecelerationKmhPerSecond);
        ApplyLoadedTrailerStability(Trailer, SafeDeltaTime);

        Runtime.LastLongitudinalSpeedKmh = LongitudinalSpeedKmh;
        Runtime.bHasVelocitySample = true;
    }

    for (auto It = RuntimeByTrailer.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid() || !SeenTrailers.Contains(It.Key()))
        {
            It.RemoveCurrent();
        }
    }
}

void UGTTTrailerRoadFeedbackSubsystem::EnsureRoadLights(
    AGTTFarmTrailer* Trailer,
    FGTTTrailerRoadFeedbackRuntime& Runtime)
{
    using namespace GTTTrailerRoadFeedback;

    if (!Runtime.TailLeft.IsValid())
    {
        Runtime.TailLeft = CreateRoadLight(Trailer, TEXT("RoadFeedback_TailLeft"), TailLeftLocation, FLinearColor(1.0f, 0.02f, 0.01f), 360.0f);
    }
    if (!Runtime.TailRight.IsValid())
    {
        Runtime.TailRight = CreateRoadLight(Trailer, TEXT("RoadFeedback_TailRight"), TailRightLocation, FLinearColor(1.0f, 0.02f, 0.01f), 360.0f);
    }
    if (!Runtime.ReverseLeft.IsValid())
    {
        Runtime.ReverseLeft = CreateRoadLight(Trailer, TEXT("RoadFeedback_ReverseLeft"), ReverseLeftLocation, FLinearColor(0.85f, 0.92f, 1.0f), 310.0f);
    }
    if (!Runtime.ReverseRight.IsValid())
    {
        Runtime.ReverseRight = CreateRoadLight(Trailer, TEXT("RoadFeedback_ReverseRight"), ReverseRightLocation, FLinearColor(0.85f, 0.92f, 1.0f), 310.0f);
    }
    if (!Runtime.HazardLeft.IsValid())
    {
        Runtime.HazardLeft = CreateRoadLight(Trailer, TEXT("RoadFeedback_HazardLeft"), HazardLeftLocation, FLinearColor(1.0f, 0.24f, 0.0f), 420.0f);
    }
    if (!Runtime.HazardRight.IsValid())
    {
        Runtime.HazardRight = CreateRoadLight(Trailer, TEXT("RoadFeedback_HazardRight"), HazardRightLocation, FLinearColor(1.0f, 0.24f, 0.0f), 420.0f);
    }
}

void UGTTTrailerRoadFeedbackSubsystem::UpdateRoadLights(
    AGTTFarmTrailer* Trailer,
    FGTTTrailerRoadFeedbackRuntime& Runtime,
    const float LongitudinalSpeedKmh,
    const float DecelerationKmhPerSecond) const
{
    using namespace GTTTrailerRoadFeedback;

    const bool bAttached = Trailer->IsAttached();
    const bool bMoving = FMath::Abs(LongitudinalSpeedKmh) > 3.0f;
    const bool bBraking = bAttached && bMoving && DecelerationKmhPerSecond >= BrakeDecelerationThresholdKmhPerSecond;
    const bool bReversing = bAttached && LongitudinalSpeedKmh <= ReverseLightThresholdKmh;
    const bool bCriticalTrailerState = Trailer->GetLostWheelCount() > 0
        || Trailer->GetHitchIntegrity() < CriticalIntegrityThreshold
        || Trailer->GetTrailerIntegrity() < CriticalIntegrityThreshold
        || Trailer->IsRoadsideRepairPending();

    const float TailIntensity = bAttached ? (bBraking ? 2600.0f : 480.0f) : 0.0f;
    SetLampIntensity(Runtime.TailLeft, TailIntensity);
    SetLampIntensity(Runtime.TailRight, TailIntensity);

    const float ReverseIntensity = bReversing ? 1750.0f : 0.0f;
    SetLampIntensity(Runtime.ReverseLeft, ReverseIntensity);
    SetLampIntensity(Runtime.ReverseRight, ReverseIntensity);

    const UWorld* World = GetWorld();
    const float PulseTime = World ? World->GetTimeSeconds() : 0.0f;
    const float HazardPulse = 0.5f + 0.5f * FMath::Sin(PulseTime * UE_PI * 2.8f);
    const float HazardIntensity = bCriticalTrailerState ? 2200.0f * HazardPulse : 0.0f;
    SetLampIntensity(Runtime.HazardLeft, HazardIntensity);
    SetLampIntensity(Runtime.HazardRight, HazardIntensity);
}

void UGTTTrailerRoadFeedbackSubsystem::ApplyLoadedTrailerStability(
    AGTTFarmTrailer* Trailer,
    const float DeltaTime) const
{
    if (!Trailer->IsAttached() || !Trailer->HasCargo() || Trailer->GetLostWheelCount() > 0)
    {
        return;
    }

    UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Trailer->GetRootComponent());
    if (!Body || !Body->IsSimulatingPhysics())
    {
        return;
    }

    const FVector Velocity = Body->GetPhysicsLinearVelocity();
    const float SpeedKmh = Velocity.Size() * 0.036f;
    const float SpeedAuthority = FMath::Clamp(
        (SpeedKmh - StabilityStartSpeedKmh) / (StabilityFullSpeedKmh - StabilityStartSpeedKmh),
        0.0f,
        1.0f);
    if (SpeedAuthority <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float LoadAuthority = FMath::Clamp(Trailer->GetTowLoadFactor(), 0.0f, 1.0f);
    const float Integrity = FMath::Min(Trailer->GetTrailerIntegrity(), Trailer->GetHitchIntegrity());
    const float IntegrityAuthority = FMath::Clamp((Integrity - 0.20f) / 0.80f, 0.15f, 1.0f);
    const float Authority = SpeedAuthority * LoadAuthority * IntegrityAuthority * MaximumStabilityAuthority;
    if (Authority <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float MassKg = FMath::Max(Body->GetMass(), 1.0f);
    const FVector Right = Trailer->GetActorRightVector();
    const float LateralSpeedCmPerSecond = FVector::DotProduct(Velocity, Right);
    const float LateralForceMagnitude = FMath::Clamp(
        -LateralSpeedCmPerSecond * MassKg * 9.0f * Authority,
        -MaximumLateralStabilityForce,
        MaximumLateralStabilityForce);
    Body->AddForce(Right * LateralForceMagnitude);

    const float YawRateRadians = Body->GetPhysicsAngularVelocityInRadians().Z;
    const float YawTorque = FMath::Clamp(
        -YawRateRadians * MassKg * 180000.0f * Authority,
        -MaximumYawStabilityTorque,
        MaximumYawStabilityTorque);
    Body->AddTorqueInRadians(FVector(0.0f, 0.0f, YawTorque));

    (void)DeltaTime;
}
