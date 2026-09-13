#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GTTChaosVehicleSpec.generated.h"

UENUM(BlueprintType)
enum class EGTTChaosDriveLayout : uint8
{
    RearWheelDrive,
    FrontWheelDrive,
    FourWheelDrive
};

USTRUCT(BlueprintType)
struct GTT_API FGTTChaosWheelSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float RadiusCm = 34.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float WidthCm = 24.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float SuspensionMaxRaiseCm = 12.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float SuspensionMaxDropCm = 18.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float SuspensionSpringRate = 250.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float SuspensionDampingRatio = 0.55f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float FrictionForceMultiplier = 2.7f;
};

USTRUCT(BlueprintType)
struct GTT_API FGTTChaosVehicleSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") FName VehicleId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float MassKg = 1600.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float EngineMaxTorqueNm = 420.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float EngineMaxRpm = 5200.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float EngineIdleRpm = 850.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float FinalDriveRatio = 3.7f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float MaxSteeringAngleDegrees = 34.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") EGTTChaosDriveLayout DriveLayout = EGTTChaosDriveLayout::RearWheelDrive;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") FGTTChaosWheelSpec FrontWheel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") FGTTChaosWheelSpec RearWheel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") TArray<float> ForwardGearRatios;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Chaos") float ReverseGearRatio = -3.1f;
};

UCLASS()
class GTT_API UGTTVehicleChaosSpecLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    static FGTTChaosVehicleSpec GetFieldmaster60Spec();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    static FGTTChaosVehicleSpec GetRattleback82Spec();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    static FGTTChaosVehicleSpec GetMulebox1200Spec();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Chaos")
    static bool GetSpecForVehicleId(FName VehicleId, FGTTChaosVehicleSpec& OutSpec);
};
