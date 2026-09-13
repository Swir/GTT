#include "Vehicles/GTTChaosVehicleSpec.h"

namespace
{
    FGTTChaosWheelSpec MakeWheel(float Radius, float Width, float Raise, float Drop, float Spring, float Damping, float Friction)
    {
        FGTTChaosWheelSpec Wheel;
        Wheel.RadiusCm = Radius;
        Wheel.WidthCm = Width;
        Wheel.SuspensionMaxRaiseCm = Raise;
        Wheel.SuspensionMaxDropCm = Drop;
        Wheel.SuspensionSpringRate = Spring;
        Wheel.SuspensionDampingRatio = Damping;
        Wheel.FrictionForceMultiplier = Friction;
        return Wheel;
    }
}

FGTTChaosVehicleSpec UGTTVehicleChaosSpecLibrary::GetFieldmaster60Spec()
{
    FGTTChaosVehicleSpec Spec;
    Spec.VehicleId = TEXT("Fieldmaster60");
    Spec.MassKg = 2850.0f;
    Spec.EngineMaxTorqueNm = 680.0f;
    Spec.EngineMaxRpm = 2600.0f;
    Spec.EngineIdleRpm = 720.0f;
    Spec.FinalDriveRatio = 5.2f;
    Spec.MaxSteeringAngleDegrees = 38.0f;
    Spec.DriveLayout = EGTTChaosDriveLayout::FourWheelDrive;
    Spec.FrontWheel = MakeWheel(46.0f, 27.0f, 15.0f, 24.0f, 310.0f, 0.62f, 3.05f);
    Spec.RearWheel = MakeWheel(68.0f, 38.0f, 12.0f, 22.0f, 365.0f, 0.68f, 3.35f);
    Spec.ForwardGearRatios = {5.10f, 3.20f, 2.05f, 1.35f, 0.92f};
    Spec.ReverseGearRatio = -4.75f;
    return Spec;
}

FGTTChaosVehicleSpec UGTTVehicleChaosSpecLibrary::GetRattleback82Spec()
{
    FGTTChaosVehicleSpec Spec;
    Spec.VehicleId = TEXT("Rattleback82");
    Spec.MassKg = 1460.0f;
    Spec.EngineMaxTorqueNm = 355.0f;
    Spec.EngineMaxRpm = 5600.0f;
    Spec.EngineIdleRpm = 850.0f;
    Spec.FinalDriveRatio = 3.73f;
    Spec.MaxSteeringAngleDegrees = 34.0f;
    Spec.DriveLayout = EGTTChaosDriveLayout::RearWheelDrive;
    Spec.FrontWheel = MakeWheel(34.0f, 21.0f, 10.0f, 15.0f, 270.0f, 0.58f, 2.75f);
    Spec.RearWheel = MakeWheel(35.0f, 23.0f, 10.0f, 15.0f, 285.0f, 0.60f, 2.90f);
    Spec.ForwardGearRatios = {3.20f, 2.10f, 1.42f, 1.00f, 0.78f};
    Spec.ReverseGearRatio = -3.05f;
    return Spec;
}

FGTTChaosVehicleSpec UGTTVehicleChaosSpecLibrary::GetMulebox1200Spec()
{
    FGTTChaosVehicleSpec Spec;
    Spec.VehicleId = TEXT("Mulebox1200");
    Spec.MassKg = 2180.0f;
    Spec.EngineMaxTorqueNm = 470.0f;
    Spec.EngineMaxRpm = 4200.0f;
    Spec.EngineIdleRpm = 780.0f;
    Spec.FinalDriveRatio = 4.45f;
    Spec.MaxSteeringAngleDegrees = 35.0f;
    Spec.DriveLayout = EGTTChaosDriveLayout::RearWheelDrive;
    Spec.FrontWheel = MakeWheel(37.0f, 24.0f, 12.0f, 19.0f, 305.0f, 0.64f, 2.85f);
    Spec.RearWheel = MakeWheel(38.0f, 26.0f, 11.0f, 18.0f, 340.0f, 0.66f, 3.00f);
    Spec.ForwardGearRatios = {4.05f, 2.45f, 1.55f, 1.00f, 0.76f};
    Spec.ReverseGearRatio = -3.65f;
    return Spec;
}

bool UGTTVehicleChaosSpecLibrary::GetSpecForVehicleId(FName VehicleId, FGTTChaosVehicleSpec& OutSpec)
{
    if (VehicleId == TEXT("Fieldmaster60") || VehicleId == TEXT("Tractor")) { OutSpec = GetFieldmaster60Spec(); return true; }
    if (VehicleId == TEXT("Rattleback82") || VehicleId == TEXT("OldCar")) { OutSpec = GetRattleback82Spec(); return true; }
    if (VehicleId == TEXT("Mulebox1200") || VehicleId == TEXT("FarmVan")) { OutSpec = GetMulebox1200Spec(); return true; }
    return false;
}
