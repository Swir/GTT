#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTBrawlTerminal.generated.h"

class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTBrawlTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()
public:
    AGTTBrawlTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
};
