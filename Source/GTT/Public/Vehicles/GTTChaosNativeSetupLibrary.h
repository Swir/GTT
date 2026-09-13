#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GTTChaosNativeSetupLibrary.generated.h"

class UChaosWheeledVehicleMovementComponent;

UCLASS()
class GTT_API UGTTChaosNativeSetupLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Apply the canonical four-wheel native Chaos setup for a GTT fleet vehicle. */
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Chaos")
    static bool ConfigureCanonicalWheelSetups(UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary);

    /** Validate that a movement component is wired to the expected wheel classes and rig bones. */
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    static bool ValidateCanonicalWheelSetups(const UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary);
};
