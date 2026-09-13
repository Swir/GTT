#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTVehicleStyleTerminal.generated.h"

class AGTTVehicleBase;
class UStaticMeshComponent;

enum class EGTTVehicleStyleService : uint8
{
    TractorVisual,
    OldCarVariant,
    BodyPanels
};

UCLASS()
class GTT_API AGTTVehicleStyleTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTVehicleStyleTerminal();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    void SetServiceType(EGTTVehicleStyleService InServiceType) { ServiceType = InServiceType; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|VehicleStyle")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|VehicleStyle", meta=(ClampMin="100.0"))
    float VehicleSearchRadius = 950.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|VehicleStyle")
    int32 VisualPackageBaseCost = 220;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|VehicleStyle")
    int32 PanelReplacementCostPerPart = 55;

private:
    AGTTVehicleBase* FindNearestOwnedVehicle() const;
    bool ApplyTractorPackage(AGTTVehicleBase* Vehicle, class UGTTPlayerEconomyComponent* Economy);
    bool ApplyOldCarVariant(AGTTVehicleBase* Vehicle, class UGTTPlayerEconomyComponent* Economy);
    bool ReplaceBodyPanels(AGTTVehicleBase* Vehicle, class UGTTPlayerEconomyComponent* Economy);
    void RebuildVisualPackage(AGTTVehicleBase* Vehicle) const;
    UStaticMeshComponent* AddVisualPart(AGTTVehicleBase* Vehicle, FName Name, const FVector& Location, const FVector& Scale) const;

    EGTTVehicleStyleService ServiceType = EGTTVehicleStyleService::TractorVisual;
};
