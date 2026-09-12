#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRuralFactionDirector.generated.h"

class AGTTCitizenPawn;

UENUM(BlueprintType)
enum class EGTTRuralFaction : uint8
{
    None,
    RustDogs,
    StoneCrows,
    MudJackals
};

UCLASS()
class GTT_API AGTTRuralFactionDirector : public AActor
{
    GENERATED_BODY()
public:
    AGTTRuralFactionDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|Faction") bool IsEncounterActive() const { return ActiveFaction != EGTTRuralFaction::None; }
    UFUNCTION(BlueprintPure, Category="GTT|Faction") int32 GetActiveHostileCount() const;
    UFUNCTION(BlueprintPure, Category="GTT|Faction") int32 GetFactionVictories() const { return FactionVictories; }
    UFUNCTION(BlueprintPure, Category="GTT|Faction") FString GetObjectiveText() const;
    UFUNCTION(BlueprintPure, Category="GTT|Faction") FString GetThreatText() const;

private:
    struct FFactionZone
    {
        EGTTRuralFaction Faction = EGTTRuralFaction::None;
        FString Label;
        FVector Center = FVector::ZeroVector;
        int32 BaseHostiles = 3;
        int32 BaseReward = 200;
        float TriggerRadius = 780.0f;
        float CooldownRemaining = 0.0f;
    };

    void BuildZones();
    void BuildOutpostMarkers();
    void TryTriggerEncounter(APawn* PlayerPawn);
    void StartEncounter(int32 ZoneIndex, APawn* PlayerPawn);
    void CompleteEncounter(APawn* PlayerPawn);
    void CleanupEncounter();
    void LoadFactionProgress();
    void SaveFactionProgress();
    FString GetFactionName(EGTTRuralFaction Faction) const;

    TArray<FFactionZone> Zones;
    UPROPERTY() TArray<TObjectPtr<AGTTCitizenPawn>> ActiveHostiles;
    EGTTRuralFaction ActiveFaction = EGTTRuralFaction::None;
    int32 ActiveZoneIndex = INDEX_NONE;
    int32 FactionVictories = 0;
    int32 RustDogsDefeated = 0;
    int32 StoneCrowsDefeated = 0;
    int32 MudJackalsDefeated = 0;
    float EvaluationClock = 0.0f;
    float EncounterElapsed = 0.0f;
    FString SaveSlotName = TEXT("GTT_Factions_01");
};
