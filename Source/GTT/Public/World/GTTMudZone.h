#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTMudZone.generated.h"

class UBoxComponent;

UCLASS()
class GTT_API AGTTMudZone : public AActor
{
    GENERATED_BODY()

public:
    AGTTMudZone();
    virtual void Tick(float DeltaSeconds) override;

    void Configure(const FVector& HalfExtent, float InDragStrength, float InDamagePerSecond = 0.0f);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> MudVolume;

    UPROPERTY(EditAnywhere, Category="GTT|Mud")
    float DragStrength = 2.4f;

    UPROPERTY(EditAnywhere, Category="GTT|Mud")
    float TireWearPerSecond = 0.008f;
};
