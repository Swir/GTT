#include "Combat/GTTCombatComponent.h"

#include "Combat/GTTWeaponPickup.h"
#include "Characters/GTTCharacter.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTCitizenPawn.h"
#include "Save/GTTCombatSave.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

// Persistent loadout contract: GTT_Combat_01 is declared on the component and used by all save/load mutations below.
UGTTCombatComponent::UGTTCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    Inventory.Add(EGTTWeaponType::BareHands);
}

void UGTTCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    LoadPersistentLoadout();
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
    SavePersistentLoadout();
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
    TSet<AActor*> Damaged;
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
    SavePersistentLoadout();
    AddCrimeHeat(Profile.PoliceHeat, TEXT("Gunshot reported in the countryside."));

    FVector ViewLocation = Owner->GetActorLocation() + FVector(0,0,65);
    FRotator ViewRotation = Owner->GetActorRotation();
    if (APawn* Pawn = Cast<APawn>(Owner)) if (AController* Controller = Pawn->GetController()) ViewRotation = Controller->GetControlRotation();

    TSet<AActor*> HitActors;
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
        if (!Profile.bRanged && !Citizen->IsBrawlParticipant()) AddCrimeHeat(Profile.PoliceHeat, TEXT("Assault reported by villagers."));
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
    SavePersistentLoadout();
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
    SavePersistentLoadout();
}

void UGTTCombatComponent::DropCurrentWeapon()
{
    if (EquippedWeapon == EGTTWeaponType::BareHands || !GetWorld() || !GetOwner()) return;
    const EGTTWeaponType DroppedType = EquippedWeapon;
    const int32 DroppedAmmo = DroppedType == EGTTWeaponType::FarmShotgun ? ShotgunAmmo : 0;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    const FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector()*90.0f + FVector(0,0,35);
    if (AGTTWeaponPickup* Pickup = GetWorld()->SpawnActor<AGTTWeaponPickup>(SpawnLocation, GetOwner()->GetActorRotation(), Params))
    {
        Pickup->Configure(DroppedType, DroppedAmmo);
        Inventory.Remove(DroppedType);
        if (DroppedType == EGTTWeaponType::FarmShotgun) ShotgunAmmo = 0;
        EquippedWeapon = EGTTWeaponType::BareHands;
        SavePersistentLoadout();
    }
}

void UGTTCombatComponent::SavePersistentLoadout()
{
    UGTTCombatSave* Save = Cast<UGTTCombatSave>(UGameplayStatics::CreateSaveGameObject(UGTTCombatSave::StaticClass()));
    if (!Save) return;

    Save->CombatSaveVersion = 1;
    Save->WeaponTypes.Reset();
    for (const EGTTWeaponType Weapon : Inventory)
    {
        if (Weapon != EGTTWeaponType::BareHands) Save->WeaponTypes.Add(static_cast<uint8>(Weapon));
    }
    Save->EquippedWeaponType = static_cast<uint8>(EquippedWeapon);
    Save->ShotgunAmmo = FMath::Clamp(ShotgunAmmo, 0, 24);
    UGameplayStatics::SaveGameToSlot(Save, CombatSaveSlotName, 0);
}

void UGTTCombatComponent::LoadPersistentLoadout()
{
    Inventory.Reset();
    Inventory.Add(EGTTWeaponType::BareHands);
    EquippedWeapon = EGTTWeaponType::BareHands;
    ShotgunAmmo = 0;

    if (!UGameplayStatics::DoesSaveGameExist(CombatSaveSlotName, 0)) return;
    UGTTCombatSave* Save = Cast<UGTTCombatSave>(UGameplayStatics::LoadGameFromSlot(CombatSaveSlotName, 0));
    if (!Save) return;

    const uint8 MaxWeaponValue = static_cast<uint8>(EGTTWeaponType::FarmShotgun);
    for (const uint8 RawType : Save->WeaponTypes)
    {
        if (RawType == 0 || RawType > MaxWeaponValue) continue;
        const EGTTWeaponType Weapon = static_cast<EGTTWeaponType>(RawType);
        if (!Inventory.Contains(Weapon)) Inventory.Add(Weapon);
    }

    ShotgunAmmo = FMath::Clamp(Save->ShotgunAmmo, 0, 24);
    if (Save->EquippedWeaponType <= MaxWeaponValue)
    {
        const EGTTWeaponType SavedEquipped = static_cast<EGTTWeaponType>(Save->EquippedWeaponType);
        if (Inventory.Contains(SavedEquipped)) EquippedWeapon = SavedEquipped;
    }
}

FString UGTTCombatComponent::GetCombatStatusText() const
{
    const FGTTWeaponProfile Profile = FGTTWeaponProfile::Make(EquippedWeapon);
    const FString WeaponText = EquippedWeapon == EGTTWeaponType::FarmShotgun
        ? FString::Printf(TEXT("%s | SHELLS %d"), *Profile.DisplayName.ToString(), ShotgunAmmo)
        : FString::Printf(TEXT("%s | INVENTORY %d"), *Profile.DisplayName.ToString(), FMath::Max(0, Inventory.Num()-1));
    return FString::Printf(TEXT("RURAL ARSENAL | HP %.0f%% | %s | LOADOUT SAVED"), GetHealthPercent()*100.0f, *WeaponText);
}
