#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GTTGameplayStatics.generated.h"

class APawn;
class UGTTWantedComponent;
class UGTTPlayerEconomyComponent;
class UGTTRadioComponent;

UCLASS()
class GTT_API UGTTGameplayStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="GTT|Wanted")
    static UGTTWantedComponent* FindWantedComponentForPawn(APawn* Pawn);

    UFUNCTION(BlueprintPure, Category="GTT|Economy")
    static UGTTPlayerEconomyComponent* FindEconomyComponentForPawn(APawn* Pawn);

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    static UGTTRadioComponent* FindRadioComponentForPawn(APawn* Pawn);

    UFUNCTION(BlueprintPure, Category="GTT|Wanted", meta=(WorldContext="WorldContextObject"))
    static int32 GetPlayerWantedLevel(const UObject* WorldContextObject, int32 PlayerIndex = 0);
};
