#pragma once

#include "CoreMinimal.h"

struct GTT_API FGTTRoadNode
{
    FName Id = NAME_None;
    FString Label;
    FVector Location = FVector::ZeroVector;
    TArray<int32> Links;
};

class GTT_API FGTTRoadGraph
{
public:
    static const TArray<FGTTRoadNode>& GetNodes();
    static const TArray<FVector>& GetVillageLoop();
    static TArray<FVector> BuildRoute(int32 StartNodeIndex, int32 GoalNodeIndex);
    static int32 FindClosestNode(const FVector& Location);
    static FString GetNodeLabel(int32 NodeIndex);

private:
    static void BuildGraph(TArray<FGTTRoadNode>& OutNodes);
};
