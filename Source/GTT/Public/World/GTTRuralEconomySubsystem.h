#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTRuralEconomySubsystem.generated.h"

class APawn;
class AGTTVehicleBase;

UCLASS()
class GTT_API UGTTRuralEconomySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    void AddContraband(APawn* PlayerPawn, int32 Units, int32 EstimatedValue, const FString& SourceLabel);
    bool SellContraband(APawn* PlayerPawn);
    bool BuyOrUseInsurance(APawn* PlayerPawn);
    bool ReleaseImpoundedVehicle(APawn* PlayerPawn);
    void HandleArrestImpound(APawn* PlayerPawn);

    int32 GetContrabandUnits() const { return ContrabandUnits; }
    int32 GetContrabandValue() const { return ContrabandValue; }
    bool HasInsurance() const { return bInsuranceActive; }
    bool HasImpoundedVehicle() const { return !ImpoundedVehicleId.IsNone(); }
    int32 GetPendingImpoundFee() const { return PendingImpoundFee; }
    int32 GetSpeedingCitationCount() const { return SpeedingCitations; }

private:
    void LoadState();
    void SaveState() const;
    void SpawnServicePoints(UWorld& World);
    void EvaluateRoadLaw();
    AGTTVehicleBase* FindNearestOwnedVehicle(const FVector& Origin, float Radius) const;
    AGTTVehicleBase* FindOwnedVehicleById(FName VehicleId) const;

    FString SaveSlotName = TEXT("GTT_RuralEconomy_01");
    int32 ContrabandUnits = 0;
    int32 ContrabandValue = 0;
    bool bInsuranceActive = false;
    FName ImpoundedVehicleId = NAME_None;
    int32 PendingImpoundFee = 0;
    int32 LifetimeFenceRevenue = 0;
    int32 SpeedingCitations = 0;
    float SpeedingExposureSeconds = 0.0f;
    float CitationCooldownSeconds = 0.0f;
    FTimerHandle RoadLawTimer;
};
