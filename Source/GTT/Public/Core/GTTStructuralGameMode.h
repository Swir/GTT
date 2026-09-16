#pragma once

#include "Core/GTTGameMode.h"
#include "GTTStructuralGameMode.generated.h"

UCLASS()
class GTT_API AGTTStructuralGameMode : public AGTTGameMode
{
    GENERATED_BODY()

public:
    virtual bool SaveProgress() override;
    virtual bool LoadProgress() override;
};
