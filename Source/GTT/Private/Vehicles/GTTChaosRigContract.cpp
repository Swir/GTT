#include "Vehicles/GTTChaosRigContract.h"

namespace
{
FGTTChaosRigContract MakeRig(FName VehicleId, bool bRequiresHitch)
{
    FGTTChaosRigContract Rig;
    Rig.VehicleId = VehicleId;
    Rig.bRequiresHitchSocket = bRequiresHitch;
    Rig.HitchSocket = bRequiresHitch ? FName(TEXT("rear_hitch")) : NAME_None;
    return Rig;
}
}

FGTTChaosRigContract UGTTChaosRigContractLibrary::GetFieldmaster60Rig()
{
    return MakeRig(TEXT("RustyFieldmaster60"), true);
}

FGTTChaosRigContract UGTTChaosRigContractLibrary::GetRattleback82Rig()
{
    return MakeRig(TEXT("Rattleback82"), false);
}

FGTTChaosRigContract UGTTChaosRigContractLibrary::GetMulebox1200Rig()
{
    return MakeRig(TEXT("Mulebox1200"), true);
}

bool UGTTChaosRigContractLibrary::GetRigForVehicleId(FName VehicleId, FGTTChaosRigContract& OutRig)
{
    if (VehicleId == TEXT("RustyFieldmaster60")) { OutRig = GetFieldmaster60Rig(); return true; }
    if (VehicleId == TEXT("Rattleback82")) { OutRig = GetRattleback82Rig(); return true; }
    if (VehicleId == TEXT("Mulebox1200")) { OutRig = GetMulebox1200Rig(); return true; }
    OutRig = FGTTChaosRigContract();
    return false;
}

TArray<FName> UGTTChaosRigContractLibrary::GetRequiredBoneNames(const FGTTChaosRigContract& Rig)
{
    return { Rig.RootBone, Rig.FrontLeftWheelBone, Rig.FrontRightWheelBone, Rig.RearLeftWheelBone, Rig.RearRightWheelBone };
}

TArray<FName> UGTTChaosRigContractLibrary::GetRequiredSocketNames(const FGTTChaosRigContract& Rig)
{
    TArray<FName> Names { Rig.DriverSocket, Rig.ExitSocket };
    if (Rig.bRequiresHitchSocket && !Rig.HitchSocket.IsNone()) Names.Add(Rig.HitchSocket);
    return Names;
}
