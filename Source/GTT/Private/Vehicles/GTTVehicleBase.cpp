#include "Vehicles/GTTVehicleBase.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Radio/GTTRadioComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

AGTTVehicleBase::AGTTVehicleBase()
{
    PrimaryActorTick.bCanEverTick = true;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    SetRootComponent(VehicleMesh);
    VehicleMesh->SetSimulatePhysics(true);
    VehicleMesh->SetNotifyRigidBodyCollision(true);
    VehicleMesh->SetLinearDamping(1.4f);
    VehicleMesh->SetAngularDamping(2.5f);
    VehicleMesh->OnComponentHit.AddDynamic(this, &AGTTVehicleBase::HandleVehicleHit);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(VehicleMesh);
    CameraBoom->TargetArmLength = 560.0f;
    CameraBoom->bUsePawnControlRotation = true;

    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    VehicleCamera->bUsePawnControlRotation = false;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;

    DamageSmokePuffA = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DamageSmokePuffA"));
    DamageSmokePuffA->SetupAttachment(VehicleMesh);
    DamageSmokePuffB = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DamageSmokePuffB"));
    DamageSmokePuffB->SetupAttachment(VehicleMesh);
    DamageSmokePuffC = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DamageSmokePuffC"));
    DamageSmokePuffC->SetupAttachment(VehicleMesh);

    for (UStaticMeshComponent* Puff : {DamageSmokePuffA.Get(), DamageSmokePuffB.Get(), DamageSmokePuffC.Get()})
    {
        if (Puff)
        {
            Puff->SetStaticMesh(SphereMesh);
            Puff->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Puff->SetGenerateOverlapEvents(false);
            Puff->SetVisibility(false, true);
            Puff->SetCastShadow(false);
        }
    }

    VehicleDisplayName = NSLOCTEXT("GTT", "DefaultVehicleName", "Old vehicle");
}

void AGTTVehicleBase::BeginPlay()
{
    Super::BeginPlay();
    CurrentFuelLiters = FMath::Clamp(StartingFuelLiters, 0.0f, FuelCapacityLiters);
    EngineTemperatureC = NormalEngineTemperatureC;
    TireIntegrity = FMath::Clamp(TireIntegrity, 0.0f, 1.0f);
    UpdateBreakableParts();
}

void AGTTVehicleBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FaultRestartTimeRemaining = FMath::Max(0.0f, FaultRestartTimeRemaining - DeltaSeconds);
    UpdateDamageSmoke(DeltaSeconds);

    if (!bEngineRunning || !bOccupied || CurrentFuelLiters <= 0.0f)
    {
        EngineTemperatureC = FMath::FInterpTo(EngineTemperatureC, NormalEngineTemperatureC, DeltaSeconds, 0.18f);
        return;
    }

    const float ConditionAlpha = GetConditionPercent();
    const float ThrottleAlpha = FMath::Clamp(FMath::Abs(LastThrottleInput), 0.0f, 1.0f);
    const float EfficiencyBonus = 1.0f - EngineUpgradeLevel * 0.035f;
    const float BurnRate = FMath::Lerp(IdleFuelBurnPerSecond, FullThrottleFuelBurnPerSecond, ThrottleAlpha) * EfficiencyBonus;
    CurrentFuelLiters = FMath::Max(0.0f, CurrentFuelLiters - BurnRate * DeltaSeconds);

    const float DamageHeat = (1.0f - ConditionAlpha) * 42.0f;
    const float UpgradeCooling = EngineUpgradeLevel * 3.0f;
    const float TargetTemperature = NormalEngineTemperatureC + ThrottleAlpha * 31.0f + DamageHeat - UpgradeCooling;
    const float HeatInterpSpeed = TargetTemperature > EngineTemperatureC ? 0.34f : 0.16f;
    EngineTemperatureC = FMath::FInterpTo(EngineTemperatureC, TargetTemperature, DeltaSeconds, HeatInterpSpeed);

    if (CurrentFuelLiters <= KINDA_SMALL_NUMBER)
    {
        CurrentFuelLiters = 0.0f;
        LastThrottleInput = 0.0f;
        ActiveFaultStatus = TEXT("OUT OF FUEL");
        SetEngineRunning(false);
        OnOutOfFuel.Broadcast();
        return;
    }

    if (EngineTemperatureC >= CriticalEngineTemperatureC)
    {
        ApplyVehicleDamage(2.5f * DeltaSeconds);
        TriggerMechanicalStall(TEXT("ENGINE OVERHEAT"));
        return;
    }

    if (ConditionAlpha < 0.50f && ThrottleAlpha > 0.25f && FaultRestartTimeRemaining <= 0.0f)
    {
        const float DamageSeverity = 1.0f - FMath::Clamp(ConditionAlpha / 0.50f, 0.0f, 1.0f);
        const float UpgradeReliability = 1.0f - EngineUpgradeLevel * 0.16f;
        const float StallChance = LowConditionFaultChancePerSecond * UpgradeReliability * DamageSeverity * (0.35f + 0.65f * ThrottleAlpha) * DeltaSeconds;
        if (FMath::FRand() < StallChance)
        {
            TriggerMechanicalStall(TEXT("ENGINE STALL"));
        }
    }
}

void AGTTVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGTTVehicleBase::HandleThrottle);
    PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGTTVehicleBase::HandleSteering);
    PlayerInputComponent->BindAction(TEXT("ExitVehicle"), IE_Pressed, this, &AGTTVehicleBase::ExitVehicle);
    PlayerInputComponent->BindAction(TEXT("RadioNext"), IE_Pressed, this, &AGTTVehicleBase::CycleRadio);
}

void AGTTVehicleBase::Interact_Implementation(AActor* Interactor)
{
    if (bOccupied || !IsValid(Interactor) || Condition <= 0.0f)
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

    if (bIllegalToTake && !bOwnedByPlayer && !bTheftReported)
    {
        if (UGTTWantedComponent* Wanted = InteractingPawn->FindComponentByClass<UGTTWantedComponent>())
        {
            Wanted->AddHeat(TheftHeat);
        }

        bTheftReported = true;
        OnVehicleStolen.Broadcast();

        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->NotifyVehicleStolen(this, InteractingPawn);
        }
    }

    PreviousPawn = InteractingPawn;
    InteractingPawn->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    InteractingPawn->SetActorHiddenInGame(true);
    InteractingPawn->SetActorEnableCollision(false);

    InteractingController->Possess(this);
    bOccupied = true;
    ActiveFaultStatus.Empty();
    SetEngineRunning(true);
    OnDriverEntered.Broadcast(InteractingPawn);
}

FText AGTTVehicleBase::GetInteractionText_Implementation() const
{
    if (bOccupied) return NSLOCTEXT("GTT", "VehicleOccupied", "Occupied");
    if (Condition <= 0.0f) return NSLOCTEXT("GTT", "VehicleBroken", "Broken down");
    if (CurrentFuelLiters <= KINDA_SMALL_NUMBER)
    {
        return FText::Format(NSLOCTEXT("GTT", "EnterEmptyVehicle", "Enter {0} (empty tank)"), VehicleDisplayName);
    }
    return FText::Format(
        bOwnedByPlayer ? NSLOCTEXT("GTT", "EnterOwnedVehicle", "Enter your {0}") : NSLOCTEXT("GTT", "EnterNamedVehicle", "Enter {0}"),
        VehicleDisplayName);
}

void AGTTVehicleBase::ExitVehicle()
{
    AController* VehicleController = GetController();
    APawn* PawnToRestore = PreviousPawn.Get();
    if (!VehicleController || !PawnToRestore) return;

    LastThrottleInput = 0.0f;
    SetEngineRunning(false);
    PawnToRestore->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    PawnToRestore->SetActorLocation(GetActorTransform().TransformPosition(ExitOffset));
    PawnToRestore->SetActorRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
    PawnToRestore->SetActorHiddenInGame(false);
    PawnToRestore->SetActorEnableCollision(true);
    VehicleController->Possess(PawnToRestore);
    OnDriverExited.Broadcast(PawnToRestore);
    PreviousPawn.Reset();
    bOccupied = false;
}

void AGTTVehicleBase::ApplyVehicleDamage(float DamageAmount)
{
    if (DamageAmount <= 0.0f || Condition <= 0.0f) return;

    const float OldCondition = Condition;
    Condition = FMath::Clamp(Condition - DamageAmount, 0.0f, MaxCondition);
    UpdateBreakableParts();

    if (OldCondition > 0.0f && Condition <= 0.0f)
    {
        LastThrottleInput = 0.0f;
        ActiveFaultStatus = TEXT("BROKEN DOWN");
        SetEngineRunning(false);
        UE_LOG(LogGTT, Warning, TEXT("Vehicle %s broke down."), *GetName());
        OnVehicleBrokenDown();
    }
}

void AGTTVehicleBase::RepairVehicle(float RepairAmount)
{
    if (RepairAmount <= 0.0f) return;

    Condition = FMath::Clamp(Condition + RepairAmount, 0.0f, MaxCondition);
    EngineTemperatureC = FMath::Min(EngineTemperatureC, NormalEngineTemperatureC + 5.0f);
    ActiveFaultStatus.Empty();
    if (GetConditionPercent() >= 0.88f) RestoreBreakableParts(); else UpdateBreakableParts();
}

void AGTTVehicleBase::RefuelVehicle(float Liters)
{
    if (Liters <= 0.0f) return;
    CurrentFuelLiters = FMath::Clamp(CurrentFuelLiters + Liters, 0.0f, FuelCapacityLiters);
    if (CurrentFuelLiters > KINDA_SMALL_NUMBER && ActiveFaultStatus == TEXT("OUT OF FUEL")) ActiveFaultStatus.Empty();
}

void AGTTVehicleBase::MarkOwnedByPlayer()
{
    bOwnedByPlayer = true;
    bIllegalToTake = false;
    bTheftReported = false;
}

void AGTTVehicleBase::RestorePersistentState(const FTransform& InTransform, float ConditionPercent, float FuelLiters, bool bOwned,
    int32 InEngineUpgradeLevel, int32 InTireUpgradeLevel, float InTireIntegrity)
{
    if (bOccupied) ExitVehicle();

    SetEngineRunning(false);
    SetActorTransform(InTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Condition = FMath::Clamp(ConditionPercent, 0.0f, 1.0f) * MaxCondition;
    CurrentFuelLiters = FMath::Clamp(FuelLiters, 0.0f, FuelCapacityLiters);
    EngineUpgradeLevel = FMath::Clamp(InEngineUpgradeLevel, 0, 3);
    TireUpgradeLevel = FMath::Clamp(InTireUpgradeLevel, 0, 3);
    TireIntegrity = FMath::Clamp(InTireIntegrity, 0.0f, 1.0f);
    EngineTemperatureC = NormalEngineTemperatureC;
    ActiveFaultStatus.Empty();
    bOwnedByPlayer = bOwned;
    if (bOwnedByPlayer)
    {
        bIllegalToTake = false;
        bTheftReported = false;
    }
    RestoreBreakableParts();
    UpdateBreakableParts();
}

bool AGTTVehicleBase::RecallToTransform(const FTransform& Destination)
{
    if (bOccupied || !bOwnedByPlayer || !VehicleMesh) return false;
    SetEngineRunning(false);
    VehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
    VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    SetActorTransform(Destination, false, nullptr, ETeleportType::TeleportPhysics);
    return true;
}

bool AGTTVehicleBase::InstallEngineUpgrade()
{
    if (!bOwnedByPlayer || EngineUpgradeLevel >= 3) return false;
    ++EngineUpgradeLevel;
    EngineTemperatureC = FMath::Min(EngineTemperatureC, NormalEngineTemperatureC);
    return true;
}

bool AGTTVehicleBase::InstallTireUpgrade()
{
    if (!bOwnedByPlayer || TireUpgradeLevel >= 3) return false;
    ++TireUpgradeLevel;
    TireIntegrity = 1.0f;
    return true;
}

void AGTTVehicleBase::RepairTires()
{
    TireIntegrity = 1.0f;
}

void AGTTVehicleBase::ApplyTireDamage(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }
    const float Reinforcement = 1.0f + TireUpgradeLevel * 0.35f;
    TireIntegrity = FMath::Clamp(TireIntegrity - Amount / Reinforcement, 0.0f, 1.0f);
}

float AGTTVehicleBase::GetConditionPercent() const
{
    return MaxCondition > 0.0f ? Condition / MaxCondition : 0.0f;
}

float AGTTVehicleBase::GetSpeedKmh() const
{
    return GetVelocity().Size() * 0.036f;
}

float AGTTVehicleBase::GetFuelPercent() const
{
    return FuelCapacityLiters > 0.0f ? CurrentFuelLiters / FuelCapacityLiters : 0.0f;
}

FString AGTTVehicleBase::GetTuningSummary() const
{
    return FString::Printf(TEXT("ENGINE L%d/3 | TIRES L%d/3 | TIRE %.0f%%"), EngineUpgradeLevel, TireUpgradeLevel, TireIntegrity * 100.0f);
}

FString AGTTVehicleBase::GetFaultStatusText() const
{
    if (Condition <= 0.0f) return TEXT("BROKEN DOWN");
    if (TireIntegrity <= 0.08f) return TEXT("FLAT TIRE");
    if (!ActiveFaultStatus.IsEmpty()) return ActiveFaultStatus;
    if (EngineTemperatureC >= OverheatStartTemperatureC) return TEXT("OVERHEATING");
    if (DetachedPartCount > 0) return FString::Printf(TEXT("BODY PARTS LOST: %d"), DetachedPartCount);
    if (GetConditionPercent() < 0.35f) return TEXT("ROUGH ENGINE");
    return FString();
}

void AGTTVehicleBase::HandleThrottle(float Value)
{
    LastThrottleInput = Value;

    if (!bEngineRunning && bOccupied && Value > 0.20f && FaultRestartTimeRemaining <= 0.0f &&
        Condition > 0.0f && CurrentFuelLiters > KINDA_SMALL_NUMBER && EngineTemperatureC < CriticalEngineTemperatureC - 5.0f)
    {
        ActiveFaultStatus.Empty();
        SetEngineRunning(true);
    }

    const float ConditionPower = FMath::Lerp(0.35f, 1.0f, GetConditionPercent());
    const float TemperaturePower = EngineTemperatureC >= OverheatStartTemperatureC ? 0.72f : 1.0f;
    const float EngineTunePower = 1.0f + EngineUpgradeLevel * 0.12f;
    const float TireGrip = FMath::Lerp(0.38f, 1.0f, TireIntegrity) * (1.0f + TireUpgradeLevel * 0.04f);
    const float EffectiveValue = bEngineRunning && Condition > 0.0f && CurrentFuelLiters > 0.0f
        ? Value * ConditionPower * TemperaturePower * EngineTunePower * TireGrip
        : 0.0f;

    if (!FMath::IsNearlyZero(EffectiveValue) && VehicleMesh && VehicleMesh->IsSimulatingPhysics())
    {
        VehicleMesh->AddForce(GetActorForwardVector() * EffectiveValue * DriveAcceleration, NAME_None, true);
    }
    OnThrottleInput(EffectiveValue);
}

void AGTTVehicleBase::HandleSteering(float Value)
{
    if (bEngineRunning && Condition > 0.0f && CurrentFuelLiters > 0.0f && VehicleMesh && VehicleMesh->IsSimulatingPhysics())
    {
        const float SpeedFactor = FMath::Clamp(GetVelocity().Size2D() / 500.0f, 0.18f, 1.0f);
        const float TireGrip = FMath::Lerp(0.28f, 1.0f, TireIntegrity) * (1.0f + TireUpgradeLevel * 0.08f);
        VehicleMesh->AddTorqueInRadians(FVector::UpVector * Value * SteeringAcceleration * SpeedFactor * TireGrip, NAME_None, true);
    }
    OnSteeringInput(Value);
}

void AGTTVehicleBase::CycleRadio()
{
    if (APawn* Driver = PreviousPawn.Get())
    {
        if (UGTTRadioComponent* Radio = Driver->FindComponentByClass<UGTTRadioComponent>())
        {
            Radio->CycleStation();
        }
    }
}

void AGTTVehicleBase::HandleVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    const float ExcessImpulse = NormalImpulse.Size() - MinDamagingImpulse;
    if (ExcessImpulse <= 0.0f) return;

    const float Damage = FMath::Clamp(ExcessImpulse / ImpulsePerDamagePoint, 0.0f, 35.0f);
    ApplyVehicleDamage(Damage);

    if (Damage >= 5.0f)
    {
        const float TireLoss = FMath::Clamp(Damage / 100.0f, 0.015f, 0.18f);
        ApplyTireDamage(TireLoss);
    }
}

void AGTTVehicleBase::RegisterBreakablePart(UStaticMeshComponent* Part, float DetachAtConditionPercent, FName PartName)
{
    if (!Part) return;
    FBreakablePartRuntime Runtime;
    Runtime.Component = Part;
    Runtime.OriginalRelativeTransform = Part->GetRelativeTransform();
    Runtime.DetachThreshold = FMath::Clamp(DetachAtConditionPercent, 0.02f, 0.98f);
    Runtime.PartName = PartName;
    BreakableParts.Add(Runtime);
}

void AGTTVehicleBase::UpdateBreakableParts()
{
    const float ConditionPercent = GetConditionPercent();
    for (FBreakablePartRuntime& Runtime : BreakableParts)
    {
        UStaticMeshComponent* Part = Runtime.Component.Get();
        if (!Part || Runtime.bDetached || ConditionPercent > Runtime.DetachThreshold) continue;

        Part->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Part->SetSimulatePhysics(true);
        Part->SetLinearDamping(0.45f);
        Part->SetAngularDamping(0.35f);
        const FVector SideKick = GetActorRightVector() * FMath::FRandRange(-210.0f, 210.0f);
        Part->AddImpulse((GetActorUpVector() * FMath::FRandRange(120.0f, 260.0f)) + SideKick, NAME_None, true);
        Runtime.bDetached = true;
        ++DetachedPartCount;
        UE_LOG(LogGTT, Warning, TEXT("%s lost body part: %s"), *GetName(), *Runtime.PartName.ToString());
    }
}

void AGTTVehicleBase::RestoreBreakableParts()
{
    DetachedPartCount = 0;
    for (FBreakablePartRuntime& Runtime : BreakableParts)
    {
        UStaticMeshComponent* Part = Runtime.Component.Get();
        if (!Part) continue;
        Part->SetSimulatePhysics(false);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->AttachToComponent(VehicleMesh, FAttachmentTransformRules::KeepRelativeTransform);
        Part->SetRelativeTransform(Runtime.OriginalRelativeTransform);
        Runtime.bDetached = false;
    }
}

void AGTTVehicleBase::UpdateDamageSmoke(float DeltaSeconds)
{
    DamageFxClock += DeltaSeconds;
    const float ConditionPercent = GetConditionPercent();
    const bool bShowSmoke = ConditionPercent < 0.62f;
    const float Severity = FMath::Clamp((0.62f - ConditionPercent) / 0.62f, 0.0f, 1.0f);
    UStaticMeshComponent* Puffs[] = {DamageSmokePuffA.Get(), DamageSmokePuffB.Get(), DamageSmokePuffC.Get()};

    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMeshComponent* Puff = Puffs[Index];
        if (!Puff) continue;
        Puff->SetVisibility(bShowSmoke, true);
        if (!bShowSmoke) continue;

        const float Phase = FMath::Fmod(DamageFxClock * (0.55f + Severity * 0.75f) + Index * 0.33f, 1.0f);
        const float Drift = FMath::Sin((DamageFxClock + Index) * 2.1f) * 18.0f;
        Puff->SetRelativeLocation(FVector(78.0f + Drift, (Index - 1) * 18.0f, 85.0f + Phase * 150.0f));
        const float Scale = 0.10f + Severity * 0.18f + Phase * 0.18f;
        Puff->SetRelativeScale3D(FVector(Scale));
    }
}

void AGTTVehicleBase::TriggerMechanicalStall(const TCHAR* Reason)
{
    if (!bEngineRunning) return;
    LastThrottleInput = 0.0f;
    ActiveFaultStatus = Reason;
    FaultRestartTimeRemaining = FaultRestartDelaySeconds;
    SetEngineRunning(false);
    UE_LOG(LogGTT, Warning, TEXT("%s mechanical fault: %s"), *GetName(), Reason);
}

void AGTTVehicleBase::SetEngineRunning(bool bNewRunning)
{
    const bool bCanRun = bNewRunning && Condition > 0.0f && CurrentFuelLiters > KINDA_SMALL_NUMBER && EngineTemperatureC < CriticalEngineTemperatureC;
    if (bEngineRunning == bCanRun) return;
    bEngineRunning = bCanRun;
    OnEngineStateChanged(bEngineRunning);
}
