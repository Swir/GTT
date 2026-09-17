#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/GTTInteractable.h"
#include "GTTLogisticsDispatcherPawn.generated.h"

class AGTTDayNightCycle;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTLogisticsDispatcherPawn : public ACharacter, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTLogisticsDispatcherPawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|NPC|Logistics")
    void ConfigureDispatcher(FName InRoleTag, const FString& InDisplayName, const FVector& InWorkLocation, const FVector& InHomeLocation);

    UFUNCTION(BlueprintPure, Category="GTT|NPC|Logistics")
    bool IsOnShift() const;

    UFUNCTION(BlueprintPure, Category="GTT|NPC|Logistics")
    FName GetRoleTag() const { return RoleTag; }

    UFUNCTION(BlueprintPure, Category="GTT|NPC|Logistics")
    FString GetDisplayName() const { return DisplayName; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC|Logistics") TObjectPtr<UStaticMeshComponent> BodyMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC|Logistics") TObjectPtr<UStaticMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|NPC|Logistics") TObjectPtr<UTextRenderComponent> RoleLabel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Logistics") float WorkStartHour = 7.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Logistics") float WorkEndHour = 17.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|NPC|Logistics") float WalkSpeed = 165.0f;

private:
    FVector ResolveScheduleTarget() const;
    FString BuildOnShiftStatusLine() const;

    FName RoleTag = NAME_None;
    FString DisplayName = TEXT("Dispatcher");
    FVector WorkLocation = FVector::ZeroVector;
    FVector HomeLocation = FVector::ZeroVector;
    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
};
