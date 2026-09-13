#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
}

AGTTFieldmasterNativePawn::AGTTFieldmasterNativePawn()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGTTFieldmasterNativePawn::BeginPlay()
{
    Super::BeginPlay();

    FString Summary;
    bNativeReady = ConfigureAndValidateNativeFieldmaster(Summary);
    NativeAcceptanceSummary = Summary;

    if (bNativeReady)
    {
        UE_LOG(LogGTT, Log, TEXT("Fieldmaster native pawn accepted: %s"), *NativeAcceptanceSummary);
    }
    else
    {
        UE_LOG(LogGTT, Warning, TEXT("Fieldmaster native pawn not accepted: %s"), *NativeAcceptanceSummary);
    }
}

bool AGTTFieldmasterNativePawn::ValidateRigContract(FString& OutSummary) const
{
    const USkeletalMeshComponent* Mesh = GetMesh();
    if (!Mesh)
    {
        OutSummary = TEXT("No skeletal mesh component");
        return false;
    }

    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig))
    {
        OutSummary = TEXT("No Fieldmaster rig contract");
        return false;
    }

    TArray<FString> Missing;
    for (const FName BoneName : UGTTChaosRigContractLibrary::GetRequiredBoneNames(Rig))
    {
        if (BoneName.IsNone() || Mesh->GetBoneIndex(BoneName) == INDEX_NONE)
        {
            Missing.Add(FString::Printf(TEXT("bone:%s"), *BoneName.ToString()));
        }
    }

    for (const FName SocketName : UGTTChaosRigContractLibrary::GetRequiredSocketNames(Rig))
    {
        if (SocketName.IsNone() || !Mesh->DoesSocketExist(SocketName))
        {
            Missing.Add(FString::Printf(TEXT("socket:%s"), *SocketName.ToString()));
        }
    }

    if (Missing.Num() > 0)
    {
        OutSummary = FString::Printf(TEXT("Rig missing %s"), *FString::Join(Missing, TEXT(", ")));
        return false;
    }

    OutSummary = TEXT("Rig contract valid");
    return true;
}

bool AGTTFieldmasterNativePawn::ConfigureAndValidateNativeFieldmaster(FString& OutSummary)
{
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
    if (!Movement)
    {
        bNativeReady = false;
        OutSummary = TEXT("No UChaosWheeledVehicleMovementComponent");
        NativeAcceptanceSummary = OutSummary;
        return false;
    }

    FString RigSummary;
    if (!ValidateRigContract(RigSummary))
    {
        bNativeReady = false;
        OutSummary = FString::Printf(TEXT("RIG: %s"), *RigSummary);
        NativeAcceptanceSummary = OutSummary;
        return false;
    }

    FString WheelConfigureSummary;
    const bool bWheelsConfigured = UGTTChaosNativeSetupLibrary::ConfigureCanonicalWheelSetups(
        Movement,
        FieldmasterVehicleId,
        WheelConfigureSummary);

    FString PowertrainConfigureSummary;
    const bool bPowertrainConfigured = UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain(
        Movement,
        FieldmasterVehicleId,
        PowertrainConfigureSummary);

    FString WheelValidationSummary;
    const bool bWheelsValid = bWheelsConfigured && UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(
        Movement,
        FieldmasterVehicleId,
        WheelValidationSummary);

    FString PowertrainValidationSummary;
    const bool bPowertrainValid = bPowertrainConfigured && UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(
        Movement,
        FieldmasterVehicleId,
        PowertrainValidationSummary);

    const bool bPhysicsAssetPresent = GetMesh() && GetMesh()->GetPhysicsAsset() != nullptr;
    bNativeReady = bWheelsValid && bPowertrainValid && bPhysicsAssetPresent;

    OutSummary = FString::Printf(
        TEXT("RIG: %s | WHEELS: %s | POWERTRAIN: %s | PHYSICS ASSET: %s"),
        *RigSummary,
        bWheelsValid ? *WheelValidationSummary : *WheelConfigureSummary,
        bPowertrainValid ? *PowertrainValidationSummary : *PowertrainConfigureSummary,
        bPhysicsAssetPresent ? TEXT("YES") : TEXT("NO"));
    NativeAcceptanceSummary = OutSummary;
    return bNativeReady;
}
