#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTFarmTrailer.generated.h"

class APawn;
class AGTTFieldmasterNativePawn;
class AGTTVehicleBase;
class UPhysicsConstraintComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FHitResult;

UCLASS()
class GTT_API AGTTFarmTrailer : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTFarmTrailer();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer") bool AttachToVehicle(AGTTVehicleBase* Vehicle);
    UFUNCTION(BlueprintCallable, Category="GTT|Trailer") bool AttachToNativeFieldmaster(AGTTFieldmasterNativePawn* Vehicle);
    UFUNCTION(BlueprintCallable, Category="GTT|Trailer") void DetachTrailer();
    UFUNCTION(BlueprintCallable, Category="GTT|Trailer") void SetCargoLoaded(bool bLoaded);
    UFUNCTION(BlueprintCallable, Category="GTT|Trailer|Recovery") bool PerformRoadsideRepair(float IntegrityRestore = 0.35f);

    UFUNCTION(BlueprintPure, Category="GTT|Trailer") bool IsAttached() const { return bAttached; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") bool IsAttachedToNativeFieldmaster() const { return NativeTowVehicle != nullptr; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") bool HasCargo() const { return bCargoLoaded; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") float GetCargoIntegrity() const { return CargoIntegrity; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") float GetTrailerIntegrity() const { return TrailerIntegrity; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") float GetHitchLoad() const { return HitchLoad; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") AGTTVehicleBase* GetTowVehicle() const { return TowVehicle; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") AActor* GetTowActor() const;
    UFUNCTION(BlueprintPure, Category="GTT|Trailer") bool HasIntactAxle() const;
    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Recovery") int32 GetLostWheelCount() const { return (bLeftWheelLost ? 1 : 0) + (bRightWheelLost ? 1 : 0); }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Presentation") bool IsPresentationDamaged() const { return TrailerIntegrity < 0.55f || !HasIntactAxle(); }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Recovery") bool NeedsRoadsideRepair() const;
    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Recovery") bool IsRoadsideRepairPending() const { return bRoadsideRepairPending; }
    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Recovery") int32 GetRoadsideRepairQuote() const;
    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Recovery") float GetRoadsideRepairDuration() const;
    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Recovery") float GetRoadsideRepairTimeRemaining() const { return RoadsideRepairTimeRemaining; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer|Dynamics")
    float GetTowLoadFactor() const
    {
        if (!bAttached) return 0.0f;
        const float CargoMassLoad = bCargoLoaded ? 0.55f : 0.22f;
        const float HitchStress = FMath::Clamp(HitchLoad, 0.0f, 1.0f) * 0.28f;
        const float DamageLoad = (1.0f - FMath::Clamp(TrailerIntegrity, 0.0f, 1.0f)) * 0.20f;
        const float AxleLoad = static_cast<float>(GetLostWheelCount()) * 0.18f;
        const float CargoShiftLoad = bCargoLoaded ? (1.0f - FMath::Clamp(CargoIntegrity, 0.0f, 1.0f)) * 0.12f : 0.0f;
        return FMath::Clamp(CargoMassLoad + HitchStress + DamageLoad + AxleLoad + CargoShiftLoad, 0.0f, 1.0f);
    }

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer") void ResetTrailer(const FTransform& Transform);

private:
    void ConfigureWheelAxle(UPhysicsConstraintComponent* Constraint, UStaticMeshComponent* Wheel);
    void ConfigureHitchConstraint();
    void RefreshAxleState();
    void RefreshPresentation();
    void RestoreWheel(UStaticMeshComponent* Wheel, UPhysicsConstraintComponent* Constraint, const FVector& RelativeLocation);
    void SetCargoVisualsVisible(bool bVisible);
    bool CanBeginRoadsideRepair(APawn* RepairPawn, FString& OutReason) const;
    void BeginRoadsideRepair(APawn* RepairPawn);
    void CancelRoadsideRepair(const FString& Reason);
    void CompleteRoadsideRepair();

    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TrailerBody;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LeftWheel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RightWheel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> LeftWheelConstraint;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> RightWheelConstraint;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CargoBlock;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> Drawbar;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> HitchCoupler;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> FrontRail;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> LeftRail;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> RightRail;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> Tailgate;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> LeftFender;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> RightFender;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> RearReflectorBar;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> CargoLogA;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> CargoLogB;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> CargoLogC;
    UPROPERTY(VisibleAnywhere, Category="GTT|Trailer|Presentation") TObjectPtr<UStaticMeshComponent> CargoLogD;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsConstraintComponent> HitchConstraint;
    UPROPERTY() TObjectPtr<AGTTVehicleBase> TowVehicle;
    UPROPERTY() TObjectPtr<AGTTFieldmasterNativePawn> NativeTowVehicle;

    bool bAttached = false;
    bool bCargoLoaded = false;
    bool bLeftWheelLost = false;
    bool bRightWheelLost = false;
    bool bRoadsideRepairPending = false;
    float CargoIntegrity = 1.0f;
    float TrailerIntegrity = 1.0f;
    float HitchLoad = 0.0f;
    float LastImpactDamageTimeSeconds = -100.0f;
    float RoadsideRepairTimeRemaining = 0.0f;
    float LockedRoadsideRepairDuration = 0.0f;
    int32 LockedRoadsideRepairQuote = 0;
    TWeakObjectPtr<APawn> RoadsideRepairPlayer;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer") float SafeHitchDistance = 360.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer") float BreakHitchDistance = 760.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Axle") float WheelBreakForce = 420000.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Axle") float WheelBreakTorque = 260000.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Recovery") float RoadsideRepairMaxDistance = 500.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Recovery") float RoadsideRepairMaxSpeedKmh = 1.5f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer|Recovery") float RoadsideRepairMaxHitchLoad = 0.15f;
};