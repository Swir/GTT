#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTVehicleBase.h"
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

void AGTTFieldmasterNativePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }

    PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGTTFieldmasterNativePawn::HandleNativeThrottle);
    PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGTTFieldmasterNativePawn::HandleNativeSteering);
}

void AGTTFieldmasterNativePawn::HandleNativeThrottle(float Value)
{
    if (!bNativeReady)
    {
        return;
    }

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(FMath::Clamp(Value, -1.0f, 1.0f));
    }
}

void AGTTFieldmasterNativePawn::HandleNativeSteering(float Value)
{
    if (!bNativeReady)
    {
        return;
    }

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetSteeringInput(FMath::Clamp(Value, -1.0f, 1.0f));
    }
}

bool AGTTFieldmasterNativePawn::ImportLegacyGameplayState(const AGTTVehicleBase* LegacyVehicle, FString& OutSummary)
{
    if (!LegacyVehicle)
    {
        OutSummary = TEXT("Legacy vehicle missing");
        return false;
    }

    if (LegacyVehicle->GetPersistentVehicleId() != FieldmasterVehicleId)
    {
        OutSummary = FString::Printf(
            TEXT("Persistent ID mismatch: expected %s, got %s"),
            *FieldmasterVehicleId.ToString(),
            *LegacyVehicle->GetPersistentVehicleId().ToString());
        return false;
    }

    FGTTVehicleMigrationSnapshot Snapshot;
    Snapshot.ConditionPercent = LegacyVehicle->GetConditionPercent();
    Snapshot.FuelLiters = LegacyVehicle->GetFuelLiters();
    Snapshot.bOwnedByPlayer = LegacyVehicle->IsOwnedByPlayer();
    Snapshot.EngineUpgradeLevel = LegacyVehicle->GetEngineUpgradeLevel();
    Snapshot.TireUpgradeLevel = LegacyVehicle->GetTireUpgradeLevel();
    Snapshot.TireIntegrity = LegacyVehicle->GetTireIntegrity();
    ApplyMigrationSnapshot(Snapshot);

    OutSummary = FString::Printf(
        TEXT("Imported %s gameplay state: condition %.1f%%, fuel %.1f L, engine upgrade %d, tire upgrade %d, tires %.0f%%, owned %s"),
        *FieldmasterVehicleId.ToString(),
        MigrationSnapshot.ConditionPercent,
        MigrationSnapshot.FuelLiters,
        MigrationSnapshot.EngineUpgradeLevel,
        MigrationSnapshot.TireUpgradeLevel,
        MigrationSnapshot.TireIntegrity * 100.0f,
        MigrationSnapshot.bOwnedByPlayer ? TEXT("YES") : TEXT("NO"));
    return true;
}

void AGTTFieldmasterNativePawn::ApplyMigrationSnapshot(const FGTTVehicleMigrationSnapshot& Snapshot)
{
    MigrationSnapshot.ConditionPercent = FMath::Clamp(Snapshot.ConditionPercent, 0.0f, 100.0f);
    MigrationSnapshot.FuelLiters = FMath::Max(0.0f, Snapshot.FuelLiters);
    MigrationSnapshot.bOwnedByPlayer = Snapshot.bOwnedByPlayer;
    MigrationSnapshot.EngineUpgradeLevel = FMath::Max(0, Snapshot.EngineUpgradeLevel);
    MigrationSnapshot.TireUpgradeLevel = FMath::Max(0, Snapshot.TireUpgradeLevel);
    MigrationSnapshot.TireIntegrity = FMath::Clamp(Snapshot.TireIntegrity, 0.0f, 1.0f);
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
