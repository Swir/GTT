#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTFarmRouteBeacon.generated.h"

class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Presentation-only world-space guidance for the legal farm cargo loop.
 *
 * Contract, cargo, economy and reputation authority stays in the existing
 * farm/logistics systems. This actor only visualizes the next real terminal so
 * the playable loop can be followed without relying on prototype wall labels.
 */
UCLASS()
class GTT_API AGTTFarmRouteBeacon : public AActor
{
    GENERATED_BODY()

public:
    AGTTFarmRouteBeacon();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|Route")
    bool IsRouteBeaconDeployed() const { return bDeployed; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|FarmJob|Route")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|FarmJob|Route")
    TObjectPtr<UStaticMeshComponent> GroundRing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|FarmJob|Route")
    TObjectPtr<UStaticMeshComponent> Pointer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|FarmJob|Route")
    TObjectPtr<UTextRenderComponent> RouteLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|FarmJob|Route")
    TObjectPtr<UPointLightComponent> RouteLight;

private:
    AGTTFarmJobDirector* ResolveDirector();
    AGTTFarmJobTerminal* ResolveTargetTerminal(uint8 StageValue);
    FVector ResolveGroundedLocation(const FVector& DesiredLocation) const;
    void SetDeployed(bool bShouldDeploy);
    void UpdatePresentation(float DeltaSeconds, uint8 StageValue, float DistanceMeters);
    void FaceLocalPlayer();

    TWeakObjectPtr<AGTTFarmJobDirector> Director;
    TWeakObjectPtr<AGTTFarmJobTerminal> TargetTerminal;
    uint8 CachedStageValue = 255;
    bool bDeployed = false;
    float PulseClock = 0.0f;
};
