#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTTerrainZone.generated.h"

class UBoxComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTVehicleTerrainType : uint8
{
    Dirt,
    Mud,
    DeepMud
};

UCLASS()
class GTT_API AGTTTerrainZone : public AActor
{
    GENERATED_BODY()

public:
    AGTTTerrainZone();

    UFUNCTION(BlueprintCallable, Category="GTT|Terrain")
    void ConfigureZone(EGTTVehicleTerrainType InTerrainType, const FVector& BoxExtent, const FString& Label);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Terrain")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Terrain")
    TObjectPtr<UTextRenderComponent> ZoneLabel;

private:
    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void ApplyToVehicle(class AGTTVehicleBase* Vehicle) const;

    EGTTVehicleTerrainType TerrainType = EGTTVehicleTerrainType::Dirt;
};
