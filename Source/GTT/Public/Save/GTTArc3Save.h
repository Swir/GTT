#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTArc3Save.generated.h"

UCLASS()
class GTT_API UGTTArc3Save : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category="GTT|Story|Arc3|Save")
    int32 Arc3SaveVersion = 1;

    UPROPERTY(VisibleAnywhere, Category="GTT|Story|Arc3|Save")
    int32 Arc3Stage = 0;
};
