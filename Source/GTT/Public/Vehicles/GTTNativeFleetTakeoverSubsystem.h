#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeFleetTakeoverSubsystem.generated.h"

UCLASS()
class GTT_API UGTTNativeFleetTakeoverSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
