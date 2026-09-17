#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRangerPullOverMarker.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Lightweight world-space shoulder target for active warden road stops.
 *
 * The actor owns presentation only. Position/phase authority remains in
 * UGTTRangerRoadStopSubsystem so HUD, traffic, ranger staging and this marker
 * can never disagree about where the player is expected to stop.
 */
UCLASS()
class GTT_API AGTTRangerPullOverMarker : public AActor
{
    GENERATED_BODY()

public:
    AGTTRangerPullOverMarker();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|Ranger|Road Stop")
    bool IsMarkerDeployed() const { return bMarkerDeployed; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Road Stop")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Road Stop")
    TObjectPtr<UStaticMeshComponent> GroundDisc;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Road Stop")
    TObjectPtr<UStaticMeshComponent> DirectionChevron;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Road Stop")
    TObjectPtr<UTextRenderComponent> MarkerLabel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger|Road Stop")
    TObjectPtr<UPointLightComponent> MarkerLight;

private:
    void SetMarkerDeployed(bool bDeployed);
    FVector ResolveGroundedLocation(const FVector& DesiredLocation) const;
    void UpdatePresentation(float DeltaSeconds, bool bSearchPhase);

    bool bMarkerDeployed = false;
    float PulseClock = 0.0f;
};