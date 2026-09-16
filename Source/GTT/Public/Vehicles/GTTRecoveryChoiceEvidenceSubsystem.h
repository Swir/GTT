#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTRecoveryChoiceEvidenceSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class AGTTServiceTerminal;

/** Packaged-demo proof for explicit roadside choice, tow-only transport and separate paid repair. */
UCLASS()
class GTT_API UGTTRecoveryChoiceEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTRecoveryChoiceEvidenceSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return bEvidenceEnabled && !bFinished; }

private:
    enum class EPhase : uint8 { WaitForStructuralDrive, StageStrandedVehicle, VerifyOffer, VerifyNoAutoTow, RequestTow, VerifyTow, InvokeRepair, VerifyRepair, Complete };
    void Fail(const FString& Reason);
    AGTTRoadVehicleNativePawn* FindEvidenceVehicle() const;
    AGTTServiceTerminal* FindWorkshopTerminal() const;

    bool bEvidenceEnabled = false;
    bool bFinished = false;
    float Elapsed = 0.0f;
    float PhaseStarted = 0.0f;
    EPhase Phase = EPhase::WaitForStructuralDrive;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> EvidenceVehicle;
    FName EvidenceVehicleId = NAME_None;
    FVector StrandedLocation = FVector::ZeroVector;
    float SavedCondition = 1.0f;
    float SavedTires = 1.0f;
    float SavedBodyMin = 1.0f;
    int32 OfferCash = 0;
    int32 TowQuote = 0;
    int32 RepairQuote = 0;
    int32 CashBeforeTow = 0;
    int32 CashBeforeRepair = 0;
};