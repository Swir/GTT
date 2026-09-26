#include "Vehicles/GTTFieldmasterNativePawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Radio/GTTRadioComponent.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTFieldmasterChaosMovementComponent.h"
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

AGTTFieldmasterNativePawn::AGTTFieldmasterNativePawn(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UGTTFieldmasterChaosMovementComponent>(AWheeledVehiclePawn::VehicleMovementComponentName))
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> VehicleMesh(
        TEXT("/Game/GTT/Vehicles/Fieldmaster/SK_GTT_Fieldmaster60.SK_GTT_Fieldmaster60"));
    if (VehicleMesh.Succeeded())
    {
        GetMesh()->SetSkeletalMesh(VehicleMesh.Object);
    }

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
        GTT_LOG( Log, TEXT("Fieldmaster native pawn accepted: %s"), *NativeAcceptanceSummary);
        TryActivateLegacyTakeover();
    }
    else
    {
        GTT_LOG( Warning, TEXT("Fieldmaster native pawn not accepted: %s"), *NativeAcceptanceSummary);
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
    }

    if (bOccupied)
    {
        RefreshNativeDriveCommand();
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
    RefreshNativeDriveCommand();
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
    LastSteeringInput = 0.0f;
    if (UGTTFieldmasterChaosMovementComponent* Movement = GetFieldmasterMovement())
    {
        Movement->HoldFieldmasterStopped();
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

UGTTFieldmasterChaosMovementComponent* AGTTFieldmasterNativePawn::GetFieldmasterMovement() const
{
    return Cast<UGTTFieldmasterChaosMovementComponent>(GetVehicleMovementComponent());
}

void AGTTFieldmasterNativePawn::RefreshNativeDriveCommand()
{
    UGTTFieldmasterChaosMovementComponent* Movement = GetFieldmasterMovement();
    if (!Movement)
    {
        return;
    }

    const bool bCanDrive = bNativeReady && bTakeoverActive && bOccupied && MigrationSnapshot.FuelLiters > KINDA_SMALL_NUMBER;
    if (!bCanDrive)
    {
        Movement->HoldFieldmasterStopped();
        return;
    }

    Movement->ApplyFieldmasterDriveCommand(
        LastThrottleInput,
        LastSteeringInput,
        true,
        MigrationSnapshot.ConditionPercent,
        MigrationSnapshot.TireIntegrity,
        GetNativeTerrainGripFactor());
}

void AGTTFieldmasterNativePawn::HandleNativeThrottle(float Value)
{
    LastThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f);
    RefreshNativeDriveCommand();
}

void AGTTFieldmasterNativePawn::HandleNativeSteering(float Value)
{
    LastSteeringInput = FMath::Clamp(Value, -1.0f, 1.0f);
    RefreshNativeDriveCommand();
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
        *FieldmasterVehicleId.ToString(), MigrationSnapshot.ConditionPercent * 100.0f, MigrationSnapshot.FuelLiters,
        MigrationSnapshot.EngineUpgradeLevel, MigrationSnapshot.TireUpgradeLevel, MigrationSnapshot.TireIntegrity * 100.0f,
        MigrationSnapshot.bOwnedByPlayer ? TEXT("YES") : TEXT("NO"));
    return true;
}

void AGTTFieldmasterNativePawn::ApplyMigrationSnapshot(const FGTTVehicleMigrationSnapshot& Snapshot)
{
    // Canonical native state is a 0..1 ratio, but this public migration boundary may still be
    // reached by an older caller carrying a 0..100 condition value. Normalize once on ingress so
    // every downstream Chaos authority consumes one consistent unit.
    const float RawCondition = Snapshot.ConditionPercent;
    const float NormalizedCondition = RawCondition > 1.0f ? RawCondition / 100.0f : RawCondition;
    MigrationSnapshot.ConditionPercent = FMath::Clamp(NormalizedCondition, 0.0f, 1.0f);
    MigrationSnapshot.FuelLiters = FMath::Max(0.0f, Snapshot.FuelLiters);
    MigrationSnapshot.bOwnedByPlayer = Snapshot.bOwnedByPlayer;
    MigrationSnapshot.EngineUpgradeLevel = FMath::Clamp(Snapshot.EngineUpgradeLevel, 0, 3);
    MigrationSnapshot.TireUpgradeLevel = FMath::Clamp(Snapshot.TireUpgradeLevel, 0, 3);
    MigrationSnapshot.TireIntegrity = FMath::Clamp(Snapshot.TireIntegrity, 0.0f, 1.0f);
    RefreshNativeDriveCommand();
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
            GTT_LOG( Warning, TEXT("Fieldmaster takeover rejected: %s"), *ImportSummary);
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
        RefreshNativeDriveCommand();
        GTT_LOG( Log, TEXT("Fieldmaster native takeover ACTIVE: %s"), *ImportSummary);
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
    if (UGTTFieldmasterChaosMovementComponent* Movement = GetFieldmasterMovement())
    {
        Movement->HoldFieldmasterStopped();
    }
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
    if (UGTTFieldmasterChaosMovementComponent* Movement = GetFieldmasterMovement())
    {
        Movement->HoldFieldmasterStopped();
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
    const USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
    if (!SkeletalMeshComponent)
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
        if (BoneName.IsNone() || SkeletalMeshComponent->GetBoneIndex(BoneName) == INDEX_NONE)
        {
            Missing.Add(FString::Printf(TEXT("bone:%s"), *BoneName.ToString()));
        }
    }

    for (const FName SocketName : UGTTChaosRigContractLibrary::GetRequiredSocketNames(Rig))
    {
        if (SocketName.IsNone() || !SkeletalMeshComponent->DoesSocketExist(SocketName))
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
    UGTTFieldmasterChaosMovementComponent* Movement = GetFieldmasterMovement();
    if (!Movement)
    {
        bNativeReady = false;
        OutSummary = TEXT("Dedicated UGTTFieldmasterChaosMovementComponent missing");
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

    FString MovementSummary;
    const bool bMovementValid = Movement->ConfigureAndValidateFieldmaster(MovementSummary);
    const bool bPhysicsAssetPresent = GetMesh() && GetMesh()->GetPhysicsAsset() != nullptr;
    bNativeReady = bMovementValid && bPhysicsAssetPresent;

    OutSummary = FString::Printf(TEXT("RIG: %s | %s | PHYSICS ASSET: %s"),
        *RigSummary,
        *MovementSummary,
        bPhysicsAssetPresent ? TEXT("YES") : TEXT("NO"));
    NativeAcceptanceSummary = OutSummary;

    if (!bNativeReady)
    {
        Movement->HoldFieldmasterStopped();
    }
    return bNativeReady;
}
