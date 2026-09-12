#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTRuralEconomyTerminal.generated.h"

class UStaticMeshComponent;

enum class EGTTRuralEconomyTerminalType : uint8
{
    Fence,
    Insurance,
    Impound
};

UCLASS()
class GTT_API AGTTRuralEconomyTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTRuralEconomyTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    void SetTerminalType(EGTTRuralEconomyTerminalType NewType) { TerminalType = NewType; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|RuralEconomy")
    TObjectPtr<UStaticMeshComponent> Mesh;

private:
    EGTTRuralEconomyTerminalType TerminalType = EGTTRuralEconomyTerminalType::Fence;
};
