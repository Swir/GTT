#include "Save/GTTUnifiedSaveSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Save/GTTArc3Save.h"
#include "Save/GTTArc4Save.h"
#include "Save/GTTCombatSave.h"
#include "Save/GTTFactionSaveGame.h"
#include "Save/GTTMainStorySave.h"
#include "Save/GTTRuralEconomySave.h"
#include "Save/GTTSaveGame.h"

namespace
{
template <typename T>
T* LoadSlot(const UObject* Context, const FString& Slot)
{
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return nullptr;
    return Cast<T>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
}
}

void UGTTUnifiedSaveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    ConsolidateLegacySlots();
    HydrateCompatibilityMirrors();
    InWorld.GetTimerManager().SetTimer(ConsolidationTimer, this, &UGTTUnifiedSaveSubsystem::ConsolidateLegacySlots, 5.0f, true, 5.0f);
}

bool UGTTUnifiedSaveSubsystem::HasUnifiedSnapshot() const
{
    const UGTTSaveGame* Primary = LoadSlot<UGTTSaveGame>(this, PrimarySlotName);
    return Primary && Primary->SaveVersion >= 4 && Primary->bUnifiedWorldStateInitialized;
}

void UGTTUnifiedSaveSubsystem::ConsolidateLegacySlots()
{
    // Never manufacture a primary save on a brand-new profile: GameMode owns creation of the base world snapshot.
    UGTTSaveGame* Primary = LoadSlot<UGTTSaveGame>(this, PrimarySlotName);
    if (!Primary) return;

    bool bChanged = Primary->SaveVersion < 4 || !Primary->bUnifiedWorldStateInitialized;

    if (const UGTTCombatSave* Legacy = LoadSlot<UGTTCombatSave>(this, CombatSlotName))
    {
        bChanged |= Primary->CombatWeaponTypes != Legacy->WeaponTypes;
        bChanged |= Primary->CombatEquippedWeaponType != Legacy->EquippedWeaponType;
        bChanged |= Primary->CombatShotgunAmmo != Legacy->ShotgunAmmo;
        Primary->CombatWeaponTypes = Legacy->WeaponTypes;
        Primary->CombatEquippedWeaponType = Legacy->EquippedWeaponType;
        Primary->CombatShotgunAmmo = Legacy->ShotgunAmmo;
    }

    if (const UGTTMainStorySave* Legacy = LoadSlot<UGTTMainStorySave>(this, StorySlotName))
    {
        bChanged |= Primary->MainStoryStage != Legacy->StoryStage;
        Primary->MainStoryStage = Legacy->StoryStage;
    }

    if (const UGTTArc3Save* Legacy = LoadSlot<UGTTArc3Save>(this, Arc3SlotName))
    {
        bChanged |= Primary->Arc3Stage != Legacy->Arc3Stage;
        Primary->Arc3Stage = Legacy->Arc3Stage;
    }

    if (const UGTTArc4Save* Legacy = LoadSlot<UGTTArc4Save>(this, Arc4SlotName))
    {
        bChanged |= Primary->Arc4Stage != Legacy->Arc4Stage;
        bChanged |= Primary->Arc4StartingFactionVictories != Legacy->StartingFactionVictories;
        bChanged |= Primary->bArc4ContrabandPrepared != Legacy->bContrabandPrepared;
        Primary->Arc4Stage = Legacy->Arc4Stage;
        Primary->Arc4StartingFactionVictories = Legacy->StartingFactionVictories;
        Primary->bArc4ContrabandPrepared = Legacy->bContrabandPrepared;
    }

    if (const UGTTFactionSaveGame* Legacy = LoadSlot<UGTTFactionSaveGame>(this, FactionSlotName))
    {
        bChanged |= Primary->FactionVictories != Legacy->FactionVictories;
        bChanged |= Primary->RustDogsDefeated != Legacy->RustDogsDefeated;
        bChanged |= Primary->StoneCrowsDefeated != Legacy->StoneCrowsDefeated;
        bChanged |= Primary->MudJackalsDefeated != Legacy->MudJackalsDefeated;
        Primary->FactionVictories = Legacy->FactionVictories;
        Primary->RustDogsDefeated = Legacy->RustDogsDefeated;
        Primary->StoneCrowsDefeated = Legacy->StoneCrowsDefeated;
        Primary->MudJackalsDefeated = Legacy->MudJackalsDefeated;
    }

    if (const UGTTRuralEconomySave* Legacy = LoadSlot<UGTTRuralEconomySave>(this, RuralEconomySlotName))
    {
        bChanged |= Primary->ContrabandUnits != Legacy->ContrabandUnits;
        bChanged |= Primary->ContrabandValue != Legacy->ContrabandValue;
        bChanged |= Primary->bInsuranceActive != Legacy->bInsuranceActive;
        bChanged |= Primary->ImpoundedVehicleId != Legacy->ImpoundedVehicleId;
        bChanged |= Primary->PendingImpoundFee != Legacy->PendingImpoundFee;
        bChanged |= Primary->LifetimeFenceRevenue != Legacy->LifetimeFenceRevenue;
        bChanged |= Primary->SpeedingCitations != Legacy->SpeedingCitations;
        Primary->ContrabandUnits = Legacy->ContrabandUnits;
        Primary->ContrabandValue = Legacy->ContrabandValue;
        Primary->bInsuranceActive = Legacy->bInsuranceActive;
        Primary->ImpoundedVehicleId = Legacy->ImpoundedVehicleId;
        Primary->PendingImpoundFee = Legacy->PendingImpoundFee;
        Primary->LifetimeFenceRevenue = Legacy->LifetimeFenceRevenue;
        Primary->SpeedingCitations = Legacy->SpeedingCitations;
    }

    if (!bChanged) return;
    // Unified consolidation must never downgrade newer schemas (v5+ adds Native structural damage).
    Primary->SaveVersion = FMath::Max(Primary->SaveVersion, 4);
    Primary->bUnifiedWorldStateInitialized = true;
    ++Primary->UnifiedWorldStateRevision;
    UGameplayStatics::SaveGameToSlot(Primary, PrimarySlotName, 0);
}

void UGTTUnifiedSaveSubsystem::HydrateCompatibilityMirrors()
{
    const UGTTSaveGame* Primary = LoadSlot<UGTTSaveGame>(this, PrimarySlotName);
    if (!Primary || Primary->SaveVersion < 4 || !Primary->bUnifiedWorldStateInitialized) return;

    UGTTCombatSave* Combat = Cast<UGTTCombatSave>(UGameplayStatics::CreateSaveGameObject(UGTTCombatSave::StaticClass()));
    if (Combat)
    {
        Combat->CombatSaveVersion = 1;
        Combat->WeaponTypes = Primary->CombatWeaponTypes;
        Combat->EquippedWeaponType = Primary->CombatEquippedWeaponType;
        Combat->ShotgunAmmo = Primary->CombatShotgunAmmo;
        UGameplayStatics::SaveGameToSlot(Combat, CombatSlotName, 0);
    }

    UGTTMainStorySave* Story = Cast<UGTTMainStorySave>(UGameplayStatics::CreateSaveGameObject(UGTTMainStorySave::StaticClass()));
    if (Story)
    {
        Story->StorySaveVersion = 2;
        Story->StoryStage = Primary->MainStoryStage;
        UGameplayStatics::SaveGameToSlot(Story, StorySlotName, 0);
    }

    UGTTArc3Save* Arc3 = Cast<UGTTArc3Save>(UGameplayStatics::CreateSaveGameObject(UGTTArc3Save::StaticClass()));
    if (Arc3)
    {
        Arc3->Arc3SaveVersion = 1;
        Arc3->Arc3Stage = Primary->Arc3Stage;
        UGameplayStatics::SaveGameToSlot(Arc3, Arc3SlotName, 0);
    }

    UGTTArc4Save* Arc4 = Cast<UGTTArc4Save>(UGameplayStatics::CreateSaveGameObject(UGTTArc4Save::StaticClass()));
    if (Arc4)
    {
        Arc4->Arc4SaveVersion = 1;
        Arc4->Arc4Stage = Primary->Arc4Stage;
        Arc4->StartingFactionVictories = Primary->Arc4StartingFactionVictories;
        Arc4->bContrabandPrepared = Primary->bArc4ContrabandPrepared;
        UGameplayStatics::SaveGameToSlot(Arc4, Arc4SlotName, 0);
    }

    UGTTFactionSaveGame* Factions = Cast<UGTTFactionSaveGame>(UGameplayStatics::CreateSaveGameObject(UGTTFactionSaveGame::StaticClass()));
    if (Factions)
    {
        Factions->FactionVictories = Primary->FactionVictories;
        Factions->RustDogsDefeated = Primary->RustDogsDefeated;
        Factions->StoneCrowsDefeated = Primary->StoneCrowsDefeated;
        Factions->MudJackalsDefeated = Primary->MudJackalsDefeated;
        UGameplayStatics::SaveGameToSlot(Factions, FactionSlotName, 0);
    }

    UGTTRuralEconomySave* Rural = Cast<UGTTRuralEconomySave>(UGameplayStatics::CreateSaveGameObject(UGTTRuralEconomySave::StaticClass()));
    if (Rural)
    {
        Rural->SaveVersion = 1;
        Rural->ContrabandUnits = Primary->ContrabandUnits;
        Rural->ContrabandValue = Primary->ContrabandValue;
        Rural->bInsuranceActive = Primary->bInsuranceActive;
        Rural->ImpoundedVehicleId = Primary->ImpoundedVehicleId;
        Rural->PendingImpoundFee = Primary->PendingImpoundFee;
        Rural->LifetimeFenceRevenue = Primary->LifetimeFenceRevenue;
        Rural->SpeedingCitations = Primary->SpeedingCitations;
        UGameplayStatics::SaveGameToSlot(Rural, RuralEconomySlotName, 0);
    }
}
