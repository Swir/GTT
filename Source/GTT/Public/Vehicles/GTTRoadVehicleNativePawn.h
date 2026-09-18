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
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EGTTRoadDamageZone : uint8 { Front, Rear, Left, Right };

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

USTRUCT(BlueprintType)
struct GTT_API FGTTRoadBodyDamageSnapshot
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float FrontHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RearHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float LeftHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RightHealth = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CoolingStress = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 DetachedPanelCount = 0;
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
    virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fleet") bool ConfigureAndValidateNativeRoadVehicle(FString& OutSummary);
    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fleet") bool TryActivateLegacyTakeover();
    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fleet") void DeactivateLegacyTakeover();
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle") void ExitNativeVehicle();
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Cargo") virtual void SetCargoLoadFactor(float NewLoadFactor);
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Workshop") bool ApplyNativeWorkshopService();
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Recovery") bool ApplyNativeEmergencyRoadsidePatch()
    {
        if (!bTakeoverActive) return false;
        const float PreviousCondition = MigrationSnapshot.ConditionPercent;
        const float PreviousTires = MigrationSnapshot.TireIntegrity;
        const float PreviousFuel = MigrationSnapshot.FuelLiters;
        MigrationSnapshot.ConditionPercent = FMath::Max(MigrationSnapshot.ConditionPercent, 0.30f);
        MigrationSnapshot.TireIntegrity = FMath::Max(MigrationSnapshot.TireIntegrity, 0.32f);
        MigrationSnapshot.FuelLiters = FMath::Max(MigrationSnapshot.FuelLiters, FMath::Min(FuelCapacityLiters, 5.0f));
        const bool bChanged = !FMath::IsNearlyEqual(PreviousCondition, MigrationSnapshot.ConditionPercent, 0.001f)
            || !FMath::IsNearlyEqual(PreviousTires, MigrationSnapshot.TireIntegrity, 0.001f)
            || !FMath::IsNearlyEqual(PreviousFuel, MigrationSnapshot.FuelLiters, 0.001f);
        if (bChanged) SyncLegacyMirror();
        return bChanged;
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Service") bool RepairNativeTires()
    {
        if (!bTakeoverActive || MigrationSnapshot.TireIntegrity >= 0.999f) return false;
        MigrationSnapshot.TireIntegrity = 1.0f;
        SyncLegacyMirror();
        return true;
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Service") bool InstallNativeEngineUpgrade()
    {
        if (!bTakeoverActive || MigrationSnapshot.EngineUpgradeLevel >= 3) return false;
        MigrationSnapshot.EngineUpgradeLevel = FMath::Clamp(MigrationSnapshot.EngineUpgradeLevel + 1, 0, 3);
        SyncLegacyMirror();
        return true;
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Service") bool InstallNativeTireUpgrade()
    {
        if (!bTakeoverActive || MigrationSnapshot.TireUpgradeLevel >= 3) return false;
        MigrationSnapshot.TireUpgradeLevel = FMath::Clamp(MigrationSnapshot.TireUpgradeLevel + 1, 0, 3);
        SyncLegacyMirror();
        return true;
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Service") float RefuelNativeVehicle(float Liters)
    {
        if (!bTakeoverActive || Liters <= 0.0f) return 0.0f;
        const float Before = MigrationSnapshot.FuelLiters;
        MigrationSnapshot.FuelLiters = FMath::Clamp(MigrationSnapshot.FuelLiters + Liters, 0.0f, FuelCapacityLiters);
        SyncLegacyMirror();
        return MigrationSnapshot.FuelLiters - Before;
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Damage") bool ApplyPoliceSpikeDamage(float TireDamage, float ConditionDamage)
    {
        if (!bNativeReady || !bTakeoverActive) return false;
        const float PreviousTires = MigrationSnapshot.TireIntegrity;
        MigrationSnapshot.TireIntegrity = FMath::Clamp(MigrationSnapshot.TireIntegrity - FMath::Max(0.0f, TireDamage), 0.0f, 1.0f);
        MigrationSnapshot.ConditionPercent = FMath::Clamp(MigrationSnapshot.ConditionPercent - FMath::Max(0.0f, ConditionDamage), 0.0f, 1.0f);
        SyncLegacyMirror();
        return MigrationSnapshot.TireIntegrity < PreviousTires;
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Damage") bool ApplyScriptedImpactDamage(float ImpactSpeedKmh, EGTTRoadDamageZone Zone);
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Save") void RestorePersistentBodyDamage(const FGTTRoadBodyDamageSnapshot& InDamage, int32 DetachedPanelMask);
    /** Restore a previously captured native migration snapshot and mirror it back to the legacy fleet actor. */
    void RestorePersistentMigrationSnapshot(const FGTTRoadVehicleMigrationSnapshot& InSnapshot)
    {
        MigrationSnapshot = InSnapshot;
        SyncLegacyMirror();
    }
    UFUNCTION(BlueprintCallable, Category="GTT|Vehicle|Save") void FlushNativePersistenceMirror();

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fleet") bool IsNativeReady() const { return bNativeReady; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fleet") bool IsLegacyTakeoverActive() const { return bTakeoverActive; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Save") FName GetPersistentVehicleId() const { return NativeVehicleId; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle") FText GetVehicleDisplayName() const { return NativeDisplayName; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle") APawn* GetDriverPawn() const { return PreviousPawn.Get(); }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Cargo") float GetCargoLoadFactor() const { return CargoLoadFactor; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Service") float GetFuelCapacityLiters() const { return FuelCapacityLiters; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Migration") FGTTRoadVehicleMigrationSnapshot GetMigrationSnapshot() const { return MigrationSnapshot; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") float GetRuntimeWheelRisk() const { return RuntimeWheelRisk; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") int32 GetRuntimeWheelContacts() const { return RuntimeWheelContacts; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") float GetRuntimeThrottleLimit() const { return RuntimeThrottleLimit; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") float GetRuntimeSteeringLimit() const { return RuntimeSteeringLimit; }
    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Runtime") float GetRuntimeBrakeAssist() const { return RuntimeBrakeAssist; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage") float GetLastImpactSpeedKmh() const { return LastImpactSpeedKmh; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage") int32 GetNativeImpactCount() const { return NativeImpactCount; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage") EGTTRoadDamageZone GetLastImpactZone() const { return LastImpactZone; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage") FGTTRoadBodyDamageSnapshot GetBodyDamageSnapshot() const { return BodyDamage; }
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Damage") int32 GetDetachedPanelMask() const;
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Workshop") bool NeedsNativeWorkshopService() const;
    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Workshop") int32 GetBodyDamageRepairSurcharge() const;

protected:
    virtual void BeginPlay() override;
    virtual float GetCargoPowerLimit(float SpeedKmh) const;
    virtual float GetCargoSteeringLimit(float SpeedKmh) const;
    virtual float GetImpactDamageScale() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Camera") TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage") TObjectPtr<UStaticMeshComponent> FrontDamageDebris;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage") TObjectPtr<UStaticMeshComponent> RearDamageDebris;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage") TObjectPtr<UStaticMeshComponent> LeftDamageDebris;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Damage") TObjectPtr<UStaticMeshComponent> RightDamageDebris;
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
    void UpdateDamageConsequences(float DeltaSeconds);
    void ApplyNativeImpactDamage(float ImpactSpeedKmh, EGTTRoadDamageZone Zone, const FVector& HitLocation, const FVector& NormalImpulse);
    EGTTRoadDamageZone DetermineImpactZone(const FVector& HitLocation) const;
    float& ResolveDamageZoneHealth(EGTTRoadDamageZone Zone);
    float GetDamageZoneHealth(EGTTRoadDamageZone Zone) const;
    void TryDetachDamagePanel(EGTTRoadDamageZone Zone, const FVector& HitLocation, const FVector& NormalImpulse, float ImpactSpeedKmh);
    void ConfigureDamageDebrisLayout();
    void RestoreNativeBodyDamage();
    void StopNativeDriveForBreakdown();
    static const TCHAR* DamageZoneToString(EGTTRoadDamageZone Zone);
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
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Damage") EGTTRoadDamageZone LastImpactZone = EGTTRoadDamageZone::Front;
    UPROPERTY(VisibleInstanceOnly, Category="GTT|Vehicle|Damage") FGTTRoadBodyDamageSnapshot BodyDamage;
    TWeakObjectPtr<APawn> PreviousPawn;
    TWeakObjectPtr<AGTTVehicleBase> LegacyMirror;
    float LastThrottleInput = 0.0f;
    float RuntimeThrottleLimit = 1.0f;
    float RuntimeSteeringLimit = 1.0f;
    float RuntimeBrakeAssist = 0.0f;
    float DamageThrottleLimit = 1.0f;
    float DamageSteeringLimit = 1.0f;
    float DamageSteeringBias = 0.0f;
    float MirrorSyncAccumulator = 0.0f;
    float TakeoverRetryAccumulator = 0.0f;
    float RuntimeGuardAccumulator = 0.0f;
    float WheelEvidenceAccumulator = 0.0f;
    float DamageEvidenceAccumulator = 0.0f;
    float LastImpactDamageTimeSeconds = -1000.0f;
    bool bFrontPanelDetached = false;
    bool bRearPanelDetached = false;
    bool bLeftPanelDetached = false;
    bool bRightPanelDetached = false;
};

UCLASS(Blueprintable)
class GTT_API AGTTRattlebackNativePawn : public AGTTRoadVehicleNativePawn
{
    GENERATED_BODY()
public: AGTTRattlebackNativePawn();
protected: virtual float GetImpactDamageScale() const override;
};

UCLASS(Blueprintable)
class GTT_API AGTTMuleboxNativePawn : public AGTTRoadVehicleNativePawn
{
    GENERATED_BODY()
public: AGTTMuleboxNativePawn();
protected:
    virtual float GetCargoPowerLimit(float SpeedKmh) const override;
    virtual float GetCargoSteeringLimit(float SpeedKmh) const override;
    virtual float GetImpactDamageScale() const override;
};
