#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTRecoveryTerminal.generated.h"

class AGTTRecoveryDirector;
class UBoxComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTRecoveryTerminalType : uint8
{
    Workshop,
    Hook
};

UCLASS()
class GTT_API AGTTRecoveryTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTRecoveryTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    void Configure(EGTTRecoveryTerminalType InType, AGTTRecoveryDirector* InDirector);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> Trigger;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY()
    TObjectPtr<AGTTRecoveryDirector> Director;
    EGTTRecoveryTerminalType TerminalType = EGTTRecoveryTerminalType::Workshop;
};
