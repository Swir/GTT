#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "GTTPoliceAIController.generated.h"
UCLASS()
class GTT_API AGTTPoliceAIController : public AAIController
{
    GENERATED_BODY()
public:
    AGTTPoliceAIController();
protected:
    virtual void BeginPlay() override;
    void UpdatePursuit();
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.1")) float RepathInterval = 0.65f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="25.0")) float AcceptanceRadius = 110.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0")) float BaseChaseSpeed = 520.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.0")) float SpeedPerWantedLevel = 55.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="50.0")) float ArrestRadius = 155.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.5")) float ArrestCooldown = 4.0f;
private:
    FTimerHandle PursuitTimer;
    float LastArrestAttemptTime = -1000.0f;
};
