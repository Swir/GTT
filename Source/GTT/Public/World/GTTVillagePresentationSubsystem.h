#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "GTTVillagePresentationSubsystem.generated.h"

class APointLight;
class AStaticMeshActor;

/**
 * Runtime presentation pass for the source-built prototype world.
 * Keeps gameplay-readable signage near the player while reducing distant text clutter,
 * adds original roadside dressing and day/night street lighting without external assets.
 */
UCLASS()
class GTT_API UGTTVillagePresentationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintPure, Category="GTT|World|Presentation")
    bool IsPresentationPassActive() const { return bPresentationBuilt; }

    UFUNCTION(BlueprintPure, Category="GTT|World|Presentation")
    int32 GetStreetLightCount() const { return StreetLights.Num(); }

private:
    void BuildPresentation();
    void RefreshPresentation();
    AStaticMeshActor* SpawnProp(const FVector& Location, const FVector& Scale, const FRotator& Rotation, const TCHAR* MeshPath);
    APointLight* SpawnStreetLight(const FVector& Location);

    UPROPERTY(Transient)
    TArray<TObjectPtr<APointLight>> StreetLights;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AStaticMeshActor>> PresentationProps;

    FTimerHandle RefreshTimer;
    bool bPresentationBuilt = false;
};
