#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GTTChaosPowertrainSetupLibrary.generated.h"

class UChaosWheeledVehicleMovementComponent;

UCLASS()
class GTT_API UGTTChaosPowertrainSetupLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    static bool ConfigureCanonicalPowertrain(UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    static bool ValidateCanonicalPowertrain(const UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary);
};
