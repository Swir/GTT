#include "Vehicles/GTTVehicleIdentitySubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace
{
constexpr float IdentityRefreshIntervalSeconds = 1.0f;
}

TStatId UGTTVehicleIdentitySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTVehicleIdentitySubsystem, STATGROUP_Tickables);
}

bool UGTTVehicleIdentitySubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

void UGTTVehicleIdentitySubsystem::Tick(float DeltaTime)
{
    RefreshAccumulator += DeltaTime;
    if (RefreshAccumulator < IdentityRefreshIntervalSeconds) return;
    RefreshAccumulator = 0.0f;
    RefreshFleetIdentity();
}

FName UGTTVehicleIdentitySubsystem::BuildUniqueInstanceId(
    const AGTTVehicleBase* Vehicle,
    FName BaseId,
    const TSet<FName>& UsedIds)
{
    if (!Vehicle || BaseId.IsNone()) return NAME_None;

    FString ActorToken = Vehicle->GetName();
    for (int32 Index = 0; Index < ActorToken.Len(); ++Index)
    {
        TCHAR& Ch = ActorToken[Index];
        if (!FChar::IsAlnum(Ch) && Ch != TCHAR('_')) Ch = TCHAR('_');
    }
    if (ActorToken.IsEmpty()) ActorToken = TEXT("instance");

    const FString Base = BaseId.ToString();
    FName Candidate(*FString::Printf(TEXT("%s__%s"), *Base, *ActorToken));
    int32 Suffix = 2;
    while (UsedIds.Contains(Candidate))
    {
        Candidate = FName(*FString::Printf(TEXT("%s__%s__%d"), *Base, *ActorToken, Suffix++));
    }
    return Candidate;
}

int32 UGTTVehicleIdentitySubsystem::RefreshFleetIdentity()
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return 0;

    for (auto It = ObservedVehicles.CreateIterator(); It; ++It)
    {
        if (!(*It).IsValid()) It.RemoveCurrent();
    }

    TArray<AGTTVehicleBase*> Vehicles;
    for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
    {
        if (AGTTVehicleBase* Vehicle = *It) Vehicles.Add(Vehicle);
    }
    Vehicles.Sort([](const AGTTVehicleBase& A, const AGTTVehicleBase& B)
    {
        return A.GetPathName() < B.GetPathName();
    });

    TSet<FName> UsedIds;

    // Persisted/owned IDs are immutable. Reserve them before handling any new actor so a newly
    // spawned duplicate can never steal an ID that may already exist in a SaveGame.
    for (AGTTVehicleBase* Vehicle : Vehicles)
    {
        if (!Vehicle || !Vehicle->IsOwnedByPlayer()) continue;
        const FName Id = Vehicle->GetPersistentVehicleId();
        if (Id.IsNone()) continue;
        if (UsedIds.Contains(Id))
        {
            UE_LOG(LogGTT, Error,
                TEXT("VEHICLE_IDENTITY event=OWNED_COLLISION result=UNRESOLVED vehicle=%s actor=%s reason=persisted_id_is_immutable"),
                *Id.ToString(), *Vehicle->GetName());
        }
        else
        {
            UsedIds.Add(Id);
        }
        ObservedVehicles.Add(TWeakObjectPtr<AGTTVehicleBase>(Vehicle));
    }

    // Existing observations retain their IDs. This prevents a later spawn whose actor name sorts
    // earlier from renaming a vehicle that has already participated in gameplay this session.
    for (AGTTVehicleBase* Vehicle : Vehicles)
    {
        if (!Vehicle || Vehicle->IsOwnedByPlayer()) continue;
        const TWeakObjectPtr<AGTTVehicleBase> WeakVehicle(Vehicle);
        if (!ObservedVehicles.Contains(WeakVehicle)) continue;
        const FName Id = Vehicle->GetPersistentVehicleId();
        if (!Id.IsNone()) UsedIds.Add(Id);
    }

    int32 RepairedThisPass = 0;
    for (AGTTVehicleBase* Vehicle : Vehicles)
    {
        if (!Vehicle || Vehicle->IsOwnedByPlayer()) continue;
        const TWeakObjectPtr<AGTTVehicleBase> WeakVehicle(Vehicle);
        if (ObservedVehicles.Contains(WeakVehicle)) continue;

        const FName BaseId = Vehicle->GetPersistentVehicleId();
        if (BaseId.IsNone())
        {
            UE_LOG(LogGTT, Warning,
                TEXT("VEHICLE_IDENTITY event=OBSERVE result=SKIP actor=%s reason=missing_persistent_id"),
                *Vehicle->GetName());
            ObservedVehicles.Add(WeakVehicle);
            continue;
        }

        if (!UsedIds.Contains(BaseId))
        {
            UsedIds.Add(BaseId);
            ObservedVehicles.Add(WeakVehicle);
            continue;
        }

        const FName UniqueId = BuildUniqueInstanceId(Vehicle, BaseId, UsedIds);
        if (UniqueId.IsNone() || !Vehicle->AssignPersistentVehicleIdForInstance(UniqueId))
        {
            UE_LOG(LogGTT, Error,
                TEXT("VEHICLE_IDENTITY event=ASSIGN result=FAIL base=%s actor=%s"),
                *BaseId.ToString(), *Vehicle->GetName());
            ObservedVehicles.Add(WeakVehicle);
            continue;
        }

        UsedIds.Add(UniqueId);
        ObservedVehicles.Add(WeakVehicle);
        ++CollisionRepairCount;
        ++RepairedThisPass;
        UE_LOG(LogGTT, Display,
            TEXT("VEHICLE_IDENTITY event=ASSIGN result=PASS base=%s unique=%s actor=%s"),
            *BaseId.ToString(), *UniqueId.ToString(), *Vehicle->GetName());
    }

    return RepairedThisPass;
}
