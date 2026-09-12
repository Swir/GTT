#include "Combat/GTTCombatComponent.h"

#include "Characters/GTTCharacter.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTCitizenPawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

UGTTCombatComponent::UGTTCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    Inventory.Add(EGTTWeaponType::BareHands);
}

void UGTTCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
}

void UGTTCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AttackCooldownRemaining = FMath::Max(0.0f, AttackCooldownRemaining - DeltaTime);
}

bool UGTTCombatComponent::AddWeapon(EGTTWeaponType Type, int32 Ammo, bool bAutoEquip)
{
    if (Type != EGTTWeaponType::BareHands && !Inventory.Contains(Type)) Inventory.Add(Type);
    if (Type == EGTTWeaponType::FarmShotgun) ShotgunAmmo = FMath::Clamp(ShotgunAmmo + FMath::Max(0, Ammo), 0, 24);
    if (bAutoEquip) EquippedWeapon = Type;
    return true;
}

void UGTTCombatComponent::Attack()
{
    if (AttackCooldownRemaining > 0.0f || !GetOwner()) return;
    const FGTTWeaponProfile Profile = FGTTWeaponProfile::Make(EquippedWeapon);
    if (Profile.bRanged)
    {
        if (ShotgunAmmo <= 0)
        {
            if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Cast<APawn>(GetOwner()))) Economy->PushMessage(TEXT("OLD FARM SHOTGUN: no shells."), 2.5f);
            AttackCooldownRemaining = 0.35f;
            return;
        }
        PerformShotgunAttack(Profile);
    }
    else PerformMeleeAttack(Profile);
    AttackCooldownRemaining = Profile.Cooldown;
}

void UGTTCombatComponent::PerformMeleeAttack(const FGTTWeaponProfile& Profile)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;
    const FVector Forward = Owner->GetActorForwardVector();
    const FVector Start = Owner->GetActorLocation() + FVector(0, 0, 55) + Forward * 45.0f;
    const FVector End = Start + Forward * Profile.Range;
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTMelee), false, Owner);
    const FCollisionShape Shape = FCollisionShape::MakeSphere(52.0f);
    if (!World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Shape, Params)) return;
    TSet<TObjectPtr<AActor>> Damaged;
    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!Target || Damaged.Contains(Target)) continue;
        Damaged.Add(Target);
        ApplyHit(Target, Forward, Profile);
    }
}

void UGTTCombatComponent::PerformShotgunAttack(const FGTTWeaponProfile& Profile)
{
    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner) return;
    --ShotgunAmmo;
    AddCrimeHeat(Profile.PoliceHeat, TEXT("Gunshot reported in the countryside."));

    FVector ViewLocation = Owner->GetActorLocation() + FVector(0,0,65);
    FRotator ViewRotation = Owner->GetActorRotation();
    if (APawn* Pawn = Cast<APawn>(Owner)) if (AController* Controller = Pawn->GetController()) ViewRotation = Controller->GetControlRotation();

    TSet<TObjectPtr<AActor>> HitActors;
    for (int32 Ray=0; Ray<5; ++Ray)
    {
        const FVector Direction = FMath::VRandCone(ViewRotation.Vector(), FMath::DegreesToRadians(5.5f));
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTShotgun), false, Owner);
        if (World->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + Direction * Profile.Range, ECC_Visibility, Params))
        {
            if (AActor* Target = Hit.GetActor()) if (!HitActors.Contains(Target)) { HitActors.Add(Target); ApplyHit(Target, Direction, Profile); }
        }
    }
}

void UGTTCombatComponent::ApplyHit(AActor* Target, const FVector& HitDirection, const FGTTWeaponProfile& Profile)
{
    if (!Target) return;
    if (AGTTCitizenPawn* Citizen = Cast<AGTTCitizenPawn>(Target))
    {
        Citizen->ApplyCombatHit(Profile.Damage, HitDirection, Profile.Knockback, Cast<APawn>(GetOwner()));
        if (!Profile.bRanged) AddCrimeHeat(Profile.PoliceHeat, TEXT("Assault reported by villagers."));
        return;
    }
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Target))
    {
        Vehicle->ApplyVehicleDamage(Profile.bRanged ? 9.0f : FMath::Clamp(Profile.Damage * 0.12f, 1.0f, 5.0f));
    }
}

void UGTTCombatComponent::ApplyIncomingDamage(float Damage, const FString& SourceLabel)
{
    if (Damage <= 0.0f || Health <= 0.0f) return;
    Health = FMath::Max(0.0f, Health - Damage);
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Cast<APawn>(GetOwner())))
        Economy->PushMessage(FString::Printf(TEXT("%s | HEALTH %.0f%%"), *SourceLabel, GetHealthPercent()*100.0f), 2.0f);
    if (Health <= 0.0f) HandleDefeat();
}

void UGTTCombatComponent::HandleDefeat()
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn) return;
    Health = MaxHealth;
    EquippedWeapon = EGTTWeaponType::BareHands;
    ShotgunAmmo = FMath::Max(0, ShotgunAmmo - 2);
    Pawn->SetActorLocation(FVector(-2550,-1250,120), false, nullptr, ETeleportType::TeleportPhysics);
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Pawn)) Wanted->ClearWanted();
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn))
    {
        Economy->ChargeFine(85, TEXT("KNOCKED OUT - clinic and cleanup: $85."));
        Economy->PushMessage(TEXT("You wake up back at Player Farm. Wanted cleared, weapon holstered."), 5.0f);
    }
}

void UGTTCombatComponent::AddCrimeHeat(float Amount, const FString& Message)
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || Amount <= 0.0f) return;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(OwnerPawn)) Wanted->AddHeat(Amount);
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(OwnerPawn)) Economy->PushMessage(Message, 3.5f);
}

void UGTTCombatComponent::CycleWeapon()
{
    if (Inventory.Num() == 0) return;
    const int32 Current = Inventory.IndexOfByKey(EquippedWeapon);
    EquippedWeapon = Inventory[Current == INDEX_NONE ? 0 : (Current + 1) % Inventory.Num()];
}

void UGTTCombatComponent::DropCurrentWeapon()
{
    if (EquippedWeapon == EGTTWeaponType::BareHands) return;
    Inventory.Remove(EquippedWeapon);
    EquippedWeapon = EGTTWeaponType::BareHands;
}

FString UGTTCombatComponent::GetCombatStatusText() const
{
    const FGTTWeaponProfile Profile = FGTTWeaponProfile::Make(EquippedWeapon);
    const FString WeaponText = EquippedWeapon == EGTTWeaponType::FarmShotgun
        ? FString::Printf(TEXT("%s | SHELLS %d"), *Profile.DisplayName.ToString(), ShotgunAmmo)
        : FString::Printf(TEXT("%s | INVENTORY %d"), *Profile.DisplayName.ToString(), FMath::Max(0, Inventory.Num()-1));
    return FString::Printf(TEXT("RURAL ARSENAL | HP %.0f%% | %s"), GetHealthPercent()*100.0f, *WeaponText);
}
