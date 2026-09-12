#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GTTRangerAIController.generated.h"

UCLASS()
class GTT_API AGTTRangerAIController : public AAIController
{
    GENERATED_BODY()

public:
    AGTTRangerAIController();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.2"))
    float RepathInterval = 0.75f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="25.0"))
    float AcceptanceRadius = 120.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="50.0"))
    float CitationRadius = 155.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="100.0"))
    float BaseChaseSpeed = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.0"))
    float SpeedPerAlertLevel = 45.0f;

private:
    void UpdatePursuit();
    FTimerHandle PursuitTimer;
};
