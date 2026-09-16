#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FGTTStoredVehicleData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") FName VehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") FTransform Transform;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") float ConditionPercent = 1.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") float FuelLiters = 0.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") int32 EngineUpgradeLevel = 0;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") int32 TireUpgradeLevel = 0;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle") float TireIntegrity = 1.0f;
};

USTRUCT(BlueprintType)
struct FGTTStoredRoadStructuralDamageData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") FName VehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") float FrontHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") float RearHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") float LeftHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") float RightHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") float CoolingStress = 0.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Vehicle|Structural") int32 DetachedPanelMask = 0;
};

UCLASS()
class GTT_API UGTTSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") int32 SaveVersion = 7;
    // Migration compatibility: legacy vehicle-tuning snapshots used SaveVersion = 3;
    // v4 introduced unified world state, v5 added persistent Native structural damage,
    // v6 persists the active garage dispatch vehicle, and v7 adds per-role mission loadouts.
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") int32 Cash = 120;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") int32 FishCount = 0;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") float FishWeightKg = 0.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") FTransform PlayerTransform;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") bool bBorrowedTractorCompleted = false;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") int32 DayNumber = 1;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save") float TimeOfDayHours = 8.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Garage") TArray<FGTTStoredVehicleData> OwnedVehicles;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Garage") TArray<FGTTStoredRoadStructuralDamageData> RoadStructuralDamage;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Garage") FName PreferredGarageVehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Garage|Loadout") FName PreferredTractorVehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Garage|Loadout") FName PreferredRoadVehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Garage|Loadout") FName PreferredCargoVehicleId = NAME_None;

    // Save v4+: primary sandbox snapshot. Dedicated pre-v4 slots remain compatibility mirrors.
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified") bool bUnifiedWorldStateInitialized = false;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified") int32 UnifiedWorldStateRevision = 0;

    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Combat") TArray<uint8> CombatWeaponTypes;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Combat") uint8 CombatEquippedWeaponType = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Combat") int32 CombatShotgunAmmo = 0;

    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Story") int32 MainStoryStage = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Story") int32 Arc3Stage = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Story") int32 Arc4Stage = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Story") int32 Arc4StartingFactionVictories = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Story") bool bArc4ContrabandPrepared = false;

    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Factions") int32 FactionVictories = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Factions") int32 RustDogsDefeated = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Factions") int32 StoneCrowsDefeated = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|Factions") int32 MudJackalsDefeated = 0;

    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") int32 ContrabandUnits = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") int32 ContrabandValue = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") bool bInsuranceActive = false;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") FName ImpoundedVehicleId = NAME_None;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") int32 PendingImpoundFee = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") int32 LifetimeFenceRevenue = 0;
    UPROPERTY(VisibleAnywhere, SaveGame, Category="GTT|Save|Unified|RuralEconomy") int32 SpeedingCitations = 0;

    // Version 1 migration fields. Kept so existing prototype saves still load.
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy") bool bTractorOwned = false;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy") FTransform TractorTransform;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy") float TractorConditionPercent = 1.0f;
    UPROPERTY(VisibleAnywhere, Category="GTT|Save|Legacy") float TractorFuelLiters = 18.0f;
};
