#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "GTTChaosVehicleWheels.generated.h"

/** Native Chaos wheel classes backed by the canonical GTT vehicle specs. */
UCLASS()
class GTT_API UGTTFieldmasterFrontWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UGTTFieldmasterFrontWheel();
};

UCLASS()
class GTT_API UGTTFieldmasterRearWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UGTTFieldmasterRearWheel();
};

UCLASS()
class GTT_API UGTTRattlebackFrontWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UGTTRattlebackFrontWheel();
};

UCLASS()
class GTT_API UGTTRattlebackRearWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UGTTRattlebackRearWheel();
};

UCLASS()
class GTT_API UGTTMuleboxFrontWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UGTTMuleboxFrontWheel();
};

UCLASS()
class GTT_API UGTTMuleboxRearWheel : public UChaosVehicleWheel
{
    GENERATED_BODY()
public:
    UGTTMuleboxRearWheel();
};
