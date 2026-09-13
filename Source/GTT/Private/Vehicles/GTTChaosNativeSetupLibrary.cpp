#include "Vehicles/GTTChaosNativeSetupLibrary.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTChaosVehicleWheels.h"

namespace
{
struct FGTTExpectedWheel
{
    TSubclassOf<UChaosVehicleWheel> WheelClass;
    FName BoneName;
};

bool BuildExpectedWheels(FName VehicleId, TArray<FGTTExpectedWheel>& OutExpected)
{
    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(VehicleId, Rig))
    {
        return false;
    }

    TSubclassOf<UChaosVehicleWheel> FrontClass;
    TSubclassOf<UChaosVehicleWheel> RearClass;

    if (VehicleId == TEXT("RustyFieldmaster60"))
    {
        FrontClass = UGTTFieldmasterFrontWheel::StaticClass();
        RearClass = UGTTFieldmasterRearWheel::StaticClass();
    }
    else if (VehicleId == TEXT("Rattleback82"))
    {
        FrontClass = UGTTRattlebackFrontWheel::StaticClass();
        RearClass = UGTTRattlebackRearWheel::StaticClass();
    }
    else if (VehicleId == TEXT("Mulebox1200"))
    {
        FrontClass = UGTTMuleboxFrontWheel::StaticClass();
        RearClass = UGTTMuleboxRearWheel::StaticClass();
    }
    else
    {
        return false;
    }

    OutExpected = {
        { FrontClass, Rig.FrontLeftWheelBone },
        { FrontClass, Rig.FrontRightWheelBone },
        { RearClass, Rig.RearLeftWheelBone },
        { RearClass, Rig.RearRightWheelBone }
    };
    return true;
}
}

bool UGTTChaosNativeSetupLibrary::ConfigureCanonicalWheelSetups(UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary)
{
    if (!Movement)
    {
        OutSummary = TEXT("No Chaos movement component");
        return false;
    }

    TArray<FGTTExpectedWheel> Expected;
    if (!BuildExpectedWheels(VehicleId, Expected))
    {
        OutSummary = FString::Printf(TEXT("Unknown vehicle id: %s"), *VehicleId.ToString());
        return false;
    }

    Movement->bMechanicalSimEnabled = true;
    Movement->WheelSetups.Reset(Expected.Num());
    for (const FGTTExpectedWheel& Wheel : Expected)
    {
        FChaosWheelSetup Setup;
        Setup.WheelClass = Wheel.WheelClass;
        Setup.BoneName = Wheel.BoneName;
        Setup.AdditionalOffset = FVector::ZeroVector;
        Movement->WheelSetups.Add(Setup);
    }

    OutSummary = FString::Printf(TEXT("Configured 4 canonical Chaos wheels for %s"), *VehicleId.ToString());
    return true;
}

bool UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(const UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary)
{
    if (!Movement)
    {
        OutSummary = TEXT("No Chaos movement component");
        return false;
    }

    TArray<FGTTExpectedWheel> Expected;
    if (!BuildExpectedWheels(VehicleId, Expected))
    {
        OutSummary = FString::Printf(TEXT("Unknown vehicle id: %s"), *VehicleId.ToString());
        return false;
    }

    if (!Movement->bMechanicalSimEnabled)
    {
        OutSummary = TEXT("Mechanical simulation disabled");
        return false;
    }

    if (Movement->WheelSetups.Num() != Expected.Num())
    {
        OutSummary = FString::Printf(TEXT("Expected 4 wheel setups, found %d"), Movement->WheelSetups.Num());
        return false;
    }

    TArray<FString> Problems;
    for (int32 Index = 0; Index < Expected.Num(); ++Index)
    {
        const FChaosWheelSetup& Actual = Movement->WheelSetups[Index];
        const FGTTExpectedWheel& Wanted = Expected[Index];
        if (Actual.WheelClass != Wanted.WheelClass)
        {
            Problems.Add(FString::Printf(TEXT("wheel[%d]-class"), Index));
        }
        if (Actual.BoneName != Wanted.BoneName)
        {
            Problems.Add(FString::Printf(TEXT("wheel[%d]-bone:%s"), Index, *Actual.BoneName.ToString()));
        }
    }

    if (Problems.Num() > 0)
    {
        OutSummary = FString::Printf(TEXT("Invalid native setup: %s"), *FString::Join(Problems, TEXT(", ")));
        return false;
    }

    OutSummary = FString::Printf(TEXT("Canonical native wheel setup valid for %s"), *VehicleId.ToString());
    return true;
}
