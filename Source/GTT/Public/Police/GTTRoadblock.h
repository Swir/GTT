#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRoadblock.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTRoadblock : public AActor
{
    GENERATED_BODY()

public:
    AGTTRoadblock();

    UFUNCTION(BlueprintCallable, Category="GTT|Police|Roadblock")
    void SetResponseTier(int32 NewTier);

    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock")
    int32 GetResponseTier() const { return ResponseTier; }

private:
    UFUNCTION()
    void HandleSpikeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LeftBarrier;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> RightBarrier;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> SpikeStrip;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Sign;

    int32 ResponseTier = 1;
};
