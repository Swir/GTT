#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTRoadsideRecoverySubsystem.generated.h"

class AGTTRoadVehicleNativePawn;

UENUM(BlueprintType)
enum class EGTTRoadsideRecoveryMode : uint8 { None, EmergencyPatch, RoadsideAssistance, PoliceImpound };

USTRUCT()
struct FGTTRoadsideRecoveryRuntime
{
    GENERATED_BODY()
    float StrandedSeconds = 0.0f;
    float CooldownSeconds = 0.0f;
    EGTTRoadsideRecoveryMode Mode = EGTTRoadsideRecoveryMode::None;
    bool bAnnounced = false;
    bool bTowRequested = false;
    bool bPatchRequested = false;
    int32 PendingTowQuote = 0;
    int32 PendingPatchQuote = 0;
    FName PendingPersistentVehicleId = NAME_None;
};

UCLASS()
class GTT_API UGTTRoadsideRecoverySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    /** Player-authorized roadside tow. Quote and exact target vehicle are locked when dispatch is accepted. */
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Recovery")
    bool RequestRoadsideTow(AGTTRoadVehicleNativePawn* Vehicle);

    /** Cheap temporary limp-home patch. Preserves body damage and never replaces workshop repair. */
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Recovery")
    bool RequestEmergencyRoadsidePatch(AGTTRoadVehicleNativePawn* Vehicle);

    /** Cancel a voluntary patch/tow before service arrives. No cash is charged until successful completion. */
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Recovery")
    bool CancelPendingRoadsideService(AGTTRoadVehicleNativePawn* Vehicle);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    bool IsRoadsideTowPending(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    bool IsRoadsidePatchPending(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    bool HasPendingRoadsideService(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    EGTTRoadsideRecoveryMode GetPendingRecoveryMode(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    int32 GetPendingRecoveryQuote(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    float GetPendingRecoverySecondsRemaining(const AGTTRoadVehicleNativePawn* Vehicle) const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    FName GetPendingRecoveryVehicleId(const AGTTRoadVehicleNativePawn* Vehicle) const;

private:
    bool IsRecoveryEligible(const AGTTRoadVehicleNativePawn* Vehicle) const;
    bool IsPlayerRecoveryChoiceEligible(const AGTTRoadVehicleNativePawn* Vehicle) const;
    void UpdateVehicle(AGTTRoadVehicleNativePawn* Vehicle, float DeltaSeconds);
    void ResetPendingService(FGTTRoadsideRecoveryRuntime& Runtime, bool bResetAnnouncement = true);
    bool IsPinnedVehicleValid(const AGTTRoadVehicleNativePawn* Vehicle, const FGTTRoadsideRecoveryRuntime& Runtime) const;
    bool CompleteRecovery(AGTTRoadVehicleNativePawn* Vehicle, EGTTRoadsideRecoveryMode Mode, int32 LockedTowQuote = 0, FName ExpectedVehicleId = NAME_None);
    bool CompleteEmergencyPatch(AGTTRoadVehicleNativePawn* Vehicle, int32 PatchQuote, FName ExpectedVehicleId);
    int32 CalculateRoadsideCost(const AGTTRoadVehicleNativePawn* Vehicle) const;
    int32 CalculateImpoundCost(const AGTTRoadVehicleNativePawn* Vehicle, int32 WantedLevel) const;
    FVector GetWorkshopDropLocation(const AGTTRoadVehicleNativePawn* Vehicle) const;
    TMap<TWeakObjectPtr<AGTTRoadVehicleNativePawn>, FGTTRoadsideRecoveryRuntime> RuntimeByVehicle;
    float ScanAccumulator = 0.0f;
};
