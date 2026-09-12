#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTFieldCheckpoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTFieldCheckpoint : public AActor
{
    GENERATED_BODY()
public:
    AGTTFieldCheckpoint();
    void SetPassIndex(int32 InPassIndex) { PassIndex = InPassIndex; }

protected:
    UFUNCTION() void HandleOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Trigger;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> GateLeft;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> GateRight;
    UPROPERTY(EditAnywhere, Category="GTT|RuralWork") int32 PassIndex = 0;
};
