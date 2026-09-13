#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTTCitizenPawn.generated.h"
class AGTTVehicleBase;
class AGTTDayNightCycle;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGTTHostileArchetype : uint8
{
    Civilian,
    Scrapper,
    Runner,
    Bruiser,
    Enforcer
};

UCLASS()
class GTT_API AGTTCitizenPawn : public ACharacter
{
    GENERATED_BODY()
public:
    AGTTCitizenPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable, Category="GTT|NPC|Crime") bool TryWitnessVehicleTheft(AGTTVehicleBase* Vehicle, APawn* Offender);
    UFUNCTION(BlueprintCallable, Category="GTT|NPC|Combat") void ApplyCombatHit(float Damage, const FVector& HitDirection, float Knockback, APawn* Attacker);
    UFUNCTION(BlueprintCallable, Category="GTT|NPC|Combat") void StartBrawlWith(APawn* Opponent);
    UFUNCTION(BlueprintCallable, Category="GTT|NPC|Combat") void ConfigureHostileArchetype(EGTTHostileArchetype NewArchetype, APawn* Target);
    UFUNCTION(BlueprintPure, Category="GTT|NPC|Combat") float GetHealthPercent() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }
    UFUNCTION(BlueprintPure, Category="GTT|NPC|Combat") bool IsKnockedOut() const { return bKnockedOut; }
    UFUNCTION(BlueprintPure, Category="GTT|NPC|Combat") bool IsBrawlParticipant() const { return bBrawlParticipant; }
    UFUNCTION(BlueprintPure, Category="GTT|NPC|Combat") bool IsFactionHostile() const { return HostileArchetype != EGTTHostileArchetype::Civilian; }
    UFUNCTION(BlueprintPure, Category="GTT|NPC|Combat") EGTTHostileArchetype GetHostileArchetype() const { return HostileArchetype; }
    UFUNCTION(BlueprintPure, Category="GTT|NPC|Combat") FString GetArchetypeLabel() const;
protected:
    void ChooseNewWanderTarget();
    FVector GetScheduleCenter() const;
    void UpdateCombatBehavior(float DeltaSeconds);
    void UpdateCombatPresentation(float DeltaSeconds);
    void ConfigureCombatProp();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC") TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC") TObjectPtr<UStaticMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC|Combat") TObjectPtr<UStaticMeshComponent> CombatProp;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC|Combat") TObjectPtr<UStaticMeshComponent> CombatPropDetail;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Crime", meta=(ClampMin="100.0")) float WitnessRadius = 1800.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Crime", meta=(ClampMin="0.0")) float WitnessHeat = 9.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Movement", meta=(ClampMin="100.0")) float WanderRadius = 750.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Movement", meta=(ClampMin="0.0")) float WanderSpeed = 135.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Combat") float MaxHealth = 100.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Combat") float RetaliationDistance = 175.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Combat") float RetaliationDamage = 7.0f;
private:
    FVector HomeLocation = FVector::ZeroVector;
    FVector WorkLocation = FVector::ZeroVector;
    FVector SocialLocation = FVector::ZeroVector;
    FVector WanderTarget = FVector::ZeroVector;
    FVector LastScheduleCenter = FVector::ZeroVector;
    float RetargetTimeRemaining = 0.0f;
    TWeakObjectPtr<AGTTVehicleBase> LastWitnessedVehicle;
    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
    TWeakObjectPtr<APawn> CombatTarget;
    float Health = 100.0f;
    float CombatCooldown = 0.0f;
    float KnockoutTimeRemaining = 0.0f;
    float HitReactionTimeRemaining = 0.0f;
    float AttackPresentationTimeRemaining = 0.0f;
    FVector LastHitDirection = FVector::ForwardVector;
    bool bKnockedOut = false;
    bool bBrawlParticipant = false;
    EGTTHostileArchetype HostileArchetype = EGTTHostileArchetype::Civilian;
};
