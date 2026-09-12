#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTHeavyHaulTerminal.generated.h"

class AGTTHeavyHaulDirector;
class UBoxComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTHeavyHaulTerminalType : uint8
{
    ContractBoard,
    Hitch,
    Load,
    Deliver
};

UCLASS()
class GTT_API AGTTHeavyHaulTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTHeavyHaulTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    void Configure(EGTTHeavyHaulTerminalType InType, AGTTHeavyHaulDirector* InDirector);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY()
    TObjectPtr<AGTTHeavyHaulDirector> Director;

    EGTTHeavyHaulTerminalType TerminalType = EGTTHeavyHaulTerminalType::ContractBoard;
};
