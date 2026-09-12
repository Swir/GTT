#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTVillageEventMarker.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTVillageNightEventType : uint8
{
    BrokenDownNeighbor,
    TractorMeet,
    BonfireRun,
    LostCrate
};

UCLASS()
class GTT_API AGTTVillageEventMarker : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTVillageEventMarker();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    void ConfigureEvent(EGTTVillageNightEventType NewType);

    UFUNCTION(BlueprintPure, Category="GTT|Village Nights")
    FString GetEventTitle() const { return EventTitle; }

private:
    void RefreshPresentation();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MarkerMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;

    EGTTVillageNightEventType EventType = EGTTVillageNightEventType::BrokenDownNeighbor;
    FString EventTitle = TEXT("BROKEN-DOWN NEIGHBOR");
};
