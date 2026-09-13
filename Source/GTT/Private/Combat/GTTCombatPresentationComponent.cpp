#include "Combat/GTTCombatPresentationComponent.h"

#include "Combat/GTTCombatComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

UGTTCombatPresentationComponent::UGTTCombatPresentationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGTTCombatPresentationComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsureVisualParts();
    RefreshEquippedWeapon();
}

void UGTTCombatPresentationComponent::EnsureVisualParts()
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->GetRootComponent() || PrimaryPart) return;

    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    auto MakePart = [Owner](const TCHAR* Name, UStaticMesh* Mesh)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner, Name);
        Part->SetStaticMesh(Mesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->RegisterComponent();
        Part->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        return Part;
    };

    PrimaryPart = MakePart(TEXT("CombatWeaponPrimary"), Cylinder);
    SecondaryPart = MakePart(TEXT("CombatWeaponSecondary"), Cube);
    DetailPart = MakePart(TEXT("CombatWeaponDetail"), Sphere);
    ResetPose();
}

void UGTTCombatPresentationComponent::RefreshEquippedWeapon()
{
    if (const AActor* Owner = GetOwner())
    {
        if (const UGTTCombatComponent* Combat = Owner->FindComponentByClass<UGTTCombatComponent>())
        {
            ConfigureWeapon(Combat->GetEquippedWeapon());
            return;
        }
    }
    ConfigureWeapon(EGTTWeaponType::BareHands);
}

void UGTTCombatPresentationComponent::ConfigureWeapon(EGTTWeaponType WeaponType)
{
    EnsureVisualParts();
    if (!PrimaryPart || !SecondaryPart || !DetailPart) return;
    if (VisualWeapon == WeaponType && PrimaryPart->IsVisible() == (WeaponType != EGTTWeaponType::BareHands)) return;

    VisualWeapon = WeaponType;
    const bool bVisible = WeaponType != EGTTWeaponType::BareHands;
    PrimaryPart->SetVisibility(bVisible);
    SecondaryPart->SetVisibility(bVisible);
    DetailPart->SetVisibility(bVisible);
    ResetPose();
    if (!bVisible) return;

    FVector PrimaryScale(.055f,.055f,.48f);
    FVector SecondaryScale(.08f,.08f,.18f);
    FVector DetailScale(.07f);
    FVector SecondaryOffset(0,0,42);
    FVector DetailOffset(0,0,-42);

    switch (WeaponType)
    {
        case EGTTWeaponType::Pitchfork:
            PrimaryScale=FVector(.045f,.045f,.62f); SecondaryScale=FVector(.30f,.045f,.035f); DetailScale=FVector(.035f,.12f,.035f); SecondaryOffset=FVector(0,0,57); DetailOffset=FVector(0,9,61); break;
        case EGTTWeaponType::Axe:
            PrimaryScale=FVector(.055f,.055f,.52f); SecondaryScale=FVector(.10f,.28f,.16f); DetailScale=FVector(.05f); SecondaryOffset=FVector(0,0,46); DetailOffset=FVector(0,0,-47); break;
        case EGTTWeaponType::Branch:
            PrimaryScale=FVector(.07f,.06f,.56f); SecondaryScale=FVector(.05f,.10f,.22f); DetailScale=FVector(.06f); SecondaryOffset=FVector(3,5,29); DetailOffset=FVector(-2,-4,49); break;
        case EGTTWeaponType::Rake:
            PrimaryScale=FVector(.045f,.045f,.64f); SecondaryScale=FVector(.07f,.34f,.045f); DetailScale=FVector(.03f,.18f,.03f); SecondaryOffset=FVector(0,0,58); DetailOffset=FVector(0,0,63); break;
        case EGTTWeaponType::CowChain:
            PrimaryScale=FVector(.035f,.035f,.44f); SecondaryScale=FVector(.04f,.04f,.26f); DetailScale=FVector(.08f); SecondaryOffset=FVector(0,4,-38); DetailOffset=FVector(0,7,-62); break;
        case EGTTWeaponType::Shovel:
            PrimaryScale=FVector(.05f,.05f,.60f); SecondaryScale=FVector(.16f,.24f,.05f); DetailScale=FVector(.055f); SecondaryOffset=FVector(0,0,54); DetailOffset=FVector(0,0,-54); break;
        case EGTTWeaponType::WorkshopWrench:
            PrimaryScale=FVector(.06f,.06f,.34f); SecondaryScale=FVector(.12f,.22f,.08f); DetailScale=FVector(.09f,.04f,.11f); SecondaryOffset=FVector(0,0,29); DetailOffset=FVector(0,0,36); break;
        case EGTTWeaponType::FarmShotgun:
            PrimaryScale=FVector(.075f,.075f,.68f); SecondaryScale=FVector(.12f,.055f,.34f); DetailScale=FVector(.09f,.11f,.18f); SecondaryOffset=FVector(0,5,-36); DetailOffset=FVector(0,7,-53); break;
        default: break;
    }

    PrimaryPart->SetRelativeScale3D(PrimaryScale);
    SecondaryPart->SetRelativeScale3D(SecondaryScale);
    SecondaryPart->SetRelativeLocation(PrimaryBaseLocation + SecondaryOffset);
    DetailPart->SetRelativeScale3D(DetailScale);
    DetailPart->SetRelativeLocation(PrimaryBaseLocation + DetailOffset);
}

void UGTTCombatPresentationComponent::PlayAttack(EGTTWeaponType WeaponType)
{
    ConfigureWeapon(WeaponType);
    AttackDuration = WeaponType == EGTTWeaponType::FarmShotgun ? 0.18f : 0.30f;
    AttackTimeRemaining = AttackDuration;
}

void UGTTCombatPresentationComponent::ResetPose()
{
    if (!PrimaryPart || !SecondaryPart || !DetailPart) return;
    PrimaryPart->SetRelativeLocation(PrimaryBaseLocation);
    PrimaryPart->SetRelativeRotation(PrimaryBaseRotation);
    SecondaryPart->SetRelativeRotation(PrimaryBaseRotation);
    DetailPart->SetRelativeRotation(PrimaryBaseRotation);
}

void UGTTCombatPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshEquippedWeapon();
    if (AttackTimeRemaining <= 0.0f || !PrimaryPart) return;

    AttackTimeRemaining = FMath::Max(0.0f, AttackTimeRemaining - DeltaTime);
    const float Alpha = 1.0f - (AttackTimeRemaining / FMath::Max(AttackDuration, KINDA_SMALL_NUMBER));
    const float Pulse = FMath::Sin(Alpha * PI);
    const bool bShotgun = VisualWeapon == EGTTWeaponType::FarmShotgun;
    const float Pitch = bShotgun ? -16.0f * Pulse : 78.0f * Pulse;
    const float Yaw = bShotgun ? 0.0f : 22.0f * Pulse;
    const FVector Recoil = bShotgun ? FVector(-20.0f * Pulse,0,5.0f*Pulse) : FVector(12.0f*Pulse,0,7.0f*Pulse);
    const FRotator AnimatedRotation = PrimaryBaseRotation + FRotator(Pitch,Yaw,0);
    const FVector AnimatedLocation = PrimaryBaseLocation + Recoil;
    PrimaryPart->SetRelativeRotation(AnimatedRotation);
    PrimaryPart->SetRelativeLocation(AnimatedLocation);
    SecondaryPart->SetRelativeRotation(AnimatedRotation);
    DetailPart->SetRelativeRotation(AnimatedRotation);
    if (AttackTimeRemaining <= 0.0f) ResetPose();
}
