#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "GTTFieldmasterNativePawn.generated.h"

class UChaosWheeledVehicleMovementComponent;

UCLASS(Blueprintable)
class GTT_API AGTTFieldmasterNativePawn : public AWheeledVehiclePawn
{
    GENERATED_BODY()

public:
    AGTTFieldmasterNativePawn();

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos")
    bool ConfigureAndValidateNativeFieldmaster(FString& OutSummary);

    UFUNCTION(BlueprintPure, Category="GTT|Chaos")
    bool IsNativeFieldmasterReady() const { return bNativeReady; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos")
    FString GetNativeAcceptanceSummary() const { return NativeAcceptanceSummary; }

protected:
    virtual void BeginPlay() override;

private:
    bool ValidateRigContract(FString& OutSummary) const;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos")
    bool bNativeReady = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos")
    FString NativeAcceptanceSummary = TEXT("Not validated");
};
