#include "Vehicles/GTTVehicleBase.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

AGTTVehicleBase::AGTTVehicleBase()
{
    PrimaryActorTick.bCanEverTick = false;

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

    VehicleDisplayName = NSLOCTEXT("GTT", "DefaultVehicleName", "Old vehicle");
}

void AGTTVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("VehicleThrottle"), this, &AGTTVehicleBase::HandleThrottle);
    PlayerInputComponent->BindAxis(TEXT("VehicleSteer"), this, &AGTTVehicleBase::HandleSteering);
    PlayerInputComponent->BindAction(TEXT("ExitVehicle"), IE_Pressed, this, &AGTTVehicleBase::ExitVehicle);
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

    if (bIllegalToTake && !bTheftReported)
    {
        if (UGTTWantedComponent* Wanted = InteractingPawn->FindComponentByClass<UGTTWantedComponent>())
        {
            Wanted->AddHeat(TheftHeat);
        }

        bTheftReported = true;
        OnVehicleStolen.Broadcast();

        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->NotifyVehicleStolen(this);
        }
    }

    PreviousPawn = InteractingPawn;
    InteractingPawn->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    InteractingPawn->SetActorHiddenInGame(true);
    InteractingPawn->SetActorEnableCollision(false);

    InteractingController->Possess(this);
    bOccupied = true;
    SetEngineRunning(true);
    OnDriverEntered.Broadcast(InteractingPawn);
}

FText AGTTVehicleBase::GetInteractionText_Implementation() const
{
    if (bOccupied)
    {
        return NSLOCTEXT("GTT", "VehicleOccupied", "Occupied");
    }

    if (Condition <= 0.0f)
    {
        return NSLOCTEXT("GTT", "VehicleBroken", "Broken down");
    }

    return FText::Format(
        NSLOCTEXT("GTT", "EnterNamedVehicle", "Enter {0}"),
        VehicleDisplayName);
}

void AGTTVehicleBase::ExitVehicle()
{
    AController* VehicleController = GetController();
    APawn* PawnToRestore = PreviousPawn.Get();

    if (!VehicleController || !PawnToRestore)
    {
        return;
    }

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
    if (DamageAmount <= 0.0f || Condition <= 0.0f)
    {
        return;
    }

    const float OldCondition = Condition;
    Condition = FMath::Clamp(Condition - DamageAmount, 0.0f, MaxCondition);

    if (OldCondition > 0.0f && Condition <= 0.0f)
    {
        SetEngineRunning(false);
        UE_LOG(LogGTT, Warning, TEXT("Vehicle %s broke down."), *GetName());
        OnVehicleBrokenDown();
    }
}

void AGTTVehicleBase::RepairVehicle(float RepairAmount)
{
    if (RepairAmount > 0.0f)
    {
        Condition = FMath::Clamp(Condition + RepairAmount, 0.0f, MaxCondition);
    }
}

float AGTTVehicleBase::GetConditionPercent() const
{
    return MaxCondition > 0.0f ? Condition / MaxCondition : 0.0f;
}

float AGTTVehicleBase::GetSpeedKmh() const
{
    return GetVelocity().Size() * 0.036f;
}

void AGTTVehicleBase::HandleThrottle(float Value)
{
    const float EffectiveValue = bEngineRunning && Condition > 0.0f ? Value : 0.0f;

    if (!FMath::IsNearlyZero(EffectiveValue) && VehicleMesh && VehicleMesh->IsSimulatingPhysics())
    {
        VehicleMesh->AddForce(GetActorForwardVector() * EffectiveValue * DriveAcceleration, NAME_None, true);
    }

    OnThrottleInput(EffectiveValue);
}

void AGTTVehicleBase::HandleSteering(float Value)
{
    if (bEngineRunning && Condition > 0.0f && VehicleMesh && VehicleMesh->IsSimulatingPhysics())
    {
        const float SpeedFactor = FMath::Clamp(GetVelocity().Size2D() / 500.0f, 0.18f, 1.0f);
        VehicleMesh->AddTorqueInRadians(
            FVector::UpVector * Value * SteeringAcceleration * SpeedFactor,
            NAME_None,
            true);
    }

    OnSteeringInput(Value);
}

void AGTTVehicleBase::HandleVehicleHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    const float ExcessImpulse = NormalImpulse.Size() - MinDamagingImpulse;
    if (ExcessImpulse <= 0.0f)
    {
        return;
    }

    const float Damage = FMath::Clamp(ExcessImpulse / ImpulsePerDamagePoint, 0.0f, 35.0f);
    ApplyVehicleDamage(Damage);
}

void AGTTVehicleBase::SetEngineRunning(bool bNewRunning)
{
    const bool bCanRun = bNewRunning && Condition > 0.0f;
    if (bEngineRunning == bCanRun)
    {
        return;
    }

    bEngineRunning = bCanRun;
    OnEngineStateChanged(bEngineRunning);
}
