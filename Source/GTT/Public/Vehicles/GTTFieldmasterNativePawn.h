#pragma once

#include "CoreMinimal.h"
#include "Interaction/GTTInteractable.h"
#include "WheeledVehiclePawn.h"
#include "GTTFieldmasterNativePawn.generated.h"

class AGTTVehicleBase;
class UCameraComponent;
class UChaosWheeledVehicleMovementComponent;
class UInputComponent;
class USpringArmComponent;
class UPrimitiveComponent;
class AActor;
struct FHitResult;

USTRUCT(BlueprintType)
struct FGTTVehicleMigrationSnapshot
{
    GENERATED_BODY()

    // Canonical vehicle-health ratio, matching AGTTVehicleBase::GetConditionPercent(): 0.0 = broken, 1.0 = healthy.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ConditionPercent = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration")
    float FuelLiters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration")
    bool bOwnedByPlayer = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration")
    int32 EngineUpgradeLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration")
    int32 TireUpgradeLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration")
    float TireIntegrity = 1.0f;
};

UCLASS(Blueprintable)
class GTT_API AGTTFieldmasterNativePawn : public AWheeledVehiclePawn, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTFieldmasterNativePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos")
    bool ConfigureAndValidateNativeFieldmaster(FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Migration")
    bool ImportLegacyGameplayState(const AGTTVehicleBase* LegacyVehicle, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Migration")
    void ApplyMigrationSnapshot(const FGTTVehicleMigrationSnapshot& Snapshot);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Takeover")
    bool TryActivateLegacyTakeover();

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Takeover")
    void DeactivateLegacyTakeover();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle")
    void ExitNativeVehicle();

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Garage")
    bool RecallToTransform(const FTransform& Destination);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Terrain")
    void ApplyNativeMudResponse(float DragStrength, float TireWearPerSecond, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Damage")
    void ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale = 1.0f);

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Terrain")
    float GetNativeMudSeverity() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Terrain")
    float GetNativeTerrainGripFactor() const;

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Trailer")
    bool TryGetRearHitchTransform(FTransform& OutTransform) const;

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Migration")
    FGTTVehicleMigrationSnapshot GetMigrationSnapshot() const { return MigrationSnapshot; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Input")
    float GetRequestedThrottleInput() const { return LastThrottleInput; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos")
    bool IsNativeFieldmasterReady() const { return bNativeReady; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Takeover")
    bool IsLegacyTakeoverActive() const { return bTakeoverActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos")
    FString GetNativeAcceptanceSummary() const { return NativeAcceptanceSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Save")
    FName GetPersistentVehicleId() const { return TEXT("RustyFieldmaster60"); }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    FText GetVehicleDisplayName() const { return NSLOCTEXT("GTT", "NativeFieldmasterName", "Rusty Fieldmaster 60"); }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    APawn* GetDriverPawn() const { return PreviousPawn.Get(); }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle")
    bool IsOccupied() const { return bOccupied; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Ownership")
    bool IsOwnedByPlayer() const { return MigrationSnapshot.bOwnedByPlayer; }

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera")
    TObjectPtr<UCameraComponent> VehicleCamera;

private:
    bool ValidateRigContract(FString& OutSummary) const;
    void HandleNativeThrottle(float Value);
    void HandleNativeSteering(float Value);
    void QuickSave();
    void QuickLoad();
    void CycleRadio();
    void SyncLegacyMirror();

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos")
    bool bNativeReady = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos")
    FString NativeAcceptanceSummary = TEXT("Not validated");

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Migration")
    FGTTVehicleMigrationSnapshot MigrationSnapshot;

    TWeakObjectPtr<APawn> PreviousPawn;
    TWeakObjectPtr<AGTTVehicleBase> LegacyMirror;
    bool bTakeoverActive = false;
    bool bOccupied = false;
    float LastThrottleInput = 0.0f;
    float MirrorSyncAccumulator = 0.0f;
    float TakeoverRetryAccumulator = 0.0f;
    float LastImpactDamageTimeSeconds = -100.0f;
    float LastNativeMudResponseTimeSeconds = -100.0f;
    float LastNativeMudSeverity = 0.0f;
};
