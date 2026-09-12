#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTTVehicleDynamicsComponent.generated.h"

class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct GTT_API FGTTVehicleDynamicsProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float WheelBaseCm = 245.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float TrackWidthCm = 165.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float SuspensionRestLengthCm = 52.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float WheelRadiusCm = 34.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float SpringStrength = 26.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float DamperStrength = 4.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float LateralGrip = 7.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float RollingResistance = 0.65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float MaxDriveForce = 1450.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float MaxSteerTorque = 105.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float MaxSpeedKmh = 92.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") float BrakeStrength = 4.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics", meta=(ClampMin="0.0", ClampMax="0.8")) float OffroadGripBias = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Dynamics") TArray<float> ForwardGearTopSpeedsKmh = {18.0f, 35.0f, 58.0f, 92.0f};
};

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTVehicleDynamicsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTVehicleDynamicsComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Dynamics") void ConfigureProfile(const FGTTVehicleDynamicsProfile& InProfile);
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Dynamics") void SetDriverInputs(float Throttle, float Steering);
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Dynamics") void SetPowerMultipliers(float EnginePower, float TireGrip);
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Dynamics") void ApplyTerrainModifier(float GripMultiplier, float ExtraRollingResistance, float DurationSeconds = 0.3f);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Dynamics") int32 GetCurrentGear() const { return CurrentGear; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Dynamics") int32 GetGroundContactCount() const { return GroundContactCount; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Dynamics") float GetSurfaceGripMultiplier() const { return SurfaceGripMultiplier; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Dynamics") float GetAverageSuspensionCompression() const { return AverageSuspensionCompression; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Dynamics") FString GetDynamicsSummary() const;

private:
    void ResolveChassis();
    void UpdateGear(float SpeedKmh);
    void ApplySuspensionAndGrip(float DeltaTime);
    void ApplyDrivetrain(float DeltaTime);

    UPROPERTY(EditAnywhere, Category="GTT|Vehicle|Dynamics") FGTTVehicleDynamicsProfile Profile;
    UPROPERTY() TObjectPtr<UPrimitiveComponent> Chassis;

    float ThrottleInput = 0.0f;
    float SteeringInput = 0.0f;
    float EnginePowerMultiplier = 1.0f;
    float TireGripMultiplier = 1.0f;
    float SurfaceGripMultiplier = 1.0f;
    float ExtraRollingResistance = 0.0f;
    float TerrainModifierTimeRemaining = 0.0f;
    int32 CurrentGear = 1;
    int32 GroundContactCount = 0;
    float AverageSuspensionCompression = 0.0f;
};
