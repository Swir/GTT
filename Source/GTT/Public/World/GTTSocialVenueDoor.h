#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTSocialVenueDoor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTSocialVenueDoor : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTSocialVenueDoor();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    void ConfigureDoor(const FString& InVenueName, const FVector& InDestination, const FRotator& InDestinationRotation, bool bInInteriorExit);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Social") TObjectPtr<UStaticMeshComponent> DoorMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Social") TObjectPtr<UTextRenderComponent> SignText;

private:
    FString VenueName = TEXT("Venue");
    FVector Destination = FVector::ZeroVector;
    FRotator DestinationRotation = FRotator::ZeroRotator;
    bool bInteriorExit = false;
};
