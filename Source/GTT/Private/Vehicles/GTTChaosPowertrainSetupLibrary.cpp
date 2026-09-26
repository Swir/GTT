#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Vehicles/GTTChaosVehicleSpec.h"

namespace
{
EVehicleDifferential ToChaosDifferential(EGTTChaosDriveLayout Layout)
{
    switch (Layout)
    {
        case EGTTChaosDriveLayout::FrontWheelDrive: return EVehicleDifferential::FrontWheelDrive;
        case EGTTChaosDriveLayout::FourWheelDrive: return EVehicleDifferential::AllWheelDrive;
        case EGTTChaosDriveLayout::RearWheelDrive:
        default: return EVehicleDifferential::RearWheelDrive;
    }
}

bool PowertrainNearlyEqual(float A, float B, float Tolerance = 0.01f)
{
    return FMath::IsNearlyEqual(A, B, Tolerance);
}
}

bool UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain(UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary)
{
    if (!Movement)
    {
        OutSummary = TEXT("No Chaos movement component");
        return false;
    }

    FGTTChaosVehicleSpec Spec;
    if (!UGTTVehicleChaosSpecLibrary::GetSpecForVehicleId(VehicleId, Spec))
    {
        OutSummary = FString::Printf(TEXT("Unknown vehicle id: %s"), *VehicleId.ToString());
        return false;
    }

    Movement->bMechanicalSimEnabled = true;
    Movement->EngineSetup.MaxTorque = Spec.EngineMaxTorqueNm;
    Movement->EngineSetup.MaxRPM = Spec.EngineMaxRpm;
    Movement->EngineSetup.EngineIdleRPM = Spec.EngineIdleRpm;
    Movement->EngineSetup.EngineBrakeEffect = 0.12f;
    Movement->EngineSetup.EngineRevUpMOI = 5.0f;
    Movement->EngineSetup.EngineRevDownRate = 600.0f;
    // Chaos disables the entire mechanical simulation when the authored curve is empty.
    // Runtime-created native vehicles therefore need a real RPM-domain torque curve,
    // even though MaxTorque and MaxRPM are configured separately.
    FRichCurve* TorqueCurve = Movement->EngineSetup.TorqueCurve.GetRichCurve();
    TorqueCurve->Reset();
    TorqueCurve->AddKey(0.0f, 0.55f);
    TorqueCurve->AddKey(Spec.EngineIdleRpm, 0.72f);
    TorqueCurve->AddKey(Spec.EngineMaxRpm * 0.45f, 1.0f);
    TorqueCurve->AddKey(Spec.EngineMaxRpm, 0.72f);

    Movement->TransmissionSetup.bUseAutomaticGears = true;
    Movement->TransmissionSetup.bUseAutoReverse = false;
    Movement->TransmissionSetup.FinalRatio = Spec.FinalDriveRatio;
    Movement->TransmissionSetup.ForwardGearRatios = Spec.ForwardGearRatios;
    Movement->TransmissionSetup.ReverseGearRatios = { Spec.ReverseGearRatio };
    Movement->TransmissionSetup.ChangeUpRPM = Spec.EngineMaxRpm * 0.82f;
    Movement->TransmissionSetup.ChangeDownRPM = Spec.EngineMaxRpm * 0.34f;
    Movement->TransmissionSetup.GearChangeTime = 0.24f;
    Movement->TransmissionSetup.TransmissionEfficiency = 0.94f;

    Movement->DifferentialSetup.DifferentialType = ToChaosDifferential(Spec.DriveLayout);
    Movement->DifferentialSetup.FrontRearSplit = Spec.DriveLayout == EGTTChaosDriveLayout::FourWheelDrive ? 0.48f : 0.50f;

    OutSummary = FString::Printf(TEXT("Canonical powertrain configured for %s: %.0f Nm / %.0f RPM / %dF+1R"),
        *VehicleId.ToString(), Spec.EngineMaxTorqueNm, Spec.EngineMaxRpm, Spec.ForwardGearRatios.Num());
    return true;
}

bool UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(const UChaosWheeledVehicleMovementComponent* Movement, FName VehicleId, FString& OutSummary)
{
    if (!Movement)
    {
        OutSummary = TEXT("No Chaos movement component");
        return false;
    }

    FGTTChaosVehicleSpec Spec;
    if (!UGTTVehicleChaosSpecLibrary::GetSpecForVehicleId(VehicleId, Spec))
    {
        OutSummary = FString::Printf(TEXT("Unknown vehicle id: %s"), *VehicleId.ToString());
        return false;
    }

    TArray<FString> Problems;
    if (!Movement->bMechanicalSimEnabled) Problems.Add(TEXT("mechanical-sim"));
    if (Movement->EngineSetup.TorqueCurve.GetRichCurveConst()->IsEmpty()) Problems.Add(TEXT("torque-curve"));
    if (!PowertrainNearlyEqual(Movement->EngineSetup.MaxTorque, Spec.EngineMaxTorqueNm)) Problems.Add(TEXT("engine-torque"));
    if (!PowertrainNearlyEqual(Movement->EngineSetup.MaxRPM, Spec.EngineMaxRpm)) Problems.Add(TEXT("engine-max-rpm"));
    if (!PowertrainNearlyEqual(Movement->EngineSetup.EngineIdleRPM, Spec.EngineIdleRpm)) Problems.Add(TEXT("engine-idle-rpm"));
    if (!PowertrainNearlyEqual(Movement->TransmissionSetup.FinalRatio, Spec.FinalDriveRatio)) Problems.Add(TEXT("final-drive"));
    if (Movement->TransmissionSetup.ForwardGearRatios.Num() != Spec.ForwardGearRatios.Num()) Problems.Add(TEXT("forward-gear-count"));
    else
    {
        for (int32 Index = 0; Index < Spec.ForwardGearRatios.Num(); ++Index)
        {
            if (!PowertrainNearlyEqual(Movement->TransmissionSetup.ForwardGearRatios[Index], Spec.ForwardGearRatios[Index]))
            {
                Problems.Add(FString::Printf(TEXT("forward-gear-%d"), Index + 1));
            }
        }
    }
    if (Movement->TransmissionSetup.ReverseGearRatios.Num() != 1 ||
        !PowertrainNearlyEqual(Movement->TransmissionSetup.ReverseGearRatios[0], Spec.ReverseGearRatio))
    {
        Problems.Add(TEXT("reverse-gear"));
    }
    if (Movement->DifferentialSetup.DifferentialType != ToChaosDifferential(Spec.DriveLayout)) Problems.Add(TEXT("differential-layout"));

    if (Problems.Num() > 0)
    {
        OutSummary = FString::Printf(TEXT("Invalid native powertrain: %s"), *FString::Join(Problems, TEXT(", ")));
        return false;
    }

    OutSummary = FString::Printf(TEXT("Canonical native powertrain valid for %s"), *VehicleId.ToString());
    return true;
}
