#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTPrototypeWorld.generated.h"

class AStaticMeshActor;
class ATextRenderActor;

UCLASS()
class GTT_API AGTTPrototypeWorld : public AActor
{
    GENERATED_BODY()

public:
    AGTTPrototypeWorld();

protected:
    virtual void BeginPlay() override;

private:
    void BuildWorld();

    AStaticMeshActor* SpawnBox(
        const FVector& Location,
        const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator,
        bool bCollision = true);

    ATextRenderActor* SpawnLabel(
        const FString& Text,
        const FVector& Location,
        const FRotator& Rotation = FRotator(0.0f, 180.0f, 0.0f),
        float WorldSize = 85.0f);

    bool bWorldBuilt = false;
};
