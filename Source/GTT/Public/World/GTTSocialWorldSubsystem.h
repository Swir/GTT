#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTSocialWorldSubsystem.generated.h"

UCLASS()
class GTT_API UGTTSocialWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    bool bBuilt = false;
};
