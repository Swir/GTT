#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTVehicleIdentitySubsystem.generated.h"

class AGTTVehicleBase;

/**
 * Keeps legacy fleet PersistentVehicleId values unique before vehicles become owned/saved.
 *
 * Vehicle classes still provide a stable model-level default ID (for example Mulebox1200).
 * When two physical legacy actors of the same model coexist, the first stable actor keeps the
 * canonical ID and later unowned duplicates receive a deterministic actor-name suffix. Owned
 * vehicles are never renamed in place: once an ID is persisted it is treated as immutable.
 */
UCLASS()
class GTT_API UGTTVehicleIdentitySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Save")
    int32 RefreshFleetIdentity();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Save")
    int32 GetCollisionRepairCount() const { return CollisionRepairCount; }

private:
    static FName BuildUniqueInstanceId(const AGTTVehicleBase* Vehicle, FName BaseId, const TSet<FName>& UsedIds);

    TSet<TWeakObjectPtr<AGTTVehicleBase>> ObservedVehicles;
    float RefreshAccumulator = 0.0f;
    int32 CollisionRepairCount = 0;
};
