#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GTTInteractable.generated.h"

UINTERFACE(BlueprintType)
class GTT_API UGTTInteractable : public UInterface
{
    GENERATED_BODY()
};

class GTT_API IGTTInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GTT|Interaction")
    void Interact(AActor* Interactor);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="GTT|Interaction")
    FText GetInteractionText() const;
};
