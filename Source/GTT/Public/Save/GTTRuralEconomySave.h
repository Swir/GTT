#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTRuralEconomySave.generated.h"

UCLASS()
class GTT_API UGTTRuralEconomySave : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() int32 SaveVersion = 1;
    UPROPERTY() int32 ContrabandUnits = 0;
    UPROPERTY() int32 ContrabandValue = 0;
    UPROPERTY() bool bInsuranceActive = false;
    UPROPERTY() FName ImpoundedVehicleId = NAME_None;
    UPROPERTY() int32 PendingImpoundFee = 0;
    UPROPERTY() int32 LifetimeFenceRevenue = 0;
    UPROPERTY() int32 SpeedingCitations = 0;
};
