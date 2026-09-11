#include "Vehicles/GTTVehicleBase.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GTT.h"

AGTTVehicleBase::AGTTVehicleBase()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(SceneRoot);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(SceneRoot);
    CameraBoom->TargetArmLength = 550.0f;
    CameraBoom->bUsePawnControlRotation = true;

    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    VehicleCamera->bUsePawnControlRotation = false;
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
    if (bOccupied || !IsValid(Interactor))
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
    InteractingPawn->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
    InteractingPawn->SetActorHiddenInGame(true);
    InteractingPawn->SetActorEnableCollision(false);

    InteractingController->Possess(this);
    bOccupied = true;
}

FText AGTTVehicleBase::GetInteractionText_Implementation() const
{
    return bOccupied
        ? NSLOCTEXT("GTT", "VehicleOccupied", "Occupied")
        : NSLOCTEXT("GTT", "EnterVehicle", "Enter vehicle");
}

void AGTTVehicleBase::ExitVehicle()
{
    AController* VehicleController = GetController();
    APawn* PawnToRestore = PreviousPawn.Get();

    if (!VehicleController || !PawnToRestore)
    {
        return;
    }

    PawnToRestore->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    PawnToRestore->SetActorLocation(GetActorTransform().TransformPosition(ExitOffset));
    PawnToRestore->SetActorRotation(FRotator(0.0f, GetActorRotation().Yaw, 0.0f));
    PawnToRestore->SetActorHiddenInGame(false);
    PawnToRestore->SetActorEnableCollision(true);

    VehicleController->Possess(PawnToRestore);
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

void AGTTVehicleBase::HandleThrottle(float Value)
{
    OnThrottleInput(Condition > 0.0f ? Value : 0.0f);
}

void AGTTVehicleBase::HandleSteering(float Value)
{
    OnSteeringInput(Value);
}
