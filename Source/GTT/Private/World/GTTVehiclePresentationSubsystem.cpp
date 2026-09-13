#include "World/GTTVehiclePresentationSubsystem.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Vehicles/GTTVehicleBase.h"
#include "World/GTTDayNightCycle.h"

bool UGTTVehiclePresentationSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGTTVehiclePresentationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    for (TActorIterator<AGTTDayNightCycle> It(&InWorld); It; ++It)
    {
        DayNightCycle = *It;
        break;
    }

    RefreshFleet();
    InWorld.GetTimerManager().SetTimer(RefreshFleetTimer, this, &UGTTVehiclePresentationSubsystem::RefreshFleet, 2.0f, true);
    InWorld.GetTimerManager().SetTimer(UpdatePresentationTimer, this, &UGTTVehiclePresentationSubsystem::UpdatePresentation, 0.08f, true);
    UpdatePresentation();
}

void UGTTVehiclePresentationSubsystem::RefreshFleet()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (!DayNightCycle)
    {
        for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It)
        {
            DayNightCycle = *It;
            break;
        }
    }

    VehiclePresentation.RemoveAll([](const FGTTVehiclePresentationRuntime& Runtime)
    {
        return !IsValid(Runtime.Vehicle);
    });

    for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        const bool bAlreadyRegistered = VehiclePresentation.ContainsByPredicate([Vehicle](const FGTTVehiclePresentationRuntime& Runtime)
        {
            return Runtime.Vehicle == Vehicle;
        });

        if (!bAlreadyRegistered)
        {
            RegisterVehicle(Vehicle);
        }
    }
}

void UGTTVehiclePresentationSubsystem::RegisterVehicle(AGTTVehicleBase* Vehicle)
{
    if (!IsValid(Vehicle) || !Vehicle->GetRootComponent())
    {
        return;
    }

    FVector FrontLeft;
    FVector FrontRight;
    FVector RearLeft;
    FVector RearRight;
    ConfigureLayout(Vehicle->GetPersistentVehicleId(), FrontLeft, FrontRight, RearLeft, RearRight);

    FGTTVehiclePresentationRuntime Runtime;
    Runtime.Vehicle = Vehicle;
    Runtime.HeadlightLeft = CreateHeadlight(Vehicle, TEXT("GTTHeadlightLeft"), FrontLeft);
    Runtime.HeadlightRight = CreateHeadlight(Vehicle, TEXT("GTTHeadlightRight"), FrontRight);
    Runtime.RearLightLeft = CreateRearLight(Vehicle, TEXT("GTTRearLightLeft"), RearLeft, FLinearColor(1.0f, 0.02f, 0.01f), 360.0f);
    Runtime.RearLightRight = CreateRearLight(Vehicle, TEXT("GTTRearLightRight"), RearRight, FLinearColor(1.0f, 0.02f, 0.01f), 360.0f);

    const FVector ReverseLeft = RearLeft + FVector(0.0f, -16.0f, 8.0f);
    const FVector ReverseRight = RearRight + FVector(0.0f, 16.0f, 8.0f);
    Runtime.ReverseLightLeft = CreateRearLight(Vehicle, TEXT("GTTReverseLightLeft"), ReverseLeft, FLinearColor::White, 300.0f);
    Runtime.ReverseLightRight = CreateRearLight(Vehicle, TEXT("GTTReverseLightRight"), ReverseRight, FLinearColor::White, 300.0f);
    Runtime.LastSpeedKmh = Vehicle->GetSpeedKmh();

    VehiclePresentation.Add(Runtime);
}

void UGTTVehiclePresentationSubsystem::ConfigureLayout(FName VehicleId, FVector& FrontLeft, FVector& FrontRight, FVector& RearLeft, FVector& RearRight) const
{
    float FrontX = 118.0f;
    float RearX = -118.0f;
    float HalfWidth = 54.0f;
    float FrontZ = 42.0f;
    float RearZ = 38.0f;

    if (VehicleId == TEXT("RustyFieldmaster60"))
    {
        FrontX = 132.0f;
        RearX = -118.0f;
        HalfWidth = 64.0f;
        FrontZ = 74.0f;
        RearZ = 54.0f;
    }
    else if (VehicleId == TEXT("Mulebox1200"))
    {
        FrontX = 146.0f;
        RearX = -148.0f;
        HalfWidth = 61.0f;
        FrontZ = 58.0f;
        RearZ = 52.0f;
    }

    FrontLeft = FVector(FrontX, -HalfWidth, FrontZ);
    FrontRight = FVector(FrontX, HalfWidth, FrontZ);
    RearLeft = FVector(RearX, -HalfWidth, RearZ);
    RearRight = FVector(RearX, HalfWidth, RearZ);
}

USpotLightComponent* UGTTVehiclePresentationSubsystem::CreateHeadlight(AGTTVehicleBase* Vehicle, const FName& Name, const FVector& RelativeLocation)
{
    if (!Vehicle || !Vehicle->GetRootComponent())
    {
        return nullptr;
    }

    USpotLightComponent* Light = NewObject<USpotLightComponent>(Vehicle, Name);
    Vehicle->AddInstanceComponent(Light);
    Light->SetupAttachment(Vehicle->GetRootComponent());
    Light->SetRelativeLocation(RelativeLocation);
    Light->SetRelativeRotation(FRotator::ZeroRotator);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensity(6500.0f);
    Light->SetAttenuationRadius(2100.0f);
    Light->SetInnerConeAngle(18.0f);
    Light->SetOuterConeAngle(34.0f);
    Light->SetLightColor(FLinearColor(1.0f, 0.82f, 0.58f));
    Light->SetCastShadows(false);
    Light->SetVisibility(false);
    Light->RegisterComponent();
    return Light;
}

UPointLightComponent* UGTTVehiclePresentationSubsystem::CreateRearLight(AGTTVehicleBase* Vehicle, const FName& Name, const FVector& RelativeLocation, const FLinearColor& Color, float Radius)
{
    if (!Vehicle || !Vehicle->GetRootComponent())
    {
        return nullptr;
    }

    UPointLightComponent* Light = NewObject<UPointLightComponent>(Vehicle, Name);
    Vehicle->AddInstanceComponent(Light);
    Light->SetupAttachment(Vehicle->GetRootComponent());
    Light->SetRelativeLocation(RelativeLocation);
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensity(0.0f);
    Light->SetAttenuationRadius(Radius);
    Light->SetLightColor(Color);
    Light->SetCastShadows(false);
    Light->SetVisibility(false);
    Light->RegisterComponent();
    return Light;
}

void UGTTVehiclePresentationSubsystem::UpdatePresentation()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const bool bNight = DayNightCycle && DayNightCycle->IsNight();
    const float WorldSeconds = World->GetTimeSeconds();
    constexpr float UpdateSeconds = 0.08f;

    for (FGTTVehiclePresentationRuntime& Runtime : VehiclePresentation)
    {
        AGTTVehicleBase* Vehicle = Runtime.Vehicle;
        if (!IsValid(Vehicle))
        {
            continue;
        }

        const bool bEngine = Vehicle->IsEngineRunning();
        const bool bOccupied = Vehicle->IsOccupied();
        const float SpeedKmh = Vehicle->GetSpeedKmh();
        const float DecelerationKmhPerSecond = (Runtime.LastSpeedKmh - SpeedKmh) / UpdateSeconds;
        const float ForwardSpeedCms = FVector::DotProduct(Vehicle->GetVelocity(), Vehicle->GetActorForwardVector());
        const bool bReversing = bEngine && ForwardSpeedCms < -35.0f;
        const bool bBraking = bOccupied && bEngine && ((DecelerationKmhPerSecond > 7.0f && SpeedKmh > 2.0f) || SpeedKmh < 0.75f);

        const float Condition = Vehicle->GetConditionPercent();
        const bool bElectricalFlicker = Condition < 0.28f && FMath::Sin(WorldSeconds * 19.0f + Vehicle->GetUniqueID() * 0.13f) < -0.25f;
        const bool bHeadlightsOn = bEngine && bNight && !bElectricalFlicker;
        const float HeadlightIntensity = FMath::Lerp(4100.0f, 6500.0f, FMath::Clamp(Condition, 0.0f, 1.0f));

        for (USpotLightComponent* Headlight : {Runtime.HeadlightLeft.Get(), Runtime.HeadlightRight.Get()})
        {
            if (!Headlight) continue;
            Headlight->SetIntensity(HeadlightIntensity);
            Headlight->SetVisibility(bHeadlightsOn);
        }

        const bool bTailVisible = bEngine && (bNight || bBraking);
        const float RearIntensity = bBraking ? 1250.0f : 110.0f;
        for (UPointLightComponent* RearLight : {Runtime.RearLightLeft.Get(), Runtime.RearLightRight.Get()})
        {
            if (!RearLight) continue;
            RearLight->SetIntensity(RearIntensity);
            RearLight->SetVisibility(bTailVisible);
        }

        for (UPointLightComponent* ReverseLight : {Runtime.ReverseLightLeft.Get(), Runtime.ReverseLightRight.Get()})
        {
            if (!ReverseLight) continue;
            ReverseLight->SetIntensity(900.0f);
            ReverseLight->SetVisibility(bReversing);
        }

        Runtime.LastSpeedKmh = SpeedKmh;
    }
}
