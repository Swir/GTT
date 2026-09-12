#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTNightFavorTerminal.generated.h"

class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGTTNightFavorTerminalType : uint8
{
    Tavern,
    Workshop,
    Neighbor
};

UCLASS()
class GTT_API AGTTNightFavorTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()
public:
    AGTTNightFavorTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    void SetTerminalType(EGTTNightFavorTerminalType InType) { TerminalType = InType; }

protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, Category="GTT|SideMission") EGTTNightFavorTerminalType TerminalType = EGTTNightFavorTerminalType::Tavern;
};
