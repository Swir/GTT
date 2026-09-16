#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTContractBoardTerminal.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTContractBoardTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTContractBoardTerminal();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Contracts")
    void Configure(FName InJobTag);

private:
    void RefreshLabel();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY(Transient)
    FName JobTag = NAME_None;

    float RefreshClock = 0.0f;
};
