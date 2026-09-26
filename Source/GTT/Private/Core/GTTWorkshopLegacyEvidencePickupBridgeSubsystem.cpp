#include "Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.h"

#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"
#include "GTT.h"

namespace
{
constexpr float PriorityPickupEvidenceStartSeconds = 451.0f;
constexpr float LegacyBridgeStopMarginSeconds = 2.0f;
}

void UGTTWorkshopLegacyEvidencePickupBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    const bool bDemoSmoke = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    const bool bQueueEvidence = FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopQueueRuntimeScenario"));
    const bool bCapacityEvidence = FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopCapacityRuntimeScenario"));
    bPriorityPickupEvidence = FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopPriorityPickupRuntimeScenario"));
    bEnabled = bDemoSmoke && (bQueueEvidence || bCapacityEvidence);

    if (bEnabled)
    {
        GTT_LOG( Log,
            TEXT("WORKSHOP_LEGACY_EVIDENCE_PICKUP_BRIDGE enabled=YES production_pickup_api=YES economy_mutation=NO repair_mutation=NO priority_pickup_isolation=%s"),
            bPriorityPickupEvidence ? TEXT("YES") : TEXT("NO"));
    }
}

bool UGTTWorkshopLegacyEvidencePickupBridgeSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && World && World->IsGameWorld();
}

TStatId UGTTWorkshopLegacyEvidencePickupBridgeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopLegacyEvidencePickupBridgeSubsystem, STATGROUP_Tickables);
}

void UGTTWorkshopLegacyEvidencePickupBridgeSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (bPriorityPickupEvidence
        && Elapsed >= PriorityPickupEvidenceStartSeconds - LegacyBridgeStopMarginSeconds)
    {
        return;
    }

    TickAccumulator += DeltaTime;
    if (TickAccumulator < 0.10f) return;
    TickAccumulator = 0.0f;

    UWorld* World = GetWorld();
    UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    if (!Queue || !Queue->HasQueuedRepair()) return;

    const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
    for (const FGTTWorkshopRepairQueueSnapshot& Snapshot : Snapshots)
    {
        if (!Snapshot.bReadyForPickup || Snapshot.PersistentVehicleId.IsNone()) continue;

        FString Summary;
        const bool bReleased = Queue->ReleaseCompletedRepairForPickup(Snapshot.PersistentVehicleId, Summary);
        GTT_LOG( Log,
            TEXT("WORKSHOP_LEGACY_EVIDENCE_AUTO_PICKUP vehicle=%s result=%s paid_amount=%d exact_id=YES charged_again=NO repair_mutation=NO"),
            *Snapshot.PersistentVehicleId.ToString(), bReleased ? TEXT("PASS") : TEXT("FAIL"), Snapshot.PaidAmount);
    }
}
