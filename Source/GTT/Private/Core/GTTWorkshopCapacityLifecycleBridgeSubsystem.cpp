#include "Core/GTTWorkshopCapacityLifecycleBridgeSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"
#include "GTT.h"

namespace
{
    constexpr float CapacityScenarioStartGuardSeconds = 440.0f;
    constexpr float CompletionEpsilonHours = 0.02f;
}

void UGTTWorkshopCapacityLifecycleBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopCapacityRuntimeScenario"));
}

TStatId UGTTWorkshopCapacityLifecycleBridgeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopCapacityLifecycleBridgeSubsystem, STATGROUP_Tickables);
}

bool UGTTWorkshopCapacityLifecycleBridgeSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bAdvanced && World && World->IsGameWorld();
}

void UGTTWorkshopCapacityLifecycleBridgeSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < CapacityScenarioStartGuardSeconds) return;

    UWorld* World = GetWorld();
    if (!World) return;

    UGTTWorkshopRepairQueueSubsystem* Queue = World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>();
    if (!Queue) return;

    const TArray<FGTTWorkshopRepairQueueSnapshot> Entries = Queue->GetQueueSnapshots();
    if (Entries.Num() < 2) return;

    double LatestCompletionAbsoluteHours = -1.0;
    int32 CheckedInCount = 0;
    for (const FGTTWorkshopRepairQueueSnapshot& Entry : Entries)
    {
        if (!Entry.bCheckedIn || Entry.ServiceCompleteDay <= 0) continue;
        ++CheckedInCount;
        LatestCompletionAbsoluteHours = FMath::Max(
            LatestCompletionAbsoluteHours,
            static_cast<double>(Entry.ServiceCompleteDay) * 24.0 + Entry.ServiceCompleteHour);
    }
    if (CheckedInCount < 2 || LatestCompletionAbsoluteHours < 0.0) return;

    AGTTDayNightCycle* Clock = nullptr;
    for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Clock = *It;
            break;
        }
    }
    if (!Clock) return;

    LatestCompletionAbsoluteHours += CompletionEpsilonHours;
    const int32 TargetDay = FMath::Max(1, FMath::FloorToInt(LatestCompletionAbsoluteHours / 24.0));
    const float TargetHour = static_cast<float>(FMath::Fmod(LatestCompletionAbsoluteHours, 24.0));
    Clock->RestoreTime(TargetDay, TargetHour);
    bAdvanced = true;

    GTT_LOG( Display,
        TEXT("WORKSHOP_CAPACITY_LIFECYCLE_BRIDGE result=PASS checked_in=%d target_day=%d target_hour=%.2f reason=0.1.49_timed_service_compatibility"),
        CheckedInCount, TargetDay, TargetHour);
}
