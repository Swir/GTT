#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "GTTFieldmasterNativePawn.generated.h"

class AGTTVehicleBase;
class UChaosWheeledVehicleMovementComponent;
class UInputComponent;

USTRUCT(BlueprintType)
struct FGTTVehicleMigrationSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GTT|Vehicle|Migration")
    float ConditionPercent = 100.0f;

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
class GTT_API AGTTFieldmasterNativePawn : public AWheeledVehiclePawn
{
    GENERATED_BODY()

public:
    AGTTFieldmasterNativePawn();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos")
    bool ConfigureAndValidateNativeFieldmaster(FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Migration")
    bool ImportLegacyGameplayState(const AGTTVehicleBase* LegacyVehicle, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Migration")
    void ApplyMigrationSnapshot(const FGTTVehicleMigrationSnapshot& Snapshot);

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Migration")
    FGTTVehicleMigrationSnapshot GetMigrationSnapshot() const { return MigrationSnapshot; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos")
    bool IsNativeFieldmasterReady() const { return bNativeReady; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos")
    FString GetNativeAcceptanceSummary() const { return NativeAcceptanceSummary; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Save")
    FName GetPersistentVehicleId() const { return TEXT("RustyFieldmaster60"); }

protected:
    virtual void BeginPlay() override;

private:
    bool ValidateRigContract(FString& OutSummary) const;
    void HandleNativeThrottle(float Value);
    void HandleNativeSteering(float Value);

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos")
    bool bNativeReady = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos")
    FString NativeAcceptanceSummary = TEXT("Not validated");

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Migration")
    FGTTVehicleMigrationSnapshot MigrationSnapshot;
};
