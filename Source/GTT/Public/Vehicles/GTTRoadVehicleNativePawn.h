#pragma once

#include "CoreMinimal.h"
#include "Interaction/GTTInteractable.h"
#include "WheeledVehiclePawn.h"
#include "GTTRoadVehicleNativePawn.generated.h"

class AGTTVehicleBase;
class UCameraComponent;
class UChaosWheeledVehicleMovementComponent;
class UPrimitiveComponent;
class USpringArmComponent;

USTRUCT(BlueprintType)
struct GTT_API FGTTRoadVehicleMigrationSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ConditionPercent = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FuelLiters = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOwnedByPlayer = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EngineUpgradeLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TireUpgradeLevel = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float TireIntegrity = 1.0f;
};

UCLASS(Abstract, Blueprintable)
class GTT_API AGTTRoadVehicleNativePawn : public AWheeledVehiclePawn, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTRoadVehicleNativePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
        FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fleet") bool ConfigureAndValidateNativeRoadVehicle(FString& OutSummary);
    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fleet") bool TryActivateLegacyTakeover();
    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fleet") void DeactivateLegacyTakeover();
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle") void ExitNativeVehicle();
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Cargo") virtual void SetCargoLoadFactor(float NewLoadFactor);

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fleet") bool IsNativeReady() const { return bNativeReady; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fleet") bool IsLegacyTakeoverActive() const { return bTakeoverActive; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Save") FName GetPersistentVehicleId() const { return NativeVehicleId; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle") FText GetVehicleDisplayName() const { return NativeDisplayName; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle") APawn* GetDriverPawn() const { return PreviousPawn.Get(); }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Cargo") float GetCargoLoadFactor() const { return CargoLoadFactor; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Migration") FGTTRoadVehicleMigrationSnapshot GetMigrationSnapshot() const { return MigrationSnapshot; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") float GetRuntimeWheelRisk() const { return RuntimeWheelRisk; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") int32 GetRuntimeWheelContacts() const { return RuntimeWheelContacts; }

protected:
    virtual void BeginPlay() override;
    virtual float GetCargoPowerLimit(float SpeedKmh) const;
    virtual float GetCargoSteeringLimit(float SpeedKmh) const;
    virtual float GetImpactDamageScale() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera") TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera") TObjectPtr<UCameraComponent> VehicleCamera;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Chaos|Fleet") FName NativeVehicleId = NAME_None;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Chaos|Fleet") FText NativeDisplayName;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Chaos|Fleet") float FuelCapacityLiters = 45.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Chaos|Fleet") float IdleFuelBurnPerSecond = 0.03f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Chaos|Fleet") float FullThrottleFuelBurnPerSecond = 0.20f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Chaos|Fleet") FVector ExitOffset = FVector(0.0f, 180.0f, 70.0f);

private:
    bool ValidateRigContract(FString& OutSummary) const;
    bool ImportLegacyGameplayState(const AGTTVehicleBase* LegacyVehicle, FString& OutSummary);
    void HandleNativeThrottle(float Value);
    void HandleNativeSteering(float Value);
    void SyncLegacyMirror();
    void RuntimeAcceptanceGuard();
    void UpdateNativeWheelRuntime(float DeltaSeconds);
    void ApplyNativeImpactDamage(float ImpactSpeedKmh);
    void StopNativeDriveForBreakdown();

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fleet") bool bNativeReady = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fleet") bool bTakeoverActive = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fleet") bool bOccupied = false;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fleet") FString NativeAcceptanceSummary = TEXT("Not validated");
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Migration") FGTTRoadVehicleMigrationSnapshot MigrationSnapshot;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Cargo") float CargoLoadFactor = 0.0f;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Runtime") float RuntimeWheelRisk = 0.0f;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Runtime") int32 RuntimeWheelContacts = 0;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Damage") float LastImpactSpeedKmh = 0.0f;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Damage") int32 NativeImpactCount = 0;

    TWeakObjectPtr<APawn> PreviousPawn;
    TWeakObjectPtr<AGTTVehicleBase> LegacyMirror;
    float LastThrottleInput = 0.0f;
    float RuntimeThrottleLimit = 1.0f;
    float RuntimeSteeringLimit = 1.0f;
    float RuntimeBrakeAssist = 0.0f;
    float MirrorSyncAccumulator = 0.0f;
    float TakeoverRetryAccumulator = 0.0f;
    float RuntimeGuardAccumulator = 0.0f;
    float WheelEvidenceAccumulator = 0.0f;
    float LastImpactDamageTimeSeconds = -1000.0f;
};

UCLASS(Blueprintable)
class GTT_API AGTTRattlebackNativePawn : public AGTTRoadVehicleNativePawn
{
    GENERATED_BODY()
public:
    AGTTRattlebackNativePawn();
protected:
    virtual float GetImpactDamageScale() const override;
};

UCLASS(Blueprintable)
class GTT_API AGTTMuleboxNativePawn : public AGTTRoadVehicleNativePawn
{
    GENERATED_BODY()
public:
    AGTTMuleboxNativePawn();
protected:
    virtual float GetCargoPowerLimit(float SpeedKmh) const override;
    virtual float GetCargoSteeringLimit(float SpeedKmh) const override;
    virtual float GetImpactDamageScale() const override;
};
