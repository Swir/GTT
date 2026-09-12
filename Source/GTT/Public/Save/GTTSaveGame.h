#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTSaveGame.generated.h"

UCLASS()
class GTT_API UGTTSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    int32 SaveVersion = 1;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    int32 Cash = 120;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    int32 FishCount = 0;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    float FishWeightKg = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    FTransform PlayerTransform;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    bool bBorrowedTractorCompleted = false;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    int32 DayNumber = 1;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    float TimeOfDayHours = 8.0f;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    bool bTractorOwned = false;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    FTransform TractorTransform;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    float TractorConditionPercent = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    float TractorFuelLiters = 18.0f;
};
