#include "Vehicles/GTTRoadsideRecoverySubsystem.h"

#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    constexpr float RecoveryScanIntervalSeconds = 0.5f;
    constexpr float PoliceImpoundArmSeconds = 7.0f;
    constexpr float PlayerTowDispatchSeconds = 2.5f;
    constexpr float PlayerPatchDispatchSeconds = 3.0f;
    constexpr float RecoveryCooldownSeconds = 12.0f;
    constexpr float MaxRecoverySpeedKmh = 3.5f;
    const FVector WorkshopBaseLocation(-400.0f, 2650.0f, 105.0f);
}

TStatId UGTTRoadsideRecoverySubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTRoadsideRecoverySubsystem, STATGROUP_Tickables); }

void UGTTRoadsideRecoverySubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;
    for (auto& Pair : RuntimeByVehicle) Pair.Value.CooldownSeconds = FMath::Max(0.0f, Pair.Value.CooldownSeconds - DeltaSeconds);

    // The native road pawn owns vehicle input while possessed, so recovery choice input is read at world scope.
    if (APlayerController* PlayerController = World->GetFirstPlayerController())
    {
        AGTTRoadVehicleNativePawn* NativeVehicle = Cast<AGTTRoadVehicleNativePawn>(PlayerController->GetPawn());
        if (PlayerController->WasInputKeyJustPressed(EKeys::Y) || PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Left))
        {
            RequestEmergencyRoadsidePatch(NativeVehicle);
        }
        if (PlayerController->WasInputKeyJustPressed(EKeys::T) || PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))
        {
            RequestRoadsideTow(NativeVehicle);
        }
    }

    ScanAccumulator += DeltaSeconds;
    if (ScanAccumulator < RecoveryScanIntervalSeconds) return;
    const float Step = ScanAccumulator;
    ScanAccumulator = 0.0f;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It) UpdateVehicle(*It, Step);
}

bool UGTTRoadsideRecoverySubsystem::IsRecoveryEligible(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetDriverPawn()) return false;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    const bool bMechanicalBreakdown = State.ConditionPercent <= 0.05f;
    const bool bOutOfFuel = State.FuelLiters <= 0.05f;
    const bool bTiresDisabled = State.TireIntegrity <= 0.08f;
    const bool bBodyDisabled = Body.DetachedPanelCount >= 3 && FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth)) <= 0.20f;
    return Vehicle->GetVelocity().Size() * 0.036f <= MaxRecoverySpeedKmh && (bMechanicalBreakdown || bOutOfFuel || bTiresDisabled || bBodyDisabled);
}

bool UGTTRoadsideRecoverySubsystem::IsPlayerRecoveryChoiceEligible(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetDriverPawn()) return false;
    if (Vehicle->GetVelocity().Size() * 0.036f > MaxRecoverySpeedKmh) return false;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    if (!Decision) return IsRecoveryEligible(Vehicle);
    const FGTTBreakdownAssessment Assessment = Decision->AssessVehicle(Vehicle);
    return Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
        || Assessment.Recommendation == EGTTBreakdownRecommendation::Immobilized;
}

bool UGTTRoadsideRecoverySubsystem::IsRoadsideTowPending(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return false;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && Runtime->Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && Runtime->bTowRequested;
}

bool UGTTRoadsideRecoverySubsystem::IsRoadsidePatchPending(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return false;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && Runtime->Mode == EGTTRoadsideRecoveryMode::EmergencyPatch && Runtime->bPatchRequested;
}

bool UGTTRoadsideRecoverySubsystem::RequestRoadsideTow(AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!Vehicle || !IsPlayerRecoveryChoiceEligible(Vehicle)) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    if (!Driver || !Economy) return false;

    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    if (WantedLevel > 0)
    {
        Economy->PushMessage(WantedLevel == 1 ? TEXT("Roadside tow blocked while police are searching.") : TEXT("Police control recovery during an active pursuit."), 5.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_TOW_REQUEST_DENIED vehicle=%s reason=WANTED wanted=%d"), *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        return false;
    }

    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f || Runtime.bTowRequested || Runtime.bPatchRequested) return false;
    const int32 TowQuote = CalculateRoadsideCost(Vehicle);
    if (Economy->GetCash() < TowQuote)
    {
        Economy->PushMessage(FString::Printf(TEXT("Tow quote is $%d. You do not have enough cash."), TowQuote), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_ROADSIDE_RECOVERY_DENIED vehicle=%s cost=%d reason=INSUFFICIENT_CASH"), *Vehicle->GetPersistentVehicleId().ToString(), TowQuote);
        return false;
    }

    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    const int32 RepairQuote = Decision ? Decision->CalculateRepairEstimate(Vehicle) : 0;
    Runtime.Mode = EGTTRoadsideRecoveryMode::RoadsideAssistance;
    Runtime.StrandedSeconds = 0.0f;
    Runtime.bTowRequested = true;
    Runtime.bPatchRequested = false;
    Runtime.PendingPatchQuote = 0;
    Runtime.bAnnounced = true;
    Economy->PushMessage(FString::Printf(TEXT("Tow dispatched: $%d. Damage will be preserved; workshop estimate $%d remains separate."), TowQuote, RepairQuote), 6.0f);
    UE_LOG(LogGTT, Display, TEXT("NATIVE_ROADSIDE_TOW_REQUESTED vehicle=%s tow_quote=%d repair_quote=%d player_authorized=YES"), *Vehicle->GetPersistentVehicleId().ToString(), TowQuote, RepairQuote);
    return true;
}

bool UGTTRoadsideRecoverySubsystem::RequestEmergencyRoadsidePatch(AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!Vehicle || !IsPlayerRecoveryChoiceEligible(Vehicle)) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    if (!Driver || !Economy || !Decision || !Decision->CanEmergencyPatch(Vehicle)) return false;

    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    if (WantedLevel > 0)
    {
        Economy->PushMessage(WantedLevel == 1 ? TEXT("Roadside patch blocked while police are searching.") : TEXT("Police control recovery during an active pursuit."), 5.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_PATCH_REQUEST_DENIED vehicle=%s reason=WANTED wanted=%d"), *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        return false;
    }

    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f || Runtime.bTowRequested || Runtime.bPatchRequested) return false;

    const int32 PatchQuote = Decision->CalculateRoadsidePatchEstimate(Vehicle);
    if (PatchQuote <= 0 || Economy->GetCash() < PatchQuote)
    {
        Economy->PushMessage(FString::Printf(TEXT("Emergency patch costs $%d. You do not have enough cash."), PatchQuote), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_ROADSIDE_PATCH_DENIED vehicle=%s cost=%d reason=INSUFFICIENT_CASH"), *Vehicle->GetPersistentVehicleId().ToString(), PatchQuote);
        return false;
    }

    Runtime.Mode = EGTTRoadsideRecoveryMode::EmergencyPatch;
    Runtime.StrandedSeconds = 0.0f;
    Runtime.bPatchRequested = true;
    Runtime.bTowRequested = false;
    Runtime.PendingPatchQuote = PatchQuote;
    Runtime.bAnnounced = true;
    Economy->PushMessage(FString::Printf(TEXT("Emergency patch dispatched: $%d. Temporary limp-home service only; body damage and workshop repairs remain."), PatchQuote), 7.0f);
    UE_LOG(LogGTT, Display, TEXT("NATIVE_ROADSIDE_PATCH_REQUESTED vehicle=%s patch_quote=%d player_authorized=YES"), *Vehicle->GetPersistentVehicleId().ToString(), PatchQuote);
    return true;
}

void UGTTRoadsideRecoverySubsystem::UpdateVehicle(AGTTRoadVehicleNativePawn* Vehicle, float DeltaSeconds)
{
    if (!Vehicle) return;
    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    const bool bHardStranded = IsRecoveryEligible(Vehicle);
    const bool bPlayerChoiceEligible = IsPlayerRecoveryChoiceEligible(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f || (!bHardStranded && !bPlayerChoiceEligible))
    {
        Runtime.StrandedSeconds = 0.0f;
        Runtime.Mode = EGTTRoadsideRecoveryMode::None;
        Runtime.bAnnounced = false;
        Runtime.bTowRequested = false;
        Runtime.bPatchRequested = false;
        Runtime.PendingPatchQuote = 0;
        return;
    }

    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    if (!Driver || !Economy) return;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;

    if (WantedLevel == 1)
    {
        Runtime.StrandedSeconds = 0.0f;
        Runtime.Mode = EGTTRoadsideRecoveryMode::None;
        Runtime.bTowRequested = false;
        Runtime.bPatchRequested = false;
        Runtime.PendingPatchQuote = 0;
        if (!Runtime.bAnnounced)
        {
            Runtime.bAnnounced = true;
            Economy->PushMessage(TEXT("Roadside assistance unavailable while police are searching. Lose the heat or surrender."), 5.0f);
            UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_RECOVERY_BLOCKED vehicle=%s wanted=1"), *Vehicle->GetPersistentVehicleId().ToString());
        }
        return;
    }

    if (WantedLevel >= 2)
    {
        Runtime.bTowRequested = false;
        Runtime.bPatchRequested = false;
        Runtime.PendingPatchQuote = 0;

        // Police impound remains an automatic consequence only for a truly stranded vehicle.
        if (!bHardStranded)
        {
            Runtime.StrandedSeconds = 0.0f;
            Runtime.Mode = EGTTRoadsideRecoveryMode::None;
            if (!Runtime.bAnnounced)
            {
                Runtime.bAnnounced = true;
                Economy->PushMessage(TEXT("Roadside service blocked during active pursuit. Keep moving or surrender."), 5.0f);
            }
            return;
        }

        Runtime.Mode = EGTTRoadsideRecoveryMode::PoliceImpound;
        Runtime.StrandedSeconds += DeltaSeconds;
        if (!Runtime.bAnnounced)
        {
            Runtime.bAnnounced = true;
            Economy->PushMessage(TEXT("Vehicle disabled during an active pursuit. Police impound response inbound."), 5.0f);
            UE_LOG(LogGTT, Warning, TEXT("NATIVE_POLICE_IMPOUND_ARMED vehicle=%s wanted=%d"), *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        }
        if (Runtime.StrandedSeconds >= PoliceImpoundArmSeconds && CompleteRecovery(Vehicle, Runtime.Mode))
        {
            Runtime.StrandedSeconds = 0.0f;
            Runtime.CooldownSeconds = RecoveryCooldownSeconds;
            Runtime.Mode = EGTTRoadsideRecoveryMode::None;
            Runtime.bAnnounced = false;
        }
        return;
    }

    if (Runtime.Mode == EGTTRoadsideRecoveryMode::None && !Runtime.bTowRequested && !Runtime.bPatchRequested)
    {
        // A previous wanted-state block used the announcement flag; clear it once normal service is legal again.
        Runtime.bAnnounced = false;
    }

    if (Runtime.bPatchRequested && Runtime.Mode == EGTTRoadsideRecoveryMode::EmergencyPatch)
    {
        Runtime.StrandedSeconds += DeltaSeconds;
        if (Runtime.StrandedSeconds >= PlayerPatchDispatchSeconds)
        {
            const bool bCompleted = CompleteEmergencyPatch(Vehicle, Runtime.PendingPatchQuote);
            Runtime.StrandedSeconds = 0.0f;
            Runtime.bPatchRequested = false;
            Runtime.PendingPatchQuote = 0;
            Runtime.Mode = EGTTRoadsideRecoveryMode::None;
            Runtime.bAnnounced = false;
            if (bCompleted) Runtime.CooldownSeconds = RecoveryCooldownSeconds;
        }
        return;
    }

    if (Runtime.bTowRequested && Runtime.Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance)
    {
        Runtime.StrandedSeconds += DeltaSeconds;
        if (Runtime.StrandedSeconds >= PlayerTowDispatchSeconds)
        {
            const bool bCompleted = CompleteRecovery(Vehicle, Runtime.Mode);
            Runtime.StrandedSeconds = 0.0f;
            Runtime.bTowRequested = false;
            Runtime.Mode = EGTTRoadsideRecoveryMode::None;
            Runtime.bAnnounced = false;
            if (bCompleted) Runtime.CooldownSeconds = RecoveryCooldownSeconds;
        }
        return;
    }

    Runtime.Mode = EGTTRoadsideRecoveryMode::RoadsideAssistance;
    Runtime.StrandedSeconds = 0.0f;
    if (!Runtime.bAnnounced)
    {
        Runtime.bAnnounced = true;
        const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
        const FGTTBreakdownAssessment Assessment = Decision ? Decision->AssessVehicle(Vehicle) : FGTTBreakdownAssessment();
        if (Assessment.bEmergencyPatchPossible)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("Vehicle recovery choice: Y / D-Pad Left patch $%d for limp-home, or T / D-Pad Up tow $%d. Full workshop repair ~$%d."),
                Assessment.EmergencyPatchEstimate, Assessment.TowEstimate, Assessment.RepairEstimate), 10.0f);
        }
        else
        {
            Economy->PushMessage(FString::Printf(
                TEXT("Vehicle needs recovery. T / D-Pad Up tow $%d; workshop repair ~$%d. Structural/body damage is too severe for a roadside patch."),
                Assessment.TowEstimate, Assessment.RepairEstimate), 10.0f);
        }
        UE_LOG(LogGTT, Log,
            TEXT("NATIVE_ROADSIDE_RECOVERY_ARMED vehicle=%s tow_quote=%d patch_quote=%d patch_possible=%s repair_quote=%d severity=%.3f player_choice=REQUIRED"),
            *Vehicle->GetPersistentVehicleId().ToString(), Assessment.TowEstimate, Assessment.EmergencyPatchEstimate,
            Assessment.bEmergencyPatchPossible ? TEXT("YES") : TEXT("NO"), Assessment.RepairEstimate, Assessment.Severity);
    }
}

int32 UGTTRoadsideRecoverySubsystem::CalculateRoadsideCost(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    if (const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr) return Decision->CalculateTowEstimate(Vehicle);
    return 140;
}

int32 UGTTRoadsideRecoverySubsystem::CalculateImpoundCost(const AGTTRoadVehicleNativePawn* Vehicle, int32 WantedLevel) const
{
    if (!Vehicle) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const int32 SafetyService = FMath::RoundToInt((1.0f - State.ConditionPercent) * 190.0f + (1.0f - State.TireIntegrity) * 100.0f);
    return FMath::Clamp(360 + WantedLevel * 145 + SafetyService + Vehicle->GetBodyDamageRepairSurcharge(), 505, 1200);
}

FVector UGTTRoadsideRecoverySubsystem::GetWorkshopDropLocation(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    return WorkshopBaseLocation + FVector(0.0f, Vehicle && Vehicle->GetPersistentVehicleId() == FName(TEXT("Mulebox1200")) ? 250.0f : -250.0f, 0.0f);
}

bool UGTTRoadsideRecoverySubsystem::CompleteEmergencyPatch(AGTTRoadVehicleNativePawn* Vehicle, int32 PatchQuote)
{
    if (!Vehicle || !Vehicle->GetDriverPawn() || PatchQuote <= 0) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Driver);
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld() ? GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>() : nullptr;
    if (!Economy || !Decision || !Decision->CanEmergencyPatch(Vehicle)) return false;

    const FName ExpectedId = Vehicle->GetPersistentVehicleId();
    const FGTTRoadVehicleMigrationSnapshot Before = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyBefore = Vehicle->GetBodyDamageSnapshot();
    const int32 DetachedMaskBefore = Vehicle->GetDetachedPanelMask();

    if (!Economy->SpendCash(PatchQuote, TEXT("Emergency roadside limp-home patch"))) return false;
    const bool bApplied = Vehicle->ApplyNativeEmergencyRoadsidePatch();

    const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyAfter = Vehicle->GetBodyDamageSnapshot();
    const bool bIdentityPreserved = Vehicle->GetPersistentVehicleId() == ExpectedId;
    const bool bBodyPreserved =
        FMath::IsNearlyEqual(BodyBefore.FrontHealth, BodyAfter.FrontHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.RearHealth, BodyAfter.RearHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.LeftHealth, BodyAfter.LeftHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.RightHealth, BodyAfter.RightHealth, 0.001f)
        && FMath::IsNearlyEqual(BodyBefore.CoolingStress, BodyAfter.CoolingStress, 0.001f)
        && BodyBefore.DetachedPanelCount == BodyAfter.DetachedPanelCount
        && DetachedMaskBefore == Vehicle->GetDetachedPanelMask();
    const bool bLimpFloorsApplied =
        After.ConditionPercent + KINDA_SMALL_NUMBER >= FMath::Max(Before.ConditionPercent, 0.30f)
        && After.TireIntegrity + KINDA_SMALL_NUMBER >= FMath::Max(Before.TireIntegrity, 0.32f)
        && After.FuelLiters + KINDA_SMALL_NUMBER >= FMath::Max(Before.FuelLiters, FMath::Min(Vehicle->GetFuelCapacityLiters(), 5.0f));
    const bool bSuccess = bApplied && bIdentityPreserved && bBodyPreserved && bLimpFloorsApplied;

    if (bSuccess)
    {
        Economy->PushMessage(FString::Printf(TEXT("Emergency patch complete: $%d. Limp-home only — body damage remains and a full workshop repair is still recommended."), PatchQuote), 8.0f);
        UE_LOG(LogGTT, Display,
            TEXT("NATIVE_ROADSIDE_PATCH_COMPLETE vehicle=%s cost=%d result=PASS identity_preserved=YES body_preserved=YES workshop_repair_still_required=YES"),
            *ExpectedId.ToString(), PatchQuote);
    }
    else
    {
        Vehicle->RestorePersistentMigrationSnapshot(Before);
        Economy->AddCash(PatchQuote, TEXT("Emergency patch verification failed — charge refunded."));
        Economy->PushMessage(TEXT("Emergency patch verification failed. Vehicle state was rolled back; use workshop/tow recovery before continuing."), 8.0f);
        UE_LOG(LogGTT, Warning,
            TEXT("NATIVE_ROADSIDE_PATCH_COMPLETE vehicle=%s cost=%d result=FAIL identity_preserved=%s body_preserved=%s floors_applied=%s rollback=YES refund=YES"),
            *ExpectedId.ToString(), PatchQuote, bIdentityPreserved ? TEXT("YES") : TEXT("NO"),
            bBodyPreserved ? TEXT("YES") : TEXT("NO"), bLimpFloorsApplied ? TEXT("YES") : TEXT("NO"));
    }
    return bSuccess;
}

bool UGTTRoadsideRecoverySubsystem::CompleteRecovery(AGTTRoadVehicleNativePawn* Vehicle, EGTTRoadsideRecoveryMode Mode)
{
    if (!Vehicle || !Vehicle->GetDriverPawn()) return false;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Driver);
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Driver);
    if (!Economy) return false;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    const int32 Cost = Mode == EGTTRoadsideRecoveryMode::PoliceImpound ? CalculateImpoundCost(Vehicle, WantedLevel) : CalculateRoadsideCost(Vehicle);
    if (Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && !Economy->SpendCash(Cost, TEXT("Roadside tow to workshop")))
    {
        Economy->PushMessage(FString::Printf(TEXT("Roadside tow costs $%d. Earn or save enough cash first."), Cost), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_ROADSIDE_RECOVERY_DENIED vehicle=%s cost=%d reason=INSUFFICIENT_CASH"), *Vehicle->GetPersistentVehicleId().ToString(), Cost);
        return false;
    }
    if (Mode == EGTTRoadsideRecoveryMode::PoliceImpound)
    {
        Economy->ChargeFine(Cost, TEXT("Police impound + mandatory safety service"));
        if (Wanted) Wanted->ClearWanted();
    }

    const FGTTRoadVehicleMigrationSnapshot BeforeTow = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyBeforeTow = Vehicle->GetBodyDamageSnapshot();
    const FVector DropLocation = GetWorkshopDropLocation(Vehicle);
    Vehicle->ExitNativeVehicle();
    Vehicle->SetActorTransform(FTransform(FRotator(0.0f, 90.0f, 0.0f), DropLocation), false, nullptr, ETeleportType::ResetPhysics);
    Driver->SetActorLocation(DropLocation + FVector(-180.0f, 0.0f, 20.0f), false, nullptr, ETeleportType::TeleportPhysics);

    if (Mode == EGTTRoadsideRecoveryMode::PoliceImpound)
    {
        const bool bServiced = Vehicle->ApplyNativeWorkshopService();
        Economy->PushMessage(FString::Printf(TEXT("Vehicle impounded and safety-serviced: $%d."), Cost), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_POLICE_IMPOUND vehicle=%s wanted=%d cost=%d serviced=%s destination=WORKSHOP"), *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel, Cost, bServiced ? TEXT("YES") : TEXT("NO"));
        return bServiced;
    }

    const FGTTRoadVehicleMigrationSnapshot AfterTow = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyAfterTow = Vehicle->GetBodyDamageSnapshot();
    const bool bDamagePreserved = FMath::IsNearlyEqual(BeforeTow.ConditionPercent, AfterTow.ConditionPercent, 0.001f) && FMath::IsNearlyEqual(BeforeTow.TireIntegrity, AfterTow.TireIntegrity, 0.001f) && FMath::IsNearlyEqual(BodyBeforeTow.FrontHealth, BodyAfterTow.FrontHealth, 0.001f) && BodyBeforeTow.DetachedPanelCount == BodyAfterTow.DetachedPanelCount;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    const int32 RepairEstimate = Decision ? Decision->CalculateRepairEstimate(Vehicle) : 0;
    Economy->PushMessage(FString::Printf(TEXT("Tow complete: $%d. Damage preserved; workshop estimate $%d."), Cost, RepairEstimate), 7.0f);
    UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_TOW_COMPLETE vehicle=%s tow_cost=%d repair_estimate=%d damage_preserved=%s serviced=NO destination=WORKSHOP"), *Vehicle->GetPersistentVehicleId().ToString(), Cost, RepairEstimate, bDamagePreserved ? TEXT("YES") : TEXT("NO"));
    return bDamagePreserved;
}
