#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTRuralWorkTerminal.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGTTRuralWorkTerminalType : uint8
{
    TimberStart,
    TimberPickup,
    TimberFinish,
    MowingStart
};

UCLASS()
class GTT_API AGTTRuralWorkTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()
public:
    AGTTRuralWorkTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    void SetTerminalType(EGTTRuralWorkTerminalType InType) { TerminalType = InType; }

protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, Category="GTT|RuralWork") EGTTRuralWorkTerminalType TerminalType = EGTTRuralWorkTerminalType::TimberStart;
};
