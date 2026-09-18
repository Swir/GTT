#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTServiceTerminal.generated.h"

class UStaticMeshComponent;
class AGTTRoadVehicleNativePawn;

enum class EGTTServiceType : uint8
{
    FishBuyer,
    Workshop
};

UCLASS()
class GTT_API AGTTServiceTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTServiceTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    void SetServiceType(EGTTServiceType NewType) { ServiceType = NewType; }
    EGTTServiceType GetServiceType() const { return ServiceType; }
    int32 GetNativeRoadRepairQuote(const AGTTRoadVehicleNativePawn* Vehicle) const;
    int32 GetNativeRoadFuelQuote(const AGTTRoadVehicleNativePawn* Vehicle) const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Service") TObjectPtr<UStaticMeshComponent> TerminalMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Service|Shop", meta=(ClampMin="1.0")) float FishPricePerKg = 24.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Service|Workshop", meta=(ClampMin="1")) int32 WorkshopServiceCost = 75;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Service|Workshop", meta=(ClampMin="0.1")) float NativeFuelPricePerLiter = 3.25f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Service|Workshop", meta=(ClampMin="100.0")) float VehicleSearchRadius = 900.0f;

private:
    class AGTTVehicleBase* FindNearestVehicle() const;
    EGTTServiceType ServiceType = EGTTServiceType::FishBuyer;
};
