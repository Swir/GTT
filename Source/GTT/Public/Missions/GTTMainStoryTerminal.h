#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTMainStoryTerminal.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGTTMainStoryTerminalType : uint8
{
    FarmOffice,
    NorthWood,
    VillageShop,
    Tavern,
    EastRoad,
    Workshop
};

UCLASS()
class GTT_API AGTTMainStoryTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTMainStoryTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Story") void SetTerminalType(EGTTMainStoryTerminalType NewType) { TerminalType = NewType; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Story") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Story") EGTTMainStoryTerminalType TerminalType = EGTTMainStoryTerminalType::FarmOffice;
};
