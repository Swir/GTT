#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTMainStorySave.generated.h"

UCLASS()
class GTT_API UGTTMainStorySave : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Story") int32 StorySaveVersion = 1;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Story") int32 StoryStage = 0;
};
