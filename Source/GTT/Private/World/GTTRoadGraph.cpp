#include "World/GTTRoadGraph.h"

#include "Algo/Reverse.h"
#include "Containers/Queue.h"

namespace
{
void Link(TArray<FGTTRoadNode>& Nodes, int32 A, int32 B)
{
    if (!Nodes.IsValidIndex(A) || !Nodes.IsValidIndex(B)) return;
    Nodes[A].Links.AddUnique(B);
    Nodes[B].Links.AddUnique(A);
}
}

const TArray<FGTTRoadNode>& FGTTRoadGraph::GetNodes()
{
    static TArray<FGTTRoadNode> Nodes;
    if (Nodes.IsEmpty()) BuildGraph(Nodes);
    return Nodes;
}

const TArray<FVector>& FGTTRoadGraph::GetVillageLoop()
{
    static TArray<FVector> Loop = {
        FVector(-3300,-1800,100), FVector(0,-1800,100), FVector(3300,-1800,100), FVector(3300,0,100),
        FVector(3300,1800,100), FVector(0,1800,100), FVector(-3300,1800,100), FVector(-3300,0,100)
    };
    return Loop;
}

void FGTTRoadGraph::BuildGraph(TArray<FGTTRoadNode>& OutNodes)
{
    OutNodes = {
        {TEXT("WestSouth"), TEXT("WEST SOUTH JUNCTION"), FVector(-3300,-1800,100), 55.0f, 2, true, {}},
        {TEXT("VillageSouth"), TEXT("VILLAGE SOUTH"), FVector(0,-1800,100), 45.0f, 2, true, {}},
        {TEXT("PoliceSouth"), TEXT("POLICE SOUTH"), FVector(3300,-1800,100), 40.0f, 2, true, {}},
        {TEXT("EastCrossroad"), TEXT("EAST CROSSROAD"), FVector(3300,0,100), 55.0f, 2, true, {}},
        {TEXT("EastRoad"), TEXT("EAST ROAD"), FVector(3300,1800,100), 70.0f, 2, true, {}},
        {TEXT("VillageNorth"), TEXT("VILLAGE NORTH"), FVector(0,1800,100), 45.0f, 2, true, {}},
        {TEXT("FarmNorth"), TEXT("FARM NORTH"), FVector(-3300,1800,100), 50.0f, 2, false, {}},
        {TEXT("FarmCrossroad"), TEXT("FARM CROSSROAD"), FVector(-3300,0,100), 50.0f, 2, false, {}},
        {TEXT("NeighborBend"), TEXT("NEIGHBOR BEND"), FVector(4350,1900,100), 60.0f, 2, false, {}},
        {TEXT("HillFarmTurn"), TEXT("HILL FARM TURN"), FVector(5200,2550,100), 50.0f, 2, false, {}},
        {TEXT("ForestTrack"), TEXT("FOREST TRACK"), FVector(6100,650,100), 35.0f, 1, false, {}},
        {TEXT("NorthWoodTurn"), TEXT("NORTH WOOD TURN"), FVector(7350,650,100), 55.0f, 2, false, {}},
        {TEXT("PrivateLakeRoad"), TEXT("PRIVATE LAKE ROAD"), FVector(4700,-500,100), 45.0f, 1, false, {}},
        {TEXT("FeedDepotRoad"), TEXT("FEED DEPOT ROAD"), FVector(1850,3400,100), 55.0f, 2, false, {}},
        {TEXT("WorkshopRoad"), TEXT("WORKSHOP ROAD"), FVector(-650,2400,100), 40.0f, 2, false, {}},
        {TEXT("HillFarmNorth"), TEXT("HILL FARM NORTH"), FVector(5850,3100,100), 50.0f, 2, false, {}},
        {TEXT("ForestDeep"), TEXT("FOREST DEEP"), FVector(7050,-950,100), 30.0f, 1, false, {}},
        {TEXT("WardenOutpost"), TEXT("WARDEN OUTPOST"), FVector(5050,700,100), 40.0f, 2, true, {}},
        {TEXT("WoodYardGate"), TEXT("WOOD YARD GATE"), FVector(7850,900,100), 35.0f, 1, false, {}},
        {TEXT("LakeEast"), TEXT("LAKE EAST"), FVector(6100,-500,100), 55.0f, 2, false, {}},
        {TEXT("ScrapYardRoad"), TEXT("RUST DOGS SCRAP YARD ROAD"), FVector(-5050,-450,100), 45.0f, 1, false, {}},
        {TEXT("OldQuarryRoad"), TEXT("STONE CROWS OLD QUARRY ROAD"), FVector(8600,2850,100), 50.0f, 1, false, {}},
        {TEXT("MarshCampRoad"), TEXT("MUD JACKALS MARSH CAMP ROAD"), FVector(6900,-3300,100), 35.0f, 1, false, {}},
        {TEXT("NorthPassApproach"), TEXT("NORTH PASS APPROACH"), FVector(9300,2550,100), 60.0f, 2, true, {}},
        {TEXT("NorthPass"), TEXT("NORTH PASS CHECKPOINT"), FVector(10150,2550,100), 45.0f, 2, true, {}},
        {TEXT("RiverFord"), TEXT("RIVER FORD"), FVector(9600,1200,100), 30.0f, 1, false, {}},
        {TEXT("RidgeExchange"), TEXT("RIDGE EXCHANGE"), FVector(11200,800,100), 50.0f, 2, false, {}},
        {TEXT("QuarryNorth"), TEXT("QUARRY NORTH CUT"), FVector(9800,3350,100), 40.0f, 1, false, {}}
    };

    Link(OutNodes,0,1); Link(OutNodes,1,2); Link(OutNodes,2,3); Link(OutNodes,3,4);
    Link(OutNodes,4,5); Link(OutNodes,5,6); Link(OutNodes,6,7); Link(OutNodes,7,0);
    Link(OutNodes,4,8); Link(OutNodes,8,9); Link(OutNodes,9,15); Link(OutNodes,15,13); Link(OutNodes,13,5);
    Link(OutNodes,3,12); Link(OutNodes,12,19); Link(OutNodes,19,16); Link(OutNodes,16,10); Link(OutNodes,10,17);
    Link(OutNodes,17,11); Link(OutNodes,11,18); Link(OutNodes,18,15); Link(OutNodes,5,14); Link(OutNodes,14,6);
    Link(OutNodes,7,20); Link(OutNodes,11,21); Link(OutNodes,16,22); Link(OutNodes,19,22);
    Link(OutNodes,21,23); Link(OutNodes,23,24); Link(OutNodes,24,27); Link(OutNodes,24,25); Link(OutNodes,25,26); Link(OutNodes,26,27);
}

int32 FGTTRoadGraph::FindClosestNode(const FVector& Location)
{
    const TArray<FGTTRoadNode>& Nodes = GetNodes();
    int32 Best = INDEX_NONE;
    float BestDistSq = TNumericLimits<float>::Max();
    for (int32 Index=0; Index<Nodes.Num(); ++Index)
    {
        const float DistSq = FVector::DistSquared2D(Location, Nodes[Index].Location);
        if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = Index; }
    }
    return Best;
}

TArray<FVector> FGTTRoadGraph::BuildRoute(int32 StartNodeIndex, int32 GoalNodeIndex)
{
    const TArray<FGTTRoadNode>& Nodes = GetNodes();
    TArray<FVector> Result;
    if (!Nodes.IsValidIndex(StartNodeIndex) || !Nodes.IsValidIndex(GoalNodeIndex)) return Result;

    TArray<int32> Parent; Parent.Init(INDEX_NONE, Nodes.Num());
    TArray<bool> Visited; Visited.Init(false, Nodes.Num());
    TQueue<int32> Queue; Queue.Enqueue(StartNodeIndex); Visited[StartNodeIndex] = true;
    int32 Current = INDEX_NONE;
    while (Queue.Dequeue(Current))
    {
        if (Current == GoalNodeIndex) break;
        for (int32 Next : Nodes[Current].Links)
        {
            if (!Nodes.IsValidIndex(Next) || Visited[Next]) continue;
            Visited[Next] = true; Parent[Next] = Current; Queue.Enqueue(Next);
        }
    }
    if (!Visited[GoalNodeIndex]) return Result;

    TArray<int32> Chain;
    for (int32 Node = GoalNodeIndex; Node != INDEX_NONE; Node = Parent[Node]) Chain.Add(Node);
    Algo::Reverse(Chain);
    for (int32 Node : Chain) Result.Add(Nodes[Node].Location);
    return Result;
}

FString FGTTRoadGraph::GetNodeLabel(int32 NodeIndex)
{
    const TArray<FGTTRoadNode>& Nodes = GetNodes();
    return Nodes.IsValidIndex(NodeIndex) ? Nodes[NodeIndex].Label : TEXT("NONE");
}

float FGTTRoadGraph::GetSpeedLimitAtLocation(const FVector& Location)
{
    const int32 NodeIndex = FindClosestNode(Location);
    const TArray<FGTTRoadNode>& Nodes = GetNodes();
    return Nodes.IsValidIndex(NodeIndex) ? Nodes[NodeIndex].SpeedLimitKmh : 60.0f;
}

int32 FGTTRoadGraph::GetLaneCountAtLocation(const FVector& Location)
{
    const int32 NodeIndex = FindClosestNode(Location);
    const TArray<FGTTRoadNode>& Nodes = GetNodes();
    return Nodes.IsValidIndex(NodeIndex) ? Nodes[NodeIndex].LaneCount : 2;
}

bool FGTTRoadGraph::IsPriorityRoadAtLocation(const FVector& Location)
{
    const int32 NodeIndex = FindClosestNode(Location);
    const TArray<FGTTRoadNode>& Nodes = GetNodes();
    return Nodes.IsValidIndex(NodeIndex) && Nodes[NodeIndex].bPriorityRoad;
}
