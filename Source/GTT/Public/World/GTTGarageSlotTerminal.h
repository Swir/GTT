#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTGarageSlotTerminal.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class AGTTVehicleBase;

UCLASS()
class GTT_API AGTTGarageSlotTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTGarageSlotTerminal();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Garage")
    void SetSlotIndex(int32 InSlotIndex);

    UFUNCTION(BlueprintPure, Category="GTT|Garage")
    int32 GetSlotIndex() const { return SlotIndex; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Garage")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Garage")
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Garage", meta=(ClampMin="0"))
    int32 RecallServiceCost = 15;

private:
    AGTTVehicleBase* ResolveSlotVehicle() const;
    void RefreshLabel();

    int32 SlotIndex = 0;
    float RefreshClock = 0.0f;
};
