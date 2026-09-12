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

protected:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void TryInteract();
    void QuickSave();
    void QuickLoad();
    void CycleRadio();

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Interaction", meta=(ClampMin="50.0"))
    float InteractionDistance = 350.0f;
};
