#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTSocialNPC.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGTTSocialRole : uint8
{
    Bartender,
    HallOrganizer,
    MechanicLocal,
    FarmerLocal
};

UCLASS()
class GTT_API AGTTSocialNPC : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTSocialNPC();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    void ConfigureSocialRole(EGTTSocialRole InRole, const FString& InDisplayName);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Social") TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Social") TObjectPtr<UStaticMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Social") TObjectPtr<UTextRenderComponent> NameText;

private:
    FString BuildConversation(AActor* Interactor);

    EGTTSocialRole Role = EGTTSocialRole::FarmerLocal;
    FString DisplayName = TEXT("Local");
    int32 ConversationIndex = 0;
};
