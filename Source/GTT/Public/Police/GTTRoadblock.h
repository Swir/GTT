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

    UFUNCTION(BlueprintCallable, Category="GTT|Police|Roadblock") void SetResponseTier(int32 NewTier);
    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock") int32 GetResponseTier() const { return ResponseTier; }
    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock") int32 GetSpikeHitCount() const { return SpikeHitCount; }
    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock") FName GetLastSpikedVehicleId() const { return LastSpikedVehicleId; }
    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock") float GetLastTireIntegrityBefore() const { return LastTireIntegrityBefore; }
    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock") float GetLastTireIntegrityAfter() const { return LastTireIntegrityAfter; }
    UFUNCTION(BlueprintPure, Category="GTT|Police|Roadblock") bool HasProvenSpikeConsequence() const { return SpikeHitCount > 0 && LastTireIntegrityAfter < LastTireIntegrityBefore; }

private:
    UFUNCTION()
    void HandleSpikeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LeftBarrier;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RightBarrier;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SpikeStrip;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Sign;

    int32 ResponseTier = 1;
    int32 SpikeHitCount = 0;
    FName LastSpikedVehicleId = NAME_None;
    float LastTireIntegrityBefore = 1.0f;
    float LastTireIntegrityAfter = 1.0f;
    TWeakObjectPtr<AActor> LastSpikedActor;
    float LastSpikeHitTimeSeconds = -1000.0f;
};
