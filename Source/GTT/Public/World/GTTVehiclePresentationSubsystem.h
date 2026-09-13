#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "GTTVehiclePresentationSubsystem.generated.h"

class AGTTDayNightCycle;
class AGTTVehicleBase;
class UPointLightComponent;
class USpotLightComponent;

USTRUCT()
struct FGTTVehiclePresentationRuntime
{
    GENERATED_BODY()

    UPROPERTY(Transient) TObjectPtr<AGTTVehicleBase> Vehicle;
    UPROPERTY(Transient) TObjectPtr<USpotLightComponent> HeadlightLeft;
    UPROPERTY(Transient) TObjectPtr<USpotLightComponent> HeadlightRight;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> RearLightLeft;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> RearLightRight;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> ReverseLightLeft;
    UPROPERTY(Transient) TObjectPtr<UPointLightComponent> ReverseLightRight;

    float LastSpeedKmh = 0.0f;
};

/**
 * Gameplay-driven presentation layer for the owned/driveable vehicle fleet.
 * Adds automatic night headlights, tail/brake lamps, reversing lamps and
 * damage-linked headlight instability without importing external art assets.
 */
UCLASS()
class GTT_API UGTTVehiclePresentationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Presentation")
    int32 GetPresentedVehicleCount() const { return VehiclePresentation.Num(); }

private:
    void RefreshFleet();
    void UpdatePresentation();
    void RegisterVehicle(AGTTVehicleBase* Vehicle);
    void ConfigureLayout(FName VehicleId, FVector& FrontLeft, FVector& FrontRight, FVector& RearLeft, FVector& RearRight) const;
    USpotLightComponent* CreateHeadlight(AGTTVehicleBase* Vehicle, const FName& Name, const FVector& RelativeLocation);
    UPointLightComponent* CreateRearLight(AGTTVehicleBase* Vehicle, const FName& Name, const FVector& RelativeLocation, const FLinearColor& Color, float Radius);

    UPROPERTY(Transient)
    TArray<FGTTVehiclePresentationRuntime> VehiclePresentation;

    UPROPERTY(Transient)
    TObjectPtr<AGTTDayNightCycle> DayNightCycle;

    FTimerHandle RefreshFleetTimer;
    FTimerHandle UpdatePresentationTimer;
};
