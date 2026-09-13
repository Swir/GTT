#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GTTChaosRigContract.generated.h"

USTRUCT(BlueprintType)
struct GTT_API FGTTChaosRigContract
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName VehicleId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName RootBone = TEXT("root");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName FrontLeftWheelBone = TEXT("wheel_fl");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName FrontRightWheelBone = TEXT("wheel_fr");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName RearLeftWheelBone = TEXT("wheel_rl");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName RearRightWheelBone = TEXT("wheel_rr");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName DriverSocket = TEXT("driver_seat");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName ExitSocket = TEXT("driver_exit");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") FName HitchSocket = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|ChaosRig") bool bRequiresHitchSocket = false;
};

UCLASS()
class GTT_API UGTTChaosRigContractLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|ChaosRig")
    static FGTTChaosRigContract GetFieldmaster60Rig();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|ChaosRig")
    static FGTTChaosRigContract GetRattleback82Rig();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|ChaosRig")
    static FGTTChaosRigContract GetMulebox1200Rig();

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|ChaosRig")
    static bool GetRigForVehicleId(FName VehicleId, FGTTChaosRigContract& OutRig);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|ChaosRig")
    static TArray<FName> GetRequiredBoneNames(const FGTTChaosRigContract& Rig);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|ChaosRig")
    static TArray<FName> GetRequiredSocketNames(const FGTTChaosRigContract& Rig);
};
