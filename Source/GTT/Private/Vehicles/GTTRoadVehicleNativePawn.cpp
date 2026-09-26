#include "Vehicles/GTTRoadVehicleNativePawn.h"

#include "Camera/CameraComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTChaosNativeSetupLibrary.h"
#include "Vehicles/GTTChaosPowertrainSetupLibrary.h"
#include "Vehicles/GTTChaosRigContract.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace
{
    constexpr float MirrorSyncIntervalSeconds = 0.5f;
    constexpr float TakeoverRetryIntervalSeconds = 1.0f;
    constexpr float RuntimeGuardIntervalSeconds = 2.0f;
    constexpr float WheelEvidenceIntervalSeconds = 4.0f;
    constexpr float DamageEvidenceIntervalSeconds = 4.0f;
    constexpr float RuntimeTractionRiskThreshold = 0.34f;
    constexpr float ImpactDamageCooldownSeconds = 0.30f;
    constexpr float MinimumImpactSpeedKmh = 14.0f;
    constexpr float SevereImpactSpeedKmh = 38.0f;
    constexpr float PanelDetachMinimumSpeedKmh = 27.0f;

    void ConfigureDamageDebrisComponent(UStaticMeshComponent* Component, UStaticMesh* Mesh)
    {
        if (!Component) return;
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetSimulatePhysics(false);
        Component->SetVisibility(false, true);
        Component->SetHiddenInGame(true, true);
    }
}

AGTTRoadVehicleNativePawn::AGTTRoadVehicleNativePawn()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetMesh());
    CameraBoom->TargetArmLength = 560.0f;
    CameraBoom->bUsePawnControlRotation = true;
    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    VehicleCamera->bUsePawnControlRotation = false;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

    FrontDamageDebris = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontDamageDebris"));
    FrontDamageDebris->SetupAttachment(GetMesh());
    ConfigureDamageDebrisComponent(FrontDamageDebris, CubeMesh);

    RearDamageDebris = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearDamageDebris"));
    RearDamageDebris->SetupAttachment(GetMesh());
    ConfigureDamageDebrisComponent(RearDamageDebris, CubeMesh);

    LeftDamageDebris = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDamageDebris"));
    LeftDamageDebris->SetupAttachment(GetMesh());
    ConfigureDamageDebrisComponent(LeftDamageDebris, CubeMesh);

    RightDamageDebris = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDamageDebris"));
    RightDamageDebris->SetupAttachment(GetMesh());
    ConfigureDamageDebrisComponent(RightDamageDebris, CubeMesh);
}

AGTTRattlebackNativePawn::AGTTRattlebackNativePawn()
{
    NativeVehicleId = TEXT("Rattleback82");
    NativeDisplayName = NSLOCTEXT("GTT", "NativeRattlebackName", "Rattleback 82");
    FuelCapacityLiters = 42.0f;
    IdleFuelBurnPerSecond = 0.028f;
    FullThrottleFuelBurnPerSecond = 0.19f;
    ExitOffset = FVector(0.0f, 165.0f, 65.0f);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> VehicleMesh(
        TEXT("/Game/GTT/Vehicles/Rattleback/SK_GTT_Rattleback82.SK_GTT_Rattleback82"));
    if (VehicleMesh.Succeeded()) GetMesh()->SetSkeletalMesh(VehicleMesh.Object);
}

AGTTMuleboxNativePawn::AGTTMuleboxNativePawn()
{
    NativeVehicleId = TEXT("Mulebox1200");
    NativeDisplayName = NSLOCTEXT("GTT", "NativeMuleboxName", "Mulebox 1200");
    FuelCapacityLiters = 62.0f;
    IdleFuelBurnPerSecond = 0.04f;
    FullThrottleFuelBurnPerSecond = 0.22f;
    ExitOffset = FVector(0.0f, 205.0f, 82.0f);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> VehicleMesh(
        TEXT("/Game/GTT/Vehicles/Mulebox/SK_GTT_Mulebox1200.SK_GTT_Mulebox1200"));
    if (VehicleMesh.Succeeded()) GetMesh()->SetSkeletalMesh(VehicleMesh.Object);
}

void AGTTRoadVehicleNativePawn::BeginPlay()
{
    Super::BeginPlay();
    ConfigureDamageDebrisLayout();
    FString Summary;
    bNativeReady = ConfigureAndValidateNativeRoadVehicle(Summary);
    NativeAcceptanceSummary = Summary;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    if (bNativeReady)
    {
        GTT_LOG( Log, TEXT("NATIVE_ROAD_ACCEPTED vehicle=%s %s"), *NativeVehicleId.ToString(), *NativeAcceptanceSummary);
        TryActivateLegacyTakeover();
    }
    else
    {
        GTT_LOG( Warning, TEXT("NATIVE_ROAD_WAIT vehicle=%s %s"), *NativeVehicleId.ToString(), *NativeAcceptanceSummary);
    }
}

void AGTTRoadVehicleNativePawn::Tick(float DeltaSeconds)
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

    UpdateNativeWheelRuntime(DeltaSeconds);
    UpdateDamageConsequences(DeltaSeconds);

    if (bOccupied && MigrationSnapshot.ConditionPercent <= KINDA_SMALL_NUMBER)
    {
        StopNativeDriveForBreakdown();
    }
    else if (bOccupied && MigrationSnapshot.FuelLiters > 0.0f)
    {
        const float ThrottleAlpha = FMath::Clamp(FMath::Abs(LastThrottleInput), 0.0f, 1.0f);
        const float EfficiencyBonus = 1.0f - FMath::Clamp(MigrationSnapshot.EngineUpgradeLevel, 0, 3) * 0.035f;
        const float BurnRate = FMath::Lerp(IdleFuelBurnPerSecond, FullThrottleFuelBurnPerSecond, ThrottleAlpha) * EfficiencyBonus;
        MigrationSnapshot.FuelLiters = FMath::Max(0.0f, MigrationSnapshot.FuelLiters - BurnRate * DeltaSeconds);
        if (MigrationSnapshot.FuelLiters <= KINDA_SMALL_NUMBER)
        {
            StopNativeDriveForBreakdown();
        }
    }

    MirrorSyncAccumulator += DeltaSeconds;
    if (MirrorSyncAccumulator >= MirrorSyncIntervalSeconds)
    {
        MirrorSyncAccumulator = 0.0f;
        SyncLegacyMirror();
    }

    RuntimeGuardAccumulator += DeltaSeconds;
    if (RuntimeGuardAccumulator >= RuntimeGuardIntervalSeconds)
    {
        RuntimeGuardAccumulator = 0.0f;
        RuntimeAcceptanceGuard();
    }
}

void AGTTRoadVehicleNativePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent) return;
    PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGTTRoadVehicleNativePawn::HandleNativeThrottle);
    PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGTTRoadVehicleNativePawn::HandleNativeSteering);
    PlayerInputComponent->BindAction(TEXT("ExitVehicle"), IE_Pressed, this, &AGTTRoadVehicleNativePawn::ExitNativeVehicle);
}

void AGTTRoadVehicleNativePawn::Interact_Implementation(AActor* Interactor)
{
    if (!bNativeReady || !bTakeoverActive || bOccupied || !MigrationSnapshot.bOwnedByPlayer || MigrationSnapshot.ConditionPercent <= 0.0f) return;
    APawn* InteractingPawn = Cast<APawn>(Interactor);
    if (!InteractingPawn) return;
    AController* PossessingController = InteractingPawn->GetController();
    if (!PossessingController) return;
    PreviousPawn = InteractingPawn;
    FGTTChaosRigContract Rig;
    if (GetMesh() && UGTTChaosRigContractLibrary::GetRigForVehicleId(NativeVehicleId, Rig) && GetMesh()->DoesSocketExist(Rig.DriverSocket))
        InteractingPawn->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Rig.DriverSocket);
    else
        InteractingPawn->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    InteractingPawn->SetActorHiddenInGame(true);
    InteractingPawn->SetActorEnableCollision(false);
    PossessingController->Possess(this);
    bOccupied = true;
    GTT_LOG( Log, TEXT("NATIVE_ROAD_DRIVER_ENTER vehicle=%s"), *NativeVehicleId.ToString());
}

FText AGTTRoadVehicleNativePawn::GetInteractionText_Implementation() const
{
    if (!bNativeReady) return FText::FromString(FString::Printf(TEXT("%s native rig unavailable"), *NativeDisplayName.ToString()));
    if (!bTakeoverActive) return FText::FromString(FString::Printf(TEXT("%s native takeover standby"), *NativeDisplayName.ToString()));
    if (bOccupied) return NSLOCTEXT("GTT", "NativeRoadOccupied", "Occupied");
    if (!MigrationSnapshot.bOwnedByPlayer) return FText::FromString(FString::Printf(TEXT("Own %s to use Native Chaos"), *NativeDisplayName.ToString()));
    if (MigrationSnapshot.ConditionPercent <= 0.0f) return NSLOCTEXT("GTT", "NativeRoadBroken", "Broken down");
    return FText::FromString(FString::Printf(TEXT("Enter %s"), *NativeDisplayName.ToString()));
}

void AGTTRoadVehicleNativePawn::ExitNativeVehicle()
{
    AController* PossessingController = GetController();
    APawn* PawnToRestore = PreviousPawn.Get();
    if (!PossessingController || !PawnToRestore) return;
    LastThrottleInput = 0.0f;
    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetBrakeInput(1.0f);
    }
    PawnToRestore->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    FVector ExitLocation = GetActorTransform().TransformPosition(ExitOffset);
    FGTTChaosRigContract Rig;
    if (GetMesh() && UGTTChaosRigContractLibrary::GetRigForVehicleId(NativeVehicleId, Rig) && GetMesh()->DoesSocketExist(Rig.ExitSocket))
        ExitLocation = GetMesh()->GetSocketLocation(Rig.ExitSocket);
    PawnToRestore->SetActorLocation(ExitLocation);
    PawnToRestore->SetActorRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
    PawnToRestore->SetActorHiddenInGame(false);
    PawnToRestore->SetActorEnableCollision(true);
    PossessingController->Possess(PawnToRestore);
    PreviousPawn.Reset();
    bOccupied = false;
    SyncLegacyMirror();
    GTT_LOG( Log, TEXT("NATIVE_ROAD_DRIVER_EXIT vehicle=%s"), *NativeVehicleId.ToString());
}

void AGTTRoadVehicleNativePawn::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
    FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

    if (!bNativeReady || !bTakeoverActive || !GetWorld() || Other == this)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastImpactDamageTimeSeconds < ImpactDamageCooldownSeconds)
    {
        return;
    }

    const float VehicleSpeedKmh = GetVelocity().Size() * 0.036f;
    float ImpulseEquivalentKmh = 0.0f;
    if (GetMesh() && GetMesh()->GetMass() > KINDA_SMALL_NUMBER)
    {
        ImpulseEquivalentKmh = (NormalImpulse.Size() / GetMesh()->GetMass()) * 0.036f;
    }

    const float ImpactSpeedKmh = FMath::Max(VehicleSpeedKmh, ImpulseEquivalentKmh);
    if (ImpactSpeedKmh < MinimumImpactSpeedKmh)
    {
        return;
    }

    LastImpactDamageTimeSeconds = Now;
    LastImpactSpeedKmh = ImpactSpeedKmh;
    LastImpactZone = DetermineImpactZone(HitLocation);
    ++NativeImpactCount;

    const float PreviousCondition = MigrationSnapshot.ConditionPercent;
    const float PreviousTires = MigrationSnapshot.TireIntegrity;
    ApplyNativeImpactDamage(ImpactSpeedKmh, LastImpactZone, HitLocation, NormalImpulse);

    if (!FMath::IsNearlyEqual(PreviousCondition, MigrationSnapshot.ConditionPercent) ||
        !FMath::IsNearlyEqual(PreviousTires, MigrationSnapshot.TireIntegrity))
    {
        SyncLegacyMirror();
        GTT_LOG( Log,
            TEXT("NATIVE_ROAD_IMPACT_DAMAGE vehicle=%s zone=%s speed_kmh=%.1f condition=%.1f%% tire_integrity=%.2f condition_delta=%.3f tire_delta=%.3f cargo=%.2f impacts=%d other=%s"),
            *NativeVehicleId.ToString(),
            DamageZoneToString(LastImpactZone),
            ImpactSpeedKmh,
            MigrationSnapshot.ConditionPercent * 100.0f,
            MigrationSnapshot.TireIntegrity,
            PreviousCondition - MigrationSnapshot.ConditionPercent,
            PreviousTires - MigrationSnapshot.TireIntegrity,
            CargoLoadFactor,
            NativeImpactCount,
            Other ? *Other->GetName() : TEXT("WORLD"));
    }
}

bool AGTTRoadVehicleNativePawn::ValidateRigContract(FString& OutSummary) const
{
    if (!GetMesh()) { OutSummary = TEXT("No skeletal mesh component"); return false; }
    FGTTChaosRigContract Rig;
    if (!UGTTChaosRigContractLibrary::GetRigForVehicleId(NativeVehicleId, Rig)) { OutSummary = TEXT("No rig contract"); return false; }
    TArray<FString> Missing;
    for (const FName BoneName : UGTTChaosRigContractLibrary::GetRequiredBoneNames(Rig))
        if (BoneName.IsNone() || GetMesh()->GetBoneIndex(BoneName) == INDEX_NONE) Missing.Add(FString::Printf(TEXT("bone:%s"), *BoneName.ToString()));
    for (const FName SocketName : UGTTChaosRigContractLibrary::GetRequiredSocketNames(Rig))
        if (SocketName.IsNone() || !GetMesh()->DoesSocketExist(SocketName)) Missing.Add(FString::Printf(TEXT("socket:%s"), *SocketName.ToString()));
    if (Missing.Num() > 0) { OutSummary = FString::Printf(TEXT("Rig missing %s"), *FString::Join(Missing, TEXT(", "))); return false; }
    OutSummary = TEXT("Rig contract valid");
    return true;
}

bool AGTTRoadVehicleNativePawn::ConfigureAndValidateNativeRoadVehicle(FString& OutSummary)
{
    const bool bNeedsPhysicsRebuild = !bNativeReady;
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
    if (!Movement || NativeVehicleId.IsNone()) { OutSummary = TEXT("Native movement or vehicle ID missing"); bNativeReady = false; return false; }
    FString RigSummary;
    if (!ValidateRigContract(RigSummary)) { OutSummary = FString::Printf(TEXT("RIG: %s"), *RigSummary); bNativeReady = false; return false; }
    FString WheelConfigureSummary, PowertrainConfigureSummary, WheelValidationSummary, PowertrainValidationSummary;
    const bool bWheelsConfigured = UGTTChaosNativeSetupLibrary::ConfigureCanonicalWheelSetups(Movement, NativeVehicleId, WheelConfigureSummary);
    const bool bPowertrainConfigured = UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain(Movement, NativeVehicleId, PowertrainConfigureSummary);
    const bool bWheelsValid = bWheelsConfigured && UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups(Movement, NativeVehicleId, WheelValidationSummary);
    const bool bPowertrainValid = bPowertrainConfigured && UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain(Movement, NativeVehicleId, PowertrainValidationSummary);
    const bool bPhysicsAssetPresent = GetMesh() && GetMesh()->GetPhysicsAsset() != nullptr;
    bNativeReady = bWheelsValid && bPowertrainValid && bPhysicsAssetPresent;
    OutSummary = FString::Printf(TEXT("RIG: %s | WHEELS: %s | POWERTRAIN: %s | PHYSICS ASSET: %s"), *RigSummary,
        bWheelsValid ? *WheelValidationSummary : *WheelConfigureSummary,
        bPowertrainValid ? *PowertrainValidationSummary : *PowertrainConfigureSummary,
        bPhysicsAssetPresent ? TEXT("YES") : TEXT("NO"));
    NativeAcceptanceSummary = OutSummary;
    if (bNativeReady && bNeedsPhysicsRebuild)
    {
        // ConfigureCanonicalWheelSetups runs after the component's initial registration.
        // Rebuild once so the Chaos simulation owns the canonical four-wheel setup.
        Movement->RecreatePhysicsState();
    }
    return bNativeReady;
}

bool AGTTRoadVehicleNativePawn::ImportLegacyGameplayState(const AGTTVehicleBase* LegacyVehicle, FString& OutSummary)
{
    if (!LegacyVehicle || LegacyVehicle->GetPersistentVehicleId() != NativeVehicleId) { OutSummary = TEXT("Legacy vehicle mismatch"); return false; }
    MigrationSnapshot.ConditionPercent = FMath::Clamp(LegacyVehicle->GetConditionPercent(), 0.0f, 1.0f);
    MigrationSnapshot.FuelLiters = FMath::Clamp(LegacyVehicle->GetFuelLiters(), 0.0f, FuelCapacityLiters);
    MigrationSnapshot.bOwnedByPlayer = LegacyVehicle->IsOwnedByPlayer();
    MigrationSnapshot.EngineUpgradeLevel = LegacyVehicle->GetEngineUpgradeLevel();
    MigrationSnapshot.TireUpgradeLevel = LegacyVehicle->GetTireUpgradeLevel();
    MigrationSnapshot.TireIntegrity = LegacyVehicle->GetTireIntegrity();

    if (!bTakeoverActive && BodyDamage.DetachedPanelCount == 0 &&
        FMath::IsNearlyEqual(BodyDamage.FrontHealth, 1.0f) && FMath::IsNearlyEqual(BodyDamage.RearHealth, 1.0f) &&
        FMath::IsNearlyEqual(BodyDamage.LeftHealth, 1.0f) && FMath::IsNearlyEqual(BodyDamage.RightHealth, 1.0f))
    {
        const float SeedHealth = FMath::Clamp(FMath::Lerp(0.35f, 1.0f, MigrationSnapshot.ConditionPercent), 0.35f, 1.0f);
        BodyDamage.FrontHealth = SeedHealth;
        BodyDamage.RearHealth = SeedHealth;
        BodyDamage.LeftHealth = SeedHealth;
        BodyDamage.RightHealth = SeedHealth;
    }

    OutSummary = FString::Printf(TEXT("state condition=%.0f%% fuel=%.1f owned=%s engine=%d tires=%d integrity=%.2f body=%.2f/%.2f/%.2f/%.2f"),
        MigrationSnapshot.ConditionPercent * 100.0f, MigrationSnapshot.FuelLiters, MigrationSnapshot.bOwnedByPlayer ? TEXT("YES") : TEXT("NO"),
        MigrationSnapshot.EngineUpgradeLevel, MigrationSnapshot.TireUpgradeLevel, MigrationSnapshot.TireIntegrity,
        BodyDamage.FrontHealth, BodyDamage.RearHealth, BodyDamage.LeftHealth, BodyDamage.RightHealth);
    return true;
}

bool AGTTRoadVehicleNativePawn::TryActivateLegacyTakeover()
{
    if (bTakeoverActive) return true;
    if (!bNativeReady || !GetWorld()) return false;
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* LegacyVehicle = *It;
        if (!LegacyVehicle || LegacyVehicle->GetPersistentVehicleId() != NativeVehicleId) continue;
        if (!LegacyVehicle->IsOwnedByPlayer() || LegacyVehicle->IsOccupied()) return false;
        FString ImportSummary;
        if (!ImportLegacyGameplayState(LegacyVehicle, ImportSummary)) return false;
        LegacyMirror = LegacyVehicle;
        SetActorTransform(LegacyVehicle->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
        LegacyVehicle->SetActorHiddenInGame(true);
        LegacyVehicle->SetActorEnableCollision(false);
        LegacyVehicle->SetActorTickEnabled(false);
        SetActorHiddenInGame(false);
        SetActorEnableCollision(true);
        // Standby keeps this pawn non-physical. Build the skeletal rigid body first,
        // then create the Chaos vehicle against that live body and its four wheel setups.
        if (USkeletalMeshComponent* VehicleMesh = GetMesh())
        {
            VehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            VehicleMesh->SetSimulatePhysics(true);
            VehicleMesh->RecreatePhysicsState();
            VehicleMesh->WakeAllRigidBodies();
        }
        if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
        {
            Movement->RecreatePhysicsState();
            if (!Movement->HasValidPhysicsState())
            {
                Movement->CreatePhysicsState();
            }
        }
        bTakeoverActive = true;
        MirrorSyncAccumulator = 0.0f;
        WheelEvidenceAccumulator = 0.0f;
        DamageEvidenceAccumulator = 0.0f;
        GTT_LOG( Log, TEXT("NATIVE_ROAD_TAKEOVER_ACTIVE vehicle=%s %s"), *NativeVehicleId.ToString(), *ImportSummary);
        return true;
    }
    return false;
}

void AGTTRoadVehicleNativePawn::DeactivateLegacyTakeover()
{
    if (!bTakeoverActive) return;
    if (bOccupied) ExitNativeVehicle();
    SyncLegacyMirror();
    if (AGTTVehicleBase* LegacyVehicle = LegacyMirror.Get())
    {
        LegacyVehicle->SetActorHiddenInGame(false);
        LegacyVehicle->SetActorEnableCollision(true);
        LegacyVehicle->SetActorTickEnabled(true);
    }
    LegacyMirror.Reset();
    bTakeoverActive = false;
    RuntimeWheelRisk = 0.0f;
    RuntimeWheelContacts = 0;
    RuntimeThrottleLimit = 1.0f;
    RuntimeSteeringLimit = 1.0f;
    RuntimeBrakeAssist = 0.0f;
    DamageThrottleLimit = 1.0f;
    DamageSteeringLimit = 1.0f;
    DamageSteeringBias = 0.0f;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void AGTTRoadVehicleNativePawn::SyncLegacyMirror()
{
    AGTTVehicleBase* LegacyVehicle = LegacyMirror.Get();
    if (!bTakeoverActive || !LegacyVehicle) return;
    LegacyVehicle->RestorePersistentState(GetActorTransform(), MigrationSnapshot.ConditionPercent, MigrationSnapshot.FuelLiters,
        MigrationSnapshot.bOwnedByPlayer, MigrationSnapshot.EngineUpgradeLevel, MigrationSnapshot.TireUpgradeLevel, MigrationSnapshot.TireIntegrity);
}

void AGTTRoadVehicleNativePawn::RuntimeAcceptanceGuard()
{
    if (!bTakeoverActive) return;
    FString Summary;
    if (!ConfigureAndValidateNativeRoadVehicle(Summary))
    {
        GTT_LOG( Error, TEXT("NATIVE_ROAD_FALLBACK vehicle=%s reason=%s"), *NativeVehicleId.ToString(), *Summary);
        DeactivateLegacyTakeover();
    }
}

void AGTTRoadVehicleNativePawn::UpdateNativeWheelRuntime(float DeltaSeconds)
{
    RuntimeWheelRisk = 0.0f;
    RuntimeWheelContacts = 0;
    RuntimeThrottleLimit = 1.0f;
    RuntimeSteeringLimit = 1.0f;
    RuntimeBrakeAssist = 0.0f;

    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
    if (!Movement || Movement->GetNumWheels() != 4)
    {
        return;
    }

    int32 ValidWheels = 0;
    int32 SlippingWheels = 0;
    int32 SkiddingWheels = 0;
    float MaxSlipMagnitude = 0.0f;
    float MaxSlipAngle = 0.0f;
    float MinSuspensionLength = 1.0f;
    float MaxSuspensionLength = 0.0f;

    for (int32 WheelIndex = 0; WheelIndex < 4; ++WheelIndex)
    {
        const FWheelStatus& WheelState = Movement->GetWheelState(WheelIndex);
        if (!WheelState.bIsValid)
        {
            continue;
        }

        ++ValidWheels;
        RuntimeWheelContacts += WheelState.bInContact ? 1 : 0;
        SlippingWheels += WheelState.bIsSlipping ? 1 : 0;
        SkiddingWheels += WheelState.bIsSkidding ? 1 : 0;
        MaxSlipMagnitude = FMath::Max(MaxSlipMagnitude, FMath::Abs(WheelState.SlipMagnitude));
        MaxSlipAngle = FMath::Max(MaxSlipAngle, FMath::Abs(WheelState.SlipAngle));
        if (WheelState.NormalizedSuspensionLength >= 0.0f)
        {
            MinSuspensionLength = FMath::Min(MinSuspensionLength, WheelState.NormalizedSuspensionLength);
            MaxSuspensionLength = FMath::Max(MaxSuspensionLength, WheelState.NormalizedSuspensionLength);
        }
    }

    if (ValidWheels != 4)
    {
        return;
    }

    const float ContactRisk = FMath::Clamp((4.0f - static_cast<float>(RuntimeWheelContacts)) / 3.0f, 0.0f, 1.0f);
    const float SlipRisk = FMath::Clamp(static_cast<float>(SlippingWheels) / 3.0f, 0.0f, 1.0f);
    const float SkidRisk = FMath::Clamp(static_cast<float>(SkiddingWheels) / 2.0f, 0.0f, 1.0f);
    const float MagnitudeRisk = FMath::Clamp(MaxSlipMagnitude / 650.0f, 0.0f, 1.0f);
    const float AngleRisk = FMath::Clamp(MaxSlipAngle / 32.0f, 0.0f, 1.0f);
    const float SuspensionRisk = FMath::Clamp((MaxSuspensionLength - MinSuspensionLength) / 0.55f, 0.0f, 1.0f) * 0.72f;
    const float RawRisk = FMath::Max3(ContactRisk, SkidRisk,
        FMath::Max(SlipRisk, FMath::Max(MagnitudeRisk, FMath::Max(AngleRisk, SuspensionRisk))));
    const float TirePenalty = 1.0f - FMath::Clamp(MigrationSnapshot.TireIntegrity, 0.0f, 1.0f);
    const float TuneAssist = FMath::Clamp(static_cast<float>(MigrationSnapshot.TireUpgradeLevel) * 0.035f, 0.0f, 0.105f);
    const float RearBodyRisk = (1.0f - BodyDamage.RearHealth) * 0.14f + static_cast<float>(BodyDamage.DetachedPanelCount) * 0.015f;
    RuntimeWheelRisk = FMath::Clamp(RawRisk + TirePenalty * 0.22f + RearBodyRisk - TuneAssist, 0.0f, 1.0f);

    if (RuntimeWheelRisk >= RuntimeTractionRiskThreshold)
    {
        RuntimeThrottleLimit = FMath::Clamp(0.96f - RuntimeWheelRisk * 0.62f + TuneAssist, 0.24f, 0.90f);
        RuntimeBrakeAssist = RuntimeWheelRisk >= 0.62f
            ? FMath::Clamp(0.03f + RuntimeWheelRisk * 0.18f, 0.0f, 0.22f)
            : 0.0f;
        RuntimeSteeringLimit = RuntimeWheelRisk >= 0.78f
            ? FMath::Clamp(1.20f - RuntimeWheelRisk * 0.55f, 0.55f, 1.0f)
            : 1.0f;
    }

    WheelEvidenceAccumulator += DeltaSeconds;
    if (WheelEvidenceAccumulator >= WheelEvidenceIntervalSeconds)
    {
        WheelEvidenceAccumulator = 0.0f;
        GTT_LOG( Log,
            TEXT("NATIVE_ROAD_WHEEL_STATE_EVIDENCE vehicle=%s contacts=%d/4 slipping=%d skidding=%d slip_mag=%.2f slip_angle=%.2f suspension_spread=%.2f risk=%.2f throttle_limit=%.2f brake_assist=%.2f steering_limit=%.2f tire_integrity=%.2f tire_level=%d rear_body=%.2f"),
            *NativeVehicleId.ToString(),
            RuntimeWheelContacts,
            SlippingWheels,
            SkiddingWheels,
            MaxSlipMagnitude,
            MaxSlipAngle,
            MaxSuspensionLength - MinSuspensionLength,
            RuntimeWheelRisk,
            RuntimeThrottleLimit,
            RuntimeBrakeAssist,
            RuntimeSteeringLimit,
            MigrationSnapshot.TireIntegrity,
            MigrationSnapshot.TireUpgradeLevel,
            BodyDamage.RearHealth);
    }
}

void AGTTRoadVehicleNativePawn::UpdateDamageConsequences(float DeltaSeconds)
{
    const float FrontDamage = 1.0f - FMath::Clamp(BodyDamage.FrontHealth, 0.0f, 1.0f);
    const float SideDamage = 1.0f - FMath::Min(BodyDamage.LeftHealth, BodyDamage.RightHealth);
    const float ThrottleLoad = FMath::Clamp(FMath::Abs(LastThrottleInput), 0.0f, 1.0f);
    const bool bCoolingUnderLoad = bOccupied && BodyDamage.FrontHealth < 0.55f && ThrottleLoad > 0.50f;

    if (bCoolingUnderLoad)
    {
        const float CoolingDamage = FMath::Clamp((0.55f - BodyDamage.FrontHealth) / 0.55f, 0.0f, 1.0f);
        BodyDamage.CoolingStress = FMath::Clamp(
            BodyDamage.CoolingStress + DeltaSeconds * (0.035f + CoolingDamage * 0.15f + CargoLoadFactor * 0.025f),
            0.0f, 1.0f);
    }
    else
    {
        BodyDamage.CoolingStress = FMath::Max(0.0f, BodyDamage.CoolingStress - DeltaSeconds * 0.055f);
    }

    if (BodyDamage.CoolingStress >= 0.92f && bOccupied && ThrottleLoad > 0.65f)
    {
        MigrationSnapshot.ConditionPercent = FMath::Max(0.0f, MigrationSnapshot.ConditionPercent - DeltaSeconds * 0.0025f);
    }

    DamageThrottleLimit = FMath::Clamp(1.0f - FrontDamage * 0.22f - BodyDamage.CoolingStress * 0.30f, 0.46f, 1.0f);
    DamageSteeringLimit = FMath::Clamp(
        1.0f - SideDamage * 0.30f - static_cast<float>(BodyDamage.DetachedPanelCount) * 0.025f,
        0.56f, 1.0f);
    DamageSteeringBias = FMath::Clamp((BodyDamage.LeftHealth - BodyDamage.RightHealth) * 0.11f, -0.12f, 0.12f);

    if (MigrationSnapshot.ConditionPercent <= KINDA_SMALL_NUMBER)
    {
        StopNativeDriveForBreakdown();
    }

    DamageEvidenceAccumulator += DeltaSeconds;
    if (DamageEvidenceAccumulator >= DamageEvidenceIntervalSeconds)
    {
        DamageEvidenceAccumulator = 0.0f;
        GTT_LOG( Log,
            TEXT("NATIVE_ROAD_DAMAGE_DYNAMICS vehicle=%s front=%.2f rear=%.2f left=%.2f right=%.2f cooling=%.2f detached=%d power_limit=%.2f steering_limit=%.2f steering_bias=%.2f repair_surcharge=%d"),
            *NativeVehicleId.ToString(), BodyDamage.FrontHealth, BodyDamage.RearHealth,
            BodyDamage.LeftHealth, BodyDamage.RightHealth, BodyDamage.CoolingStress,
            BodyDamage.DetachedPanelCount, DamageThrottleLimit, DamageSteeringLimit,
            DamageSteeringBias, GetBodyDamageRepairSurcharge());
    }
}

EGTTRoadDamageZone AGTTRoadVehicleNativePawn::DetermineImpactZone(const FVector& HitLocation) const
{
    const FVector LocalHit = GetActorTransform().InverseTransformPosition(HitLocation);
    if (FMath::Abs(LocalHit.X) >= FMath::Abs(LocalHit.Y))
    {
        return LocalHit.X >= 0.0f ? EGTTRoadDamageZone::Front : EGTTRoadDamageZone::Rear;
    }
    return LocalHit.Y >= 0.0f ? EGTTRoadDamageZone::Right : EGTTRoadDamageZone::Left;
}

const TCHAR* AGTTRoadVehicleNativePawn::DamageZoneToString(EGTTRoadDamageZone Zone)
{
    switch (Zone)
    {
        case EGTTRoadDamageZone::Front: return TEXT("FRONT");
        case EGTTRoadDamageZone::Rear: return TEXT("REAR");
        case EGTTRoadDamageZone::Left: return TEXT("LEFT");
        case EGTTRoadDamageZone::Right: return TEXT("RIGHT");
        default: return TEXT("UNKNOWN");
    }
}

float& AGTTRoadVehicleNativePawn::ResolveDamageZoneHealth(EGTTRoadDamageZone Zone)
{
    switch (Zone)
    {
        case EGTTRoadDamageZone::Front: return BodyDamage.FrontHealth;
        case EGTTRoadDamageZone::Rear: return BodyDamage.RearHealth;
        case EGTTRoadDamageZone::Left: return BodyDamage.LeftHealth;
        case EGTTRoadDamageZone::Right: return BodyDamage.RightHealth;
        default: return BodyDamage.FrontHealth;
    }
}

float AGTTRoadVehicleNativePawn::GetDamageZoneHealth(EGTTRoadDamageZone Zone) const
{
    switch (Zone)
    {
        case EGTTRoadDamageZone::Front: return BodyDamage.FrontHealth;
        case EGTTRoadDamageZone::Rear: return BodyDamage.RearHealth;
        case EGTTRoadDamageZone::Left: return BodyDamage.LeftHealth;
        case EGTTRoadDamageZone::Right: return BodyDamage.RightHealth;
        default: return 1.0f;
    }
}

void AGTTRoadVehicleNativePawn::ApplyNativeImpactDamage(float ImpactSpeedKmh, EGTTRoadDamageZone Zone, const FVector& HitLocation, const FVector& NormalImpulse)
{
    if (!bNativeReady || !bTakeoverActive || ImpactSpeedKmh < MinimumImpactSpeedKmh)
    {
        return;
    }

    const float Severity = FMath::Clamp((ImpactSpeedKmh - MinimumImpactSpeedKmh) / 70.0f, 0.0f, 1.6f);
    const float CargoInertia = 1.0f + CargoLoadFactor * 0.25f;
    const float DamageScale = FMath::Max(0.1f, GetImpactDamageScale()) * CargoInertia;
    const float BodyDamageRatio = Severity * 0.18f * DamageScale;
    MigrationSnapshot.ConditionPercent = FMath::Clamp(MigrationSnapshot.ConditionPercent - BodyDamageRatio, 0.0f, 1.0f);

    float ZoneScale = 0.30f;
    if (Zone == EGTTRoadDamageZone::Front) ZoneScale = 0.36f;
    else if (Zone == EGTTRoadDamageZone::Rear) ZoneScale = 0.27f;

    float& ZoneHealth = ResolveDamageZoneHealth(Zone);
    const float PreviousZoneHealth = ZoneHealth;
    ZoneHealth = FMath::Clamp(ZoneHealth - Severity * ZoneScale * DamageScale, 0.0f, 1.0f);

    if (ImpactSpeedKmh > SevereImpactSpeedKmh)
    {
        const float SideTireMultiplier = (Zone == EGTTRoadDamageZone::Left || Zone == EGTTRoadDamageZone::Right) ? 1.28f : 1.0f;
        const float TireDamage = Severity * 0.075f * DamageScale * SideTireMultiplier;
        MigrationSnapshot.TireIntegrity = FMath::Clamp(MigrationSnapshot.TireIntegrity - TireDamage, 0.0f, 1.0f);
    }

    TryDetachDamagePanel(Zone, HitLocation, NormalImpulse, ImpactSpeedKmh);

    GTT_LOG( Warning,
        TEXT("NATIVE_ROAD_DAMAGE_ZONE vehicle=%s zone=%s speed_kmh=%.1f zone_health=%.2f zone_delta=%.3f condition=%.2f tires=%.2f detached=%d"),
        *NativeVehicleId.ToString(), DamageZoneToString(Zone), ImpactSpeedKmh,
        ZoneHealth, PreviousZoneHealth - ZoneHealth, MigrationSnapshot.ConditionPercent,
        MigrationSnapshot.TireIntegrity, BodyDamage.DetachedPanelCount);

    if (MigrationSnapshot.ConditionPercent <= KINDA_SMALL_NUMBER)
    {
        StopNativeDriveForBreakdown();
        GTT_LOG( Warning, TEXT("NATIVE_ROAD_BREAKDOWN vehicle=%s impact_speed_kmh=%.1f"), *NativeVehicleId.ToString(), ImpactSpeedKmh);
    }
}

void AGTTRoadVehicleNativePawn::TryDetachDamagePanel(EGTTRoadDamageZone Zone, const FVector& HitLocation, const FVector& NormalImpulse, float ImpactSpeedKmh)
{
    if (ImpactSpeedKmh < PanelDetachMinimumSpeedKmh)
    {
        return;
    }

    UStaticMeshComponent* Panel = nullptr;
    bool* bDetached = nullptr;
    float Threshold = 0.40f;
    switch (Zone)
    {
        case EGTTRoadDamageZone::Front: Panel = FrontDamageDebris; bDetached = &bFrontPanelDetached; Threshold = 0.48f; break;
        case EGTTRoadDamageZone::Rear: Panel = RearDamageDebris; bDetached = &bRearPanelDetached; Threshold = 0.42f; break;
        case EGTTRoadDamageZone::Left: Panel = LeftDamageDebris; bDetached = &bLeftPanelDetached; Threshold = 0.38f; break;
        case EGTTRoadDamageZone::Right: Panel = RightDamageDebris; bDetached = &bRightPanelDetached; Threshold = 0.38f; break;
        default: return;
    }

    if (!Panel || !bDetached || *bDetached || GetDamageZoneHealth(Zone) > Threshold)
    {
        return;
    }

    *bDetached = true;
    ++BodyDamage.DetachedPanelCount;
    Panel->SetHiddenInGame(false, true);
    Panel->SetVisibility(true, true);
    Panel->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Panel->SetWorldLocation(HitLocation);
    Panel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Panel->SetSimulatePhysics(true);
    if (NormalImpulse.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        Panel->AddImpulseAtLocation(NormalImpulse.GetClampedToMaxSize(180000.0f), HitLocation);
    }

    GTT_LOG( Warning,
        TEXT("NATIVE_ROAD_PANEL_DETACH vehicle=%s zone=%s speed_kmh=%.1f detached=%d"),
        *NativeVehicleId.ToString(), DamageZoneToString(Zone), ImpactSpeedKmh, BodyDamage.DetachedPanelCount);
}

void AGTTRoadVehicleNativePawn::ConfigureDamageDebrisLayout()
{
    const bool bMulebox = NativeVehicleId == FName(TEXT("Mulebox1200"));
    const float FrontX = bMulebox ? 168.0f : 146.0f;
    const float RearX = bMulebox ? -160.0f : -144.0f;
    const float SideY = bMulebox ? 105.0f : 96.0f;
    const float SideZ = bMulebox ? 96.0f : 72.0f;

    if (FrontDamageDebris)
    {
        FrontDamageDebris->SetRelativeLocation(FVector(FrontX, 0.0f, bMulebox ? 18.0f : 12.0f));
        FrontDamageDebris->SetRelativeScale3D(FVector(0.10f, bMulebox ? 1.05f : 0.96f, 0.13f));
    }
    if (RearDamageDebris)
    {
        RearDamageDebris->SetRelativeLocation(FVector(RearX, 0.0f, bMulebox ? 70.0f : 35.0f));
        RearDamageDebris->SetRelativeScale3D(FVector(0.09f, bMulebox ? 0.95f : 0.90f, bMulebox ? 0.48f : 0.18f));
    }
    if (LeftDamageDebris)
    {
        LeftDamageDebris->SetRelativeLocation(FVector(bMulebox ? -25.0f : -5.0f, -SideY, SideZ));
        LeftDamageDebris->SetRelativeScale3D(FVector(bMulebox ? 0.74f : 0.70f, 0.07f, bMulebox ? 0.62f : 0.30f));
    }
    if (RightDamageDebris)
    {
        RightDamageDebris->SetRelativeLocation(FVector(bMulebox ? -25.0f : -5.0f, SideY, SideZ));
        RightDamageDebris->SetRelativeScale3D(FVector(bMulebox ? 0.74f : 0.70f, 0.07f, bMulebox ? 0.62f : 0.30f));
    }
}

void AGTTRoadVehicleNativePawn::RestoreNativeBodyDamage()
{
    BodyDamage = FGTTRoadBodyDamageSnapshot();
    bFrontPanelDetached = false;
    bRearPanelDetached = false;
    bLeftPanelDetached = false;
    bRightPanelDetached = false;
    DamageThrottleLimit = 1.0f;
    DamageSteeringLimit = 1.0f;
    DamageSteeringBias = 0.0f;

    UStaticMeshComponent* Panels[] = {FrontDamageDebris, RearDamageDebris, LeftDamageDebris, RightDamageDebris};
    for (UStaticMeshComponent* Panel : Panels)
    {
        if (!Panel) continue;
        Panel->SetSimulatePhysics(false);
        Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Panel->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
        Panel->SetVisibility(false, true);
        Panel->SetHiddenInGame(true, true);
    }
    ConfigureDamageDebrisLayout();
}

bool AGTTRoadVehicleNativePawn::NeedsNativeWorkshopService() const
{
    const bool bBodyDamaged = BodyDamage.FrontHealth < 0.999f || BodyDamage.RearHealth < 0.999f ||
        BodyDamage.LeftHealth < 0.999f || BodyDamage.RightHealth < 0.999f ||
        BodyDamage.CoolingStress > 0.01f || BodyDamage.DetachedPanelCount > 0;
    return MigrationSnapshot.ConditionPercent < 0.999f ||
        MigrationSnapshot.FuelLiters + KINDA_SMALL_NUMBER < FuelCapacityLiters ||
        MigrationSnapshot.TireIntegrity < 0.999f || bBodyDamaged;
}

int32 AGTTRoadVehicleNativePawn::GetBodyDamageRepairSurcharge() const
{
    const float BodyLoss = ((1.0f - BodyDamage.FrontHealth) + (1.0f - BodyDamage.RearHealth) +
        (1.0f - BodyDamage.LeftHealth) + (1.0f - BodyDamage.RightHealth)) * 0.25f;
    const float RawCost = BodyLoss * 120.0f + static_cast<float>(BodyDamage.DetachedPanelCount) * 35.0f + BodyDamage.CoolingStress * 30.0f;
    return FMath::Clamp(FMath::RoundToInt(RawCost / 5.0f) * 5, 0, 250);
}

bool AGTTRoadVehicleNativePawn::ApplyNativeWorkshopService()
{
    AGTTVehicleBase* LegacyVehicle = LegacyMirror.Get();
    if (!bTakeoverActive || !LegacyVehicle)
    {
        return false;
    }

    LegacyVehicle->RepairVehicle(100000.0f);
    LegacyVehicle->RefuelVehicle(100000.0f);
    LegacyVehicle->RepairTires();

    FString ImportSummary;
    if (!ImportLegacyGameplayState(LegacyVehicle, ImportSummary))
    {
        return false;
    }

    RestoreNativeBodyDamage();
    SyncLegacyMirror();
    GTT_LOG( Log, TEXT("NATIVE_ROAD_WORKSHOP_RESTORE vehicle=%s %s"), *NativeVehicleId.ToString(), *ImportSummary);
    return true;
}

void AGTTRoadVehicleNativePawn::StopNativeDriveForBreakdown()
{
    LastThrottleInput = 0.0f;
    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetBrakeInput(1.0f);
    }
}

bool AGTTRoadVehicleNativePawn::ApplyAcceptanceDriveCommand(float Throttle, float Steering, float Brake)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario")) || !bNativeReady || !bTakeoverActive)
    {
        return false;
    }

    bAcceptanceDriveCommandActive = true;
    LastThrottleInput = FMath::Clamp(Throttle, -1.0f, 1.0f);
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive())
    {
        return false;
    }

    const float SpeedKmh = GetVelocity().Size() * 0.036f;
    const float TunePower = 1.0f + FMath::Clamp(MigrationSnapshot.EngineUpgradeLevel, 0, 3) * 0.08f;
    const float ConditionPower = FMath::Lerp(0.35f, 1.0f, FMath::Clamp(MigrationSnapshot.ConditionPercent, 0.0f, 1.0f));
    const float ScaledThrottle = FMath::Clamp(
        FMath::Abs(LastThrottleInput) * TunePower * ConditionPower * GetCargoPowerLimit(SpeedKmh) * RuntimeThrottleLimit * DamageThrottleLimit,
        0.0f,
        1.0f);
    const float TireGrip = FMath::Lerp(0.45f, 1.0f, FMath::Clamp(MigrationSnapshot.TireIntegrity, 0.0f, 1.0f));
    const float TuneGrip = 1.0f + FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3) * 0.05f;
    Movement->SetThrottleInput(ScaledThrottle);
    Movement->SetSteeringInput(FMath::Clamp(
        (Steering * DamageSteeringLimit + DamageSteeringBias) * TireGrip * TuneGrip * GetCargoSteeringLimit(SpeedKmh) * RuntimeSteeringLimit,
        -1.0f,
        1.0f));
    Movement->SetBrakeInput(FMath::Clamp(FMath::Max(Brake, RuntimeBrakeAssist), 0.0f, 1.0f));
    if (!FMath::IsNearlyZero(LastThrottleInput))
    {
        Movement->SetTargetGear(LastThrottleInput < 0.0f ? -1 : 1, true);
    }
    return true;
}

void AGTTRoadVehicleNativePawn::HandleNativeThrottle(float Value)
{
    LastThrottleInput = FMath::Clamp(Value, -1.0f, 1.0f);
    if (!bNativeReady || !bTakeoverActive || !bOccupied || MigrationSnapshot.FuelLiters <= KINDA_SMALL_NUMBER || MigrationSnapshot.ConditionPercent <= 0.0f)
    {
        LastThrottleInput = 0.0f;
        StopNativeDriveForBreakdown();
        return;
    }

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        const float SpeedKmh = GetVelocity().Size() * 0.036f;
        const float TunePower = 1.0f + FMath::Clamp(MigrationSnapshot.EngineUpgradeLevel, 0, 3) * 0.08f;
        const float ConditionPower = FMath::Lerp(0.35f, 1.0f, FMath::Clamp(MigrationSnapshot.ConditionPercent, 0.0f, 1.0f));
        const float Requested = LastThrottleInput;
        const float Scaled = FMath::Clamp(
            FMath::Abs(Requested) * TunePower * ConditionPower * GetCargoPowerLimit(SpeedKmh) * RuntimeThrottleLimit * DamageThrottleLimit,
            0.0f,
            1.0f);
        Movement->SetBrakeInput(FMath::Max(FMath::IsNearlyZero(Requested) ? 0.15f : 0.0f, RuntimeBrakeAssist));
        Movement->SetThrottleInput(Scaled);
        Movement->SetTargetGear(Requested < -KINDA_SMALL_NUMBER ? -1 : 1, true);
    }
}

void AGTTRoadVehicleNativePawn::HandleNativeSteering(float Value)
{
    if (!bNativeReady || !bTakeoverActive || !bOccupied) return;
    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
    {
        const float SpeedKmh = GetVelocity().Size() * 0.036f;
        const float TireGrip = FMath::Lerp(0.45f, 1.0f, FMath::Clamp(MigrationSnapshot.TireIntegrity, 0.0f, 1.0f));
        const float TuneGrip = 1.0f + FMath::Clamp(MigrationSnapshot.TireUpgradeLevel, 0, 3) * 0.05f;
        const float DamageAdjustedInput = Value * DamageSteeringLimit + DamageSteeringBias;
        Movement->SetSteeringInput(FMath::Clamp(
            DamageAdjustedInput * TireGrip * TuneGrip * GetCargoSteeringLimit(SpeedKmh) * RuntimeSteeringLimit,
            -1.0f,
            1.0f));
    }
}

void AGTTRoadVehicleNativePawn::SetCargoLoadFactor(float NewLoadFactor)
{
    CargoLoadFactor = FMath::Clamp(NewLoadFactor, 0.0f, 1.0f);
    GTT_LOG( Log, TEXT("NATIVE_ROAD_CARGO vehicle=%s load=%.2f"), *NativeVehicleId.ToString(), CargoLoadFactor);
}

float AGTTRoadVehicleNativePawn::GetCargoPowerLimit(float SpeedKmh) const { return 1.0f; }
float AGTTRoadVehicleNativePawn::GetCargoSteeringLimit(float SpeedKmh) const { return 1.0f; }
float AGTTRoadVehicleNativePawn::GetImpactDamageScale() const { return 1.0f; }

float AGTTRattlebackNativePawn::GetImpactDamageScale() const
{
    return 1.05f;
}

float AGTTMuleboxNativePawn::GetCargoPowerLimit(float SpeedKmh) const
{
    return FMath::Lerp(1.0f, SpeedKmh > 75.0f ? 0.74f : 0.88f, GetCargoLoadFactor());
}

float AGTTMuleboxNativePawn::GetCargoSteeringLimit(float SpeedKmh) const
{
    const float SpeedRisk = FMath::Clamp((SpeedKmh - 45.0f) / 55.0f, 0.0f, 1.0f);
    return 1.0f - GetCargoLoadFactor() * SpeedRisk * 0.42f;
}

float AGTTMuleboxNativePawn::GetImpactDamageScale() const
{
    return 0.92f;
}
