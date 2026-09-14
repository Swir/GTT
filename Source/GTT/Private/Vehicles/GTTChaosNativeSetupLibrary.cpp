#include "Vehicles/GTTChaosNativeSetupLibrary.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTChaosVehicleSpec.h"
#include "Vehicles/GTTChaosVehicleWheels.h"

namespace
{
struct FGTTExpectedWheel
{
    TSubclassOf<UChaosVehicleWheel> WheelClass;
    FName BoneName;
    FGTTChaosWheelSpec WheelSpec;
    bool bSteering = false;
    bool bEngine = false;
    bool bHandbrake = false;
    float MaxSteerAngle = 0.0f;
};

bool NearlyEqual(float A, float B, float Tolerance = 0.05f)
{
    return FMath::Abs(A - B) <= Tolerance;
}

bool ValidateWheelDefaults(const FGTTExpectedWheel& Expected, int32 Index, TArray<FString>& Problems)
{
    const UChaosVehicleWheel* Wheel = Expected.WheelClass ? Cast<UChaosVehicleWheel>(Expected.WheelClass->GetDefaultObject()) : nullptr;
    if (!Wheel)
    {
        Problems.Add(FString::Printf(TEXT("wheel[%d]-missing-cdo"), Index));
        return false;
    }

    const FGTTChaosWheelSpec& Spec = Expected.WheelSpec;
    if (!NearlyEqual(Wheel->WheelRadius, Spec.RadiusCm)) Problems.Add(FString::Printf(TEXT("wheel[%d]-radius:%.1f!=%.1f"), Index, Wheel->WheelRadius, Spec.RadiusCm));
    if (!NearlyEqual(Wheel->WheelWidth, Spec.WidthCm)) Problems.Add(FString::Printf(TEXT("wheel[%d]-width:%.1f!=%.1f"), Index, Wheel->WheelWidth, Spec.WidthCm));
    if (!NearlyEqual(Wheel->SuspensionMaxRaise, Spec.SuspensionMaxRaiseCm)) Problems.Add(FString::Printf(TEXT("wheel[%d]-raise:%.1f!=%.1f"), Index, Wheel->SuspensionMaxRaise, Spec.SuspensionMaxRaiseCm));
    if (!NearlyEqual(Wheel->SuspensionMaxDrop, Spec.SuspensionMaxDropCm)) Problems.Add(FString::Printf(TEXT("wheel[%d]-drop:%.1f!=%.1f"), Index, Wheel->SuspensionMaxDrop, Spec.SuspensionMaxDropCm));
    if (!NearlyEqual(Wheel->SpringRate, Spec.SuspensionSpringRate, 0.5f)) Problems.Add(FString::Printf(TEXT("wheel[%d]-spring:%.1f!=%.1f"), Index, Wheel->SpringRate, Spec.SuspensionSpringRate));
    if (!NearlyEqual(Wheel->SuspensionDampingRatio, Spec.SuspensionDampingRatio)) Problems.Add(FString::Printf(TEXT("wheel[%d]-damping:%.2f!=%.2f"), Index, Wheel->SuspensionDampingRatio, Spec.SuspensionDampingRatio));
    if (!NearlyEqual(Wheel->FrictionForceMultiplier, Spec.FrictionForceMultiplier)) Problems.Add(FString::Printf(TEXT("wheel[%d]-friction:%.2f!=%.2f"), Index, Wheel->FrictionForceMultiplier, Spec.FrictionForceMultiplier));
    if (Wheel->bAffectedBySteering != Expected.bSteering) Problems.Add(FString::Printf(TEXT("wheel[%d]-steering"), Index));
    if (Wheel->bAffectedByEngine != Expected.bEngine) Problems.Add(FString::Printf(TEXT("wheel[%d]-engine"), Index));
    if (Wheel->bAffectedByHandbrake != Expected.bHandbrake) Problems.Add(FString::Printf(TEXT("wheel[%d]-handbrake"), Index));
    if (!Wheel->bAffectedByBrake) Problems.Add(FString::Printf(TEXT("wheel[%d]-brake-disabled"), Index));
    if (!NearlyEqual(Wheel->MaxSteerAngle, Expected.bSteering ? Expected.MaxSteerAngle : 0.0f)) Problems.Add(FString::Printf(TEXT("wheel[%d]-steer-angle:%.1f"), Index, Wheel->MaxSteerAngle));
    if (Wheel->bABSEnabled) Problems.Add(FString::Printf(TEXT("wheel[%d]-unexpected-abs"), Index));
    if (Wheel->bTractionControlEnabled) Problems.Add(FString::Printf(TEXT("wheel[%d]-unexpected-tc"), Index));
    return true;
}

bool BuildExpectedWheels(FName VehicleId, TArray<FGTTExpectedWheel>& OutExpected)
{
    FGTTChaosRigContract Rig;
    FGTTChaosVehicleSpec Spec;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(VehicleId, Rig) || !UGTTVehicleChaosSpecLibrary::GetSpecForVehicleId(VehicleId, Spec))
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
        { FrontClass, Rig.FrontLeftWheelBone, Spec.FrontWheel, true, false, false, Spec.MaxSteeringAngleDegrees },
        { FrontClass, Rig.FrontRightWheelBone, Spec.FrontWheel, true, false, false, Spec.MaxSteeringAngleDegrees },
        { RearClass, Rig.RearLeftWheelBone, Spec.RearWheel, false, true, true, 0.0f },
        { RearClass, Rig.RearRightWheelBone, Spec.RearWheel, false, true, true, 0.0f }
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

    OutSummary = FString::Printf(TEXT("Configured 4 canonical authored Chaos wheels for %s"), *VehicleId.ToString());
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
        if (!Actual.AdditionalOffset.IsNearlyZero())
        {
            Problems.Add(FString::Printf(TEXT("wheel[%d]-offset"), Index));
        }
        ValidateWheelDefaults(Wanted, Index, Problems);
    }

    if (Problems.Num() > 0)
    {
        OutSummary = FString::Printf(TEXT("Invalid native authored wheel setup: %s"), *FString::Join(Problems, TEXT(", ")));
        return false;
    }

    const UChaosVehicleWheel* FrontWheel = Cast<UChaosVehicleWheel>(Expected[0].WheelClass->GetDefaultObject());
    const UChaosVehicleWheel* RearWheel = Cast<UChaosVehicleWheel>(Expected[2].WheelClass->GetDefaultObject());
    OutSummary = FString::Printf(
        TEXT("Canonical authored wheel setup valid for %s front[r=%.1f raise=%.1f drop=%.1f spring=%.1f damping=%.2f] rear[r=%.1f raise=%.1f drop=%.1f spring=%.1f damping=%.2f]"),
        *VehicleId.ToString(),
        FrontWheel ? FrontWheel->WheelRadius : -1.0f,
        FrontWheel ? FrontWheel->SuspensionMaxRaise : -1.0f,
        FrontWheel ? FrontWheel->SuspensionMaxDrop : -1.0f,
        FrontWheel ? FrontWheel->SpringRate : -1.0f,
        FrontWheel ? FrontWheel->SuspensionDampingRatio : -1.0f,
        RearWheel ? RearWheel->WheelRadius : -1.0f,
        RearWheel ? RearWheel->SuspensionMaxRaise : -1.0f,
        RearWheel ? RearWheel->SuspensionMaxDrop : -1.0f,
        RearWheel ? RearWheel->SpringRate : -1.0f,
        RearWheel ? RearWheel->SuspensionDampingRatio : -1.0f);
    return true;
}
