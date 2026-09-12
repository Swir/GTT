#include "Characters/GTTCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/GTTInteractable.h"
#include "UObject/ConstructorHelpers.h"
#include "Wanted/GTTWantedComponent.h"

AGTTCharacter::AGTTCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(RootComponent);
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderBody->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
    PlaceholderBody->SetRelativeScale3D(FVector(0.35f, 0.35f, 1.45f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (BodyMeshFinder.Succeeded())
    {
        PlaceholderBody->SetStaticMesh(BodyMeshFinder.Object);
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 420.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    WantedComponent = CreateDefaultSubobject<UGTTWantedComponent>(TEXT("WantedComponent"));
}

void AGTTCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGTTCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGTTCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AGTTCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGTTCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AGTTCharacter::TryInteract);
}

void AGTTCharacter::MoveForward(float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AGTTCharacter::MoveRight(float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AGTTCharacter::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AGTTCharacter::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AGTTCharacter::TryInteract()
{
    if (!FollowCamera || !GetWorld())
    {
        return;
    }

    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * InteractionDistance;

    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GTTInteraction), false, this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
    {
        AActor* HitActor = Hit.GetActor();
        if (IsValid(HitActor) && HitActor->GetClass()->ImplementsInterface(UGTTInteractable::StaticClass()))
        {
            IGTTInteractable::Execute_Interact(HitActor, this);
        }
    }
}
