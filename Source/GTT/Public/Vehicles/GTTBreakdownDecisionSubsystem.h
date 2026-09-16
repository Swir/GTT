#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTBreakdownDecisionSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;

UENUM(BlueprintType)
enum class EGTTBreakdownRecommendation : uint8 { DriveNormally, LimpToWorkshop, TowRecommended, Immobilized };

USTRUCT(BlueprintType)
struct GTT_API FGTTBreakdownAssessment
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Breakdown") EGTTBreakdownRecommendation Recommendation = EGTTBreakdownRecommendation::DriveNormally;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Breakdown") float Severity = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Breakdown") int32 RepairEstimate = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Breakdown") int32 TowEstimate = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Breakdown") bool bCanLimpHome = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Breakdown") bool bTowRecommended = false;
};

UCLASS()
class GTT_API UGTTBreakdownDecisionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Breakdown") FGTTBreakdownAssessment AssessVehicle(const AGTTRoadVehicleNativePawn* Vehicle, int32 BaseWorkshopCost = 75) const;
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Breakdown") int32 CalculateRepairEstimate(const AGTTRoadVehicleNativePawn* Vehicle, int32 BaseWorkshopCost = 75) const;
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Breakdown") int32 CalculateTowEstimate(const AGTTRoadVehicleNativePawn* Vehicle) const;
};
