#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GTTCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UGTTWantedComponent;
class UGTTPlayerEconomyComponent;
class UGTTRadioComponent;
class UGTTCombatComponent;
class UGTTCombatPresentationComponent;

UCLASS()
class GTT_API AGTTCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGTTCharacter();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category="GTT|Wanted")
    UGTTWantedComponent* GetWantedComponent() const { return WantedComponent; }

    UFUNCTION(BlueprintPure, Category="GTT|Economy")
    UGTTPlayerEconomyComponent* GetEconomyComponent() const { return EconomyComponent; }

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    UGTTRadioComponent* GetRadioComponent() const { return RadioComponent; }

    UFUNCTION(BlueprintPure, Category="GTT|Combat")
    UGTTCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void TryInteract();
    void QuickSave();
    void QuickLoad();
    void CycleRadio();
    void Attack();
    void CycleWeapon();
    void DropWeapon();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Character")
    TObjectPtr<UStaticMeshComponent> PlaceholderBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Wanted")
    TObjectPtr<UGTTWantedComponent> WantedComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Economy")
    TObjectPtr<UGTTPlayerEconomyComponent> EconomyComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Radio")
    TObjectPtr<UGTTRadioComponent> RadioComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Combat")
    TObjectPtr<UGTTCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Combat|Presentation")
    TObjectPtr<UGTTCombatPresentationComponent> CombatPresentationComponent;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Interaction", meta=(ClampMin="50.0"))
    float InteractionDistance = 350.0f;
};
