#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FGTTStoredVehicleData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    FName VehicleId = NAME_None;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    float ConditionPercent = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    float FuelLiters = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    int32 EngineUpgradeLevel = 0;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    int32 TireUpgradeLevel = 0;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle")
    float TireIntegrity = 1.0f;
};

UCLASS()
class GTT_API UGTTSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category="GTT|Save")
    int32 SaveVersion = 3;

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

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Garage")
    TArray<FGTTStoredVehicleData> OwnedVehicles;

    // Version 1 migration fields. Kept so existing prototype saves still load.
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy")
    bool bTractorOwned = false;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy")
    FTransform TractorTransform;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy")
    float TractorConditionPercent = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy")
    float TractorFuelLiters = 18.0f;
};
