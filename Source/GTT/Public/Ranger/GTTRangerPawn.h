#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTTRangerPawn.generated.h"

class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTRangerPawn : public ACharacter
{
    GENERATED_BODY()

public:
    AGTTRangerPawn();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Ranger")
    TObjectPtr<UStaticMeshComponent> HatMesh;
};
