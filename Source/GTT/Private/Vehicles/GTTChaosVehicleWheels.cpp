#include "Vehicles/GTTChaosVehicleWheels.h"
#include "Vehicles/GTTChaosVehicleSpec.h"

namespace
{
void ApplyWheelSpec(UChaosVehicleWheel& Wheel, const FGTTChaosWheelSpec& Spec, bool bSteering, bool bEngine, bool bHandbrake, float MaxSteerAngle)
{
    Wheel.WheelRadius = Spec.RadiusCm;
    Wheel.WheelWidth = Spec.WidthCm;
    Wheel.SuspensionMaxRaise = Spec.SuspensionMaxRaiseCm;
    Wheel.SuspensionMaxDrop = Spec.SuspensionMaxDropCm;
    Wheel.SpringRate = Spec.SuspensionSpringRate;
    Wheel.SuspensionDampingRatio = Spec.SuspensionDampingRatio;
    Wheel.FrictionForceMultiplier = Spec.FrictionForceMultiplier;
    Wheel.bAffectedBySteering = bSteering;
    Wheel.bAffectedByEngine = bEngine;
    Wheel.bAffectedByBrake = true;
    Wheel.bAffectedByHandbrake = bHandbrake;
    Wheel.MaxSteerAngle = bSteering ? MaxSteerAngle : 0.0f;
    Wheel.bABSEnabled = false;
    Wheel.bTractionControlEnabled = false;
}
}

UGTTFieldmasterFrontWheel::UGTTFieldmasterFrontWheel()
{
    const FGTTChaosVehicleSpec Spec = UGTTVehicleChaosSpecLibrary::GetFieldmaster60Spec();
    ApplyWheelSpec(*this, Spec.FrontWheel, true, false, false, Spec.MaxSteeringAngleDegrees);
}

UGTTFieldmasterRearWheel::UGTTFieldmasterRearWheel()
{
    const FGTTChaosVehicleSpec Spec = UGTTVehicleChaosSpecLibrary::GetFieldmaster60Spec();
    ApplyWheelSpec(*this, Spec.RearWheel, false, true, true, Spec.MaxSteeringAngleDegrees);
}

UGTTRattlebackFrontWheel::UGTTRattlebackFrontWheel()
{
    const FGTTChaosVehicleSpec Spec = UGTTVehicleChaosSpecLibrary::GetRattleback82Spec();
    ApplyWheelSpec(*this, Spec.FrontWheel, true, false, false, Spec.MaxSteeringAngleDegrees);
}

UGTTRattlebackRearWheel::UGTTRattlebackRearWheel()
{
    const FGTTChaosVehicleSpec Spec = UGTTVehicleChaosSpecLibrary::GetRattleback82Spec();
    ApplyWheelSpec(*this, Spec.RearWheel, false, true, true, Spec.MaxSteeringAngleDegrees);
}

UGTTMuleboxFrontWheel::UGTTMuleboxFrontWheel()
{
    const FGTTChaosVehicleSpec Spec = UGTTVehicleChaosSpecLibrary::GetMulebox1200Spec();
    ApplyWheelSpec(*this, Spec.FrontWheel, true, false, false, Spec.MaxSteeringAngleDegrees);
}

UGTTMuleboxRearWheel::UGTTMuleboxRearWheel()
{
    const FGTTChaosVehicleSpec Spec = UGTTVehicleChaosSpecLibrary::GetMulebox1200Spec();
    ApplyWheelSpec(*this, Spec.RearWheel, false, true, true, Spec.MaxSteeringAngleDegrees);
}
