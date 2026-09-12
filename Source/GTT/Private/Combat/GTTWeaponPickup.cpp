#include "Combat/GTTWeaponPickup.h"

#include "Combat/GTTCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGTTWeaponPickup::AGTTWeaponPickup()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.65f,0.10f,0.10f));

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0,0,120));
    Label->SetRelativeRotation(FRotator(0,180,0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(34.0f);
    Label->SetTextRenderColor(FColor(255,210,80));
    RefreshVisuals();
}

void AGTTWeaponPickup::Configure(EGTTWeaponType InType, int32 InAmmo)
{
    WeaponType = InType;
    Ammo = FMath::Max(0, InAmmo);
    RefreshVisuals();
}

void AGTTWeaponPickup::RefreshVisuals()
{
    if (!Label) return;
    const FGTTWeaponProfile Profile = FGTTWeaponProfile::Make(WeaponType);
    Label->SetText(FText::FromString(WeaponType == EGTTWeaponType::FarmShotgun
        ? FString::Printf(TEXT("%s + %d SHELLS"), *Profile.DisplayName.ToString(), Ammo)
        : Profile.DisplayName.ToString()));
}

void AGTTWeaponPickup::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn) return;
    if (UGTTCombatComponent* Combat = Pawn->FindComponentByClass<UGTTCombatComponent>())
    {
        if (Combat->AddWeapon(WeaponType, Ammo, true)) Destroy();
    }
}

FText AGTTWeaponPickup::GetInteractionText_Implementation() const
{
    const FGTTWeaponProfile Profile = FGTTWeaponProfile::Make(WeaponType);
    return FText::Format(NSLOCTEXT("GTT","PickupRuralWeapon","Pick up {0}"), Profile.DisplayName);
}
