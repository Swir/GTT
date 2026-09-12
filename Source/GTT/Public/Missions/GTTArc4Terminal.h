#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTArc4Terminal.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
UENUM(BlueprintType)
enum class EGTTArc4TerminalType : uint8 { FarmOffice, NorthPass, RidgeExchange };
UCLASS()
class GTT_API AGTTArc4Terminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()
public:
    AGTTArc4Terminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    void SetTerminalType(EGTTArc4TerminalType NewType);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Sign;
    EGTTArc4TerminalType TerminalType = EGTTArc4TerminalType::FarmOffice;
};
