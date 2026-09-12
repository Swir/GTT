#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTArc3Terminal.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTArc3TerminalType : uint8
{
    FarmOffice,
    RedBarn,
    CountyDrop
};

UCLASS()
class GTT_API AGTTArc3Terminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTArc3Terminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Story|Arc3")
    void SetTerminalType(EGTTArc3TerminalType NewType);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Story|Arc3") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Story|Arc3") TObjectPtr<UTextRenderComponent> Sign;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Story|Arc3") EGTTArc3TerminalType TerminalType = EGTTArc3TerminalType::FarmOffice;
};
