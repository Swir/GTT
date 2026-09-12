#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "GTTUnifiedSaveSubsystem.generated.h"

UCLASS()
class GTT_API UGTTUnifiedSaveSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Save|Unified")
    void ConsolidateLegacySlots();

    UFUNCTION(BlueprintPure, Category="GTT|Save|Unified")
    bool HasUnifiedSnapshot() const;

private:
    void HydrateCompatibilityMirrors();

    FString PrimarySlotName = TEXT("GTT_Prototype_01");
    FString CombatSlotName = TEXT("GTT_Combat_01");
    FString StorySlotName = TEXT("GTT_MainStory_01");
    FString Arc3SlotName = TEXT("GTT_MainStory_Arc3_01");
    FString Arc4SlotName = TEXT("GTT_MainStory_Arc4_01");
    FString FactionSlotName = TEXT("GTT_Factions_01");
    FString RuralEconomySlotName = TEXT("GTT_RuralEconomy_01");
    FTimerHandle ConsolidationTimer;
};
