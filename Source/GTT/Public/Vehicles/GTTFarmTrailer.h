#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTFarmTrailer.generated.h"

class AGTTFieldmasterNativePawn;
class AGTTVehicleBase;
class UPhysicsConstraintComponent;
class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTFarmTrailer : public AActor
{
    GENERATED_BODY()

public:
    AGTTFarmTrailer();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer")
    bool AttachToVehicle(AGTTVehicleBase* Vehicle);

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer")
    bool AttachToNativeFieldmaster(AGTTFieldmasterNativePawn* Vehicle);

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer")
    void DetachTrailer();

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer")
    void SetCargoLoaded(bool bLoaded);

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    bool IsAttached() const { return bAttached; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    bool IsAttachedToNativeFieldmaster() const { return NativeTowVehicle != nullptr; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    bool HasCargo() const { return bCargoLoaded; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    float GetCargoIntegrity() const { return CargoIntegrity; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    float GetTrailerIntegrity() const { return TrailerIntegrity; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    float GetHitchLoad() const { return HitchLoad; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    AGTTVehicleBase* GetTowVehicle() const { return TowVehicle; }

    UFUNCTION(BlueprintPure, Category="GTT|Trailer")
    AActor* GetTowActor() const;

    UFUNCTION(BlueprintCallable, Category="GTT|Trailer")
    void ResetTrailer(const FTransform& Transform);

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> TrailerBody;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> LeftWheel;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> RightWheel;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> CargoBlock;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPhysicsConstraintComponent> HitchConstraint;

    UPROPERTY()
    TObjectPtr<AGTTVehicleBase> TowVehicle;

    UPROPERTY()
    TObjectPtr<AGTTFieldmasterNativePawn> NativeTowVehicle;

    bool bAttached = false;
    bool bCargoLoaded = false;
    float CargoIntegrity = 1.0f;
    float TrailerIntegrity = 1.0f;
    float HitchLoad = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer")
    float SafeHitchDistance = 360.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Trailer")
    float BreakHitchDistance = 760.0f;
};
