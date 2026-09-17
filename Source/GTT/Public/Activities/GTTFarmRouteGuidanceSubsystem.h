#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmRouteGuidanceSubsystem.generated.h"

class AGTTFarmRouteBeacon;

/** Owns the single presentation beacon used by the legal farm cargo loop. */
UCLASS()
class GTT_API UGTTFarmRouteGuidanceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|Route")
    bool HasRouteBeacon() const { return RouteBeacon.IsValid(); }

private:
    TWeakObjectPtr<AGTTFarmRouteBeacon> RouteBeacon;
};
