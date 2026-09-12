#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTRecoveryTerminal.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTRecoveryTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTRecoveryTerminal();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Recovery")
    TObjectPtr<UBoxComponent> InteractionBounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Recovery")
    TObjectPtr<UStaticMeshComponent> TerminalMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Recovery")
    TObjectPtr<UTextRenderComponent> TerminalLabel;
};
