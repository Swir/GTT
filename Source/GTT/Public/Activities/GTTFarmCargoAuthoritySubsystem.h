#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmCargoAuthoritySubsystem.generated.h"

class APawn;

// Locks a Farm Cargo contract to the exact vehicle that accepted the physical load.
// This closes the old "any healthy vehicle near the terminal" handoff loophole while
// leaving payout, reputation, Wanted and save authority in their existing systems.
UCLASS()
class GTT_API UGTTFarmCargoAuthoritySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ResetForNewContract();
    bool CaptureLoadedVehicle(APawn* PlayerPawn);
    bool ValidateHandoff(const FVector& HandoffLocation, FString& OutFailureReason, float& OutSpeedKmh, float& OutDistanceCm) const;
    void MarkAcceptedHandoff(const TCHAR* StopLabel, bool bContractComplete, float SpeedKmh, float DistanceCm) const;
    void ClearLoadedVehicle(const TCHAR* Reason = TEXT("clear"));

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|CargoAuthority")
    bool HasLoadedVehicle() const { return LoadedVehicle.IsValid(); }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|CargoAuthority")
    FName GetLoadedVehicleId() const { return LoadedVehicleId; }

private:
    AActor* ResolvePickupCandidate(APawn* PlayerPawn) const;
    bool IsUsableCargoVehicle(const AActor* Candidate) const;
    static FName ResolveVehicleId(const AActor* Candidate);

    TWeakObjectPtr<AActor> LoadedVehicle;
    FName LoadedVehicleId = NAME_None;
};
