#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTRoadsideRecoverySubsystem.generated.h"

class AGTTRoadVehicleNativePawn;

UENUM(BlueprintType)
enum class EGTTRoadsideRecoveryMode : uint8 { None, RoadsideAssistance, PoliceImpound };

USTRUCT()
struct FGTTRoadsideRecoveryRuntime
{
    GENERATED_BODY()
    float StrandedSeconds = 0.0f;
    float CooldownSeconds = 0.0f;
    EGTTRoadsideRecoveryMode Mode = EGTTRoadsideRecoveryMode::None;
    bool bAnnounced = false;
    bool bTowRequested = false;
};

UCLASS()
class GTT_API UGTTRoadsideRecoverySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    /** Player-authorized roadside tow. Normal assistance never auto-tows at wanted 0. */
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Recovery")
    bool RequestRoadsideTow(AGTTRoadVehicleNativePawn* Vehicle);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Recovery")
    bool IsRoadsideTowPending(const AGTTRoadVehicleNativePawn* Vehicle) const;

private:
    bool IsRecoveryEligible(const AGTTRoadVehicleNativePawn* Vehicle) const;
    void UpdateVehicle(AGTTRoadVehicleNativePawn* Vehicle, float DeltaSeconds);
    bool CompleteRecovery(AGTTRoadVehicleNativePawn* Vehicle, EGTTRoadsideRecoveryMode Mode);
    int32 CalculateRoadsideCost(const AGTTRoadVehicleNativePawn* Vehicle) const;
    int32 CalculateImpoundCost(const AGTTRoadVehicleNativePawn* Vehicle, int32 WantedLevel) const;
    FVector GetWorkshopDropLocation(const AGTTRoadVehicleNativePawn* Vehicle) const;
    TMap<TWeakObjectPtr<AGTTRoadVehicleNativePawn>, FGTTRoadsideRecoveryRuntime> RuntimeByVehicle;
    float ScanAccumulator = 0.0f;
};