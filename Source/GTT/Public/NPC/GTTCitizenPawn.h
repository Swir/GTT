#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTTCitizenPawn.generated.h"
class AGTTVehicleBase;
class AGTTDayNightCycle;
class UStaticMeshComponent;
UCLASS()
class GTT_API AGTTCitizenPawn : public ACharacter
{
    GENERATED_BODY()
public:
    AGTTCitizenPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable, Category="GTT|NPC|Crime") bool TryWitnessVehicleTheft(AGTTVehicleBase* Vehicle, APawn* Offender);
protected:
    void ChooseNewWanderTarget();
    FVector GetScheduleCenter() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC") TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC") TObjectPtr<UStaticMeshComponent> HeadMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Crime", meta=(ClampMin="100.0")) float WitnessRadius = 1800.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Crime", meta=(ClampMin="0.0")) float WitnessHeat = 9.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Movement", meta=(ClampMin="100.0")) float WanderRadius = 750.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Movement", meta=(ClampMin="0.0")) float WanderSpeed = 135.0f;
private:
    FVector HomeLocation = FVector::ZeroVector;
    FVector WorkLocation = FVector::ZeroVector;
    FVector SocialLocation = FVector::ZeroVector;
    FVector WanderTarget = FVector::ZeroVector;
    FVector LastScheduleCenter = FVector::ZeroVector;
    float RetargetTimeRemaining = 0.0f;
    TWeakObjectPtr<AGTTVehicleBase> LastWitnessedVehicle;
    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
};
