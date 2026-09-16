#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTFarmJobTerminal.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGTTFarmJobTerminalType : uint8
{
    Start,
    Pickup,
    Finish,
    FinalFinish
};

UCLASS()
class GTT_API AGTTFarmJobTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTFarmJobTerminal();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob")
    void SetTerminalType(EGTTFarmJobTerminalType InType) { TerminalType = InType; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob")
    EGTTFarmJobTerminalType GetTerminalType() const { return TerminalType; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|FarmJob")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|FarmJob")
    EGTTFarmJobTerminalType TerminalType = EGTTFarmJobTerminalType::Start;
};
