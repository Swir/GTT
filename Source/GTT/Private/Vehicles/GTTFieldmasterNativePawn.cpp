#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "Camera/CameraComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Radio/GTTRadioComponent.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));
    constexpr float MirrorSyncIntervalSeconds = 0.5f;
    constexpr float TakeoverRetryIntervalSeconds = 1.0f;
    constexpr float NativeIdleFuelBurnPerSecond = 0.025f;
    constexpr float NativeFullThrottleFuelBurnPerSecond = 0.11f;
}

AGTTFieldmasterNativePawn::AGTTFieldmasterNativePawn()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetMesh());
    CameraBoom->TargetArmLength = 620.0f;
    CameraBoom->bUsePawnControlRotation = true;

    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    VehicleCamera->bUsePawnControlRotation = false;
}

void AGTTFieldmasterNativePawn::BeginPlay()
{
    Super::BeginPlay();

    FString Summary;
    bNativeReady = ConfigureAndValidateNativeFieldmaster(Summary);
    NativeAcceptanceSummary = Summary;

    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    if (bNativeReady)
    {
        UE_LOG(LogGTT, Log, TEXT("Fieldmaster native pawn accepted: %s"), *NativeAcceptanceSummary);
        TryActivateLegacyTakeover();
    }
    else
    {
        UE_LOG(LogGTT, Warning, TEXT("Fieldmaster native pawn not accepted: %s"), *NativeAcceptanceSummary);
    }
}

void AGTTFieldmasterNativePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bTakeoverActive)
    {
        TakeoverRetryAccumulator += DeltaSeconds;
        if (bNativeReady && TakeoverRetryAccumulator >= TakeoverRetryIntervalSeconds)
        {
            TakeoverRetryAccumulator = 0.0f;
            TryActivateLegacyTakeover();
        }
        return;
    }

    if (bOccupied && MigrationSnapshot.FuelLiters > 0.0f)
    {
        const float ThrottleAlpha = FMath::Clamp(FMath::Abs(LastThrottleInput), 0.0f, 1.0f);
        const float EfficiencyBonus = 1.0f - FMath::Clamp(MigrationSnapshot.EngineUpgradeLevel, 0, 3) * 0.035f;
        const float BurnRate = FMath::Lerp(NativeIdleFuelBurnPerSecond, NativeFullThrottleFuelBurnPerSecond, ThrottleAlpha) * EfficiencyBonus;
        MigrationSnapshot.FuelLiters = FMath::Max(0.0f, MigrationSnapshot.FuelLiters - BurnRate * DeltaSeconds);
        if (MigrationSnapshot.FuelLiters <= KINDA_SMALL_NUMBER)
        {
            LastThrottleInput = 0.0f;
            if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
            {
                Movement->SetThrottleInput(0.0f);
                Movement->SetBrakeInput(1.0f);
            }
        }
    }

    MirrorSyncAccumulator += DeltaSeconds;
    if (MirrorSyncAccumulator >= MirrorSyncIntervalSeconds)
    {
        MirrorSyncAccumulator = 0.0f;
        SyncLegacyMirror();
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
    PlayerInputComponent->BindAction(TEXT("ExitVehicle"), IE_Pressed, this, &AGTTFieldmasterNativePawn::ExitNativeVehicle);
    PlayerInputComponent->BindAction(TEXT("QuickSave"), IE_Pressed, this, &AGTTFieldmasterNativePawn::QuickSave);
    PlayerInputComponent->BindAction(TEXT("QuickLoad"), IE_Pressed, this, &AGTTFieldmasterNativePawn::QuickLoad);
    PlayerInputComponent->BindAction(TEXT("RadioNext"), IE_Pressed, this, &AGTTFieldmasterNativePawn::CycleRadio);
}

void AGTTFieldmasterNativePawn::Interact_Implementation(AActor* Interactor)
{
    if (!bNativeReady || !bTakeoverActive || bOccupied || !MigrationSnapshot.bOwnedByPlayer || MigrationSnapshot.ConditionPercent <= 0.0f)
    {
        return;
    }

    APawn* InteractingPawn = Cast<APawn>(Interactor);
    if (!InteractingPawn)
    {
        return;
    }

    AController* InteractingController = InteractingPawn->GetController();
    if (!InteractingController)
    {
        return;
    }

    PreviousPawn = InteractingPawn;

    FGTTChaosRigContract Rig;
    if (GetMesh() && UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig) && GetMesh()->DoesSocketExist(Rig.DriverSocket))
    {
        InteractingPawn->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Rig.DriverSocket);
    }
    else
    {
        InteractingPawn->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    }

    InteractingPawn->SetActorHiddenInGame(true);
    InteractingPawn->SetActorEnableCollision(false);
    InteractingController->Possess(this);
    bOccupied = true;
}

FText AGTTFieldmasterNativePawn::GetInteractionText_Implementation() const
{
    if (!bNativeReady) return NSLOCTEXT("GTT", "NativeFieldmasterRigUnavailable", "Fieldmaster native rig unavailable");
    if (!bTakeoverActive) return NSLOCTEXT("GTT", "NativeFieldmasterStandby", "Fieldmaster native takeover standby");
    if (bOccupied) return NSLOCTEXT("GTT", "NativeFieldmasterOccupied", "Occupied");
    if (!MigrationSnapshot.bOwnedByPlayer) return NSLOCTEXT("GTT", "NativeFieldmasterOwnershipRequired", "Native Fieldmaster unlocks after ownership");
    if (MigrationSnapshot.ConditionPercent <= 0.0f) return NSLOCTEXT("GTT", "NativeFieldmasterBroken", "Broken down");
    if (MigrationSnapshot.FuelLiters <= KINDA_SMALL_NUMBER) return NSLOCTEXT("GTT", "NativeFieldmasterEmpty", "Enter your Rusty Fieldmaster 60 (empty tank)");
    return NSLOCTEXT("GTT", "NativeFieldmasterEnter", "Enter your Rusty Fieldmaster 60");
}

void AGTTFieldmasterNativePawn::ExitNativeVehicle()
{
    AController* VehicleController = GetController();
    APawn* PawnToRestore = PreviousPawn.Get();
    if (!VehicleController || !PawnToRestore)
    {
        return;
    }

    LastThrottleInput = 0.0f;
    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetBrakeInput(1.0f);
    }

    PawnToRestore->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    FVector ExitLocation = GetActorTransform().TransformPosition(FVector(0.0f, 190.0f, 70.0f));
    FGTTChaosRigContract Rig;
    if (GetMesh() && UGTTChaosRigContractLibrary::GetRigForVehicleId(FieldmasterVehicleId, Rig) && GetMesh()->DoesSocketExist(Rig.ExitSocket))
    {
        ExitLocation = GetMesh()->GetSocketLocation(Rig.ExitSocket);
    }

    PawnToRestore->SetActorLocation(ExitLocation);
    PawnToRestore->SetActorRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
    PawnToRestore->SetActorHiddenInGame(false);
    PawnToRestore->SetActorEnableCollision(true);
    VehicleController->Possess(PawnToRestore);
    PreviousPawn.Reset();
    bOccupied = false;
    SyncLegacyMirror();
}

void AGTTFieldmasterNativePawn::HandleNativeThrottle(float Value)
{
    LastThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f);
    if (!bNativeReady || !bTakeoverActive || !bOccupied || MigrationSnapshot.FuelLiters <= KINDA_SMALL_NUMBER)
    {
        LastThrottleInput = 0.0f;
        return;
    }

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        const float Requested = LastThrottleInput;
        Movement->SetBrakeInput(FMath::IsNearlyZero(Requested) ? 0.15f : 0.0f);
        Movement->SetThrottleInput(FMath::Abs(Requested));
        Movement->SetTargetGear(Requested < -KINDA_SMALL_NUMBER ? -1 : 1, true);
    }
}

void AGTTFieldmasterNativePawn::HandleNativeSteering(float Value)
{
    if (!bNativeReady || !bTakeoverActive || !bOccupied)
    {
        return;
    }

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetSteeringInput(FMath::Clamp(Value, -1.0f, 1.0f));
    }
}

void AGTTFieldmasterNativePawn::QuickSave()
{
    SyncLegacyMirror();
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->SaveProgress();
    }
}

void AGTTFieldmasterNativePawn::QuickLoad()
{
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        ExitNativeVehicle();
        DeactivateLegacyTakeover();
        GameMode->LoadProgress();
        TryActivateLegacyTakeover();
    }
}

void AGTTFieldmasterNativePawn::CycleRadio()
{
    if (UGTTRadioComponent* Radio = UGTTGameplayStatics::FindRadioComponentForPawn(this))
    {
        Radio->CycleStation();
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
        OutSummary = FString::Printf(TEXT("Persistent ID mismatch: expected %s, got %s"), *FieldmasterVehicleId.ToString(), *LegacyVehicle->GetPersistentVehicleId().ToString());
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

    OutSummary = FString::Printf(TEXT("Imported %s gameplay state: condition %.1f%%, fuel %.1f L, engine upgrade %d, tire upgrade %d, tires %.0f%%, owned %s"),
        *FieldmasterVehicleId.ToString(), MigrationSnapshot.ConditionPercent, MigrationSnapshot.FuelLiters,
        MigrationSnapshot.EngineUpgradeLevel, MigrationSnapshot.TireUpgradeLevel, MigrationSnapshot.TireIntegrity * 100.0f,
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

bool AGTTFieldmasterNativePawn::TryActivateLegacyTakeover()
{
    if (bTakeoverActive)
    {
        return true;
    }
    if (!bNativeReady || !GetWorld())
    {
        return false;
    }

    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* LegacyVehicle = *It;
        if (!LegacyVehicle || LegacyVehicle->GetPersistentVehicleId() != FieldmasterVehicleId)
        {
            continue;
        }
        if (!LegacyVehicle->IsOwnedByPlayer() || LegacyVehicle->IsOccupied())
        {
            return false;
        }

        FString ImportSummary;
        if (!ImportLegacyGameplayState(LegacyVehicle, ImportSummary))
        {
            UE_LOG(LogGTT, Warning, TEXT("Fieldmaster takeover rejected: %s"), *ImportSummary);
            return false;
        }

        LegacyMirror = LegacyVehicle;
        SetActorTransform(LegacyVehicle->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
        LegacyVehicle->SetActorHiddenInGame(true);
        LegacyVehicle->SetActorEnableCollision(false);
        LegacyVehicle->SetActorTickEnabled(false);
        SetActorHiddenInGame(false);
        SetActorEnableCollision(true);
        bTakeoverActive = true;
        MirrorSyncAccumulator = 0.0f;
        UE_LOG(LogGTT, Log, TEXT("Fieldmaster native takeover ACTIVE: %s"), *ImportSummary);
        return true;
    }

    return false;
}

void AGTTFieldmasterNativePawn::DeactivateLegacyTakeover()
{
    if (!bTakeoverActive)
    {
        return;
    }

    if (bOccupied)
    {
        ExitNativeVehicle();
    }
    SyncLegacyMirror();

    if (AGTTVehicleBase* LegacyVehicle = LegacyMirror.Get())
    {
        LegacyVehicle->SetActorHiddenInGame(false);
        LegacyVehicle->SetActorEnableCollision(true);
        LegacyVehicle->SetActorTickEnabled(true);
    }

    LegacyMirror.Reset();
    bTakeoverActive = false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

bool AGTTFieldmasterNativePawn::RecallToTransform(const FTransform& Destination)
{
    if (!bTakeoverActive || bOccupied)
    {
        return false;
    }

    SetActorTransform(Destination, false, nullptr, ETeleportType::TeleportPhysics);
    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetBrakeInput(1.0f);
    }
    SyncLegacyMirror();
    return true;
}

void AGTTFieldmasterNativePawn::SyncLegacyMirror()
{
    AGTTVehicleBase* LegacyVehicle = LegacyMirror.Get();
    if (!bTakeoverActive || !LegacyVehicle)
    {
        return;
    }

    LegacyVehicle->RestorePersistentState(
        GetActorTransform(),
        MigrationSnapshot.ConditionPercent,
        MigrationSnapshot.FuelLiters,
        MigrationSnapshot.bOwnedByPlayer,
        MigrationSnapshot.EngineUpgradeLevel,
        MigrationSnapshot.TireUpgradeLevel,
        MigrationSnapshot.TireIntegrity);
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
    const bool bWheelsConfigured = UGTTChaosNativeSetupLibrary::ConfigureCanonicalWheelSetups(Movement, FieldmasterVehicleId, WheelConfigureSummary);

    FString PowertrainConfigureSummary;
    const bool bPowertrainConfigured = UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain(Movement, FieldmasterVehicleId, PowertrainConfigureSummary);

    FString WheelValidationSummary;
    const bool bWheelsValid = bWheelsConfigured && UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(Movement, FieldmasterVehicleId, WheelValidationSummary);

    FString PowertrainValidationSummary;
    const bool bPowertrainValid = bPowertrainConfigured && UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(Movement, FieldmasterVehicleId, PowertrainValidationSummary);

    const bool bPhysicsAssetPresent = GetMesh() && GetMesh()->GetPhysicsAsset() != nullptr;
    bNativeReady = bWheelsValid && bPowertrainValid && bPhysicsAssetPresent;

    OutSummary = FString::Printf(TEXT("RIG: %s | WHEELS: %s | POWERTRAIN: %s | PHYSICS ASSET: %s"),
        *RigSummary,
        bWheelsValid ? *WheelValidationSummary : *WheelConfigureSummary,
        bPowertrainValid ? *PowertrainValidationSummary : *PowertrainConfigureSummary,
        bPhysicsAssetPresent ? TEXT("YES") : TEXT("NO"));
    NativeAcceptanceSummary = OutSummary;
    return bNativeReady;
}
