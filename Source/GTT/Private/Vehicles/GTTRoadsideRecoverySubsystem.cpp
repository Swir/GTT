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

    // The Native road pawn owns vehicle input while possessed, so recovery input is read at the world layer.
    // This keeps the choice available even when the hidden on-foot driver is not receiving input.
    if (APlayerController* PlayerController = World->GetFirstPlayerController())
    {
        if (PlayerController->WasInputKeyJustPressed(EKeys::T) || PlayerController->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))
        {
            RequestRoadsideTow(Cast<AGTTRoadVehicleNativePawn>(PlayerController->GetPawn()));
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

bool UGTTRoadsideRecoverySubsystem::IsRoadsideTowPending(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return false;
    const FGTTRoadsideRecoveryRuntime* Runtime = RuntimeByVehicle.Find(TWeakObjectPtr<AGTTRoadVehicleNativePawn>(const_cast<AGTTRoadVehicleNativePawn*>(Vehicle)));
    return Runtime && Runtime->Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && Runtime->bTowRequested;
}

bool UGTTRoadsideRecoverySubsystem::RequestRoadsideTow(AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!Vehicle || !IsRecoveryEligible(Vehicle)) return false;
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
    if (Runtime.CooldownSeconds > 0.0f || Runtime.bTowRequested) return false;
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
    Runtime.bAnnounced = true;
    Economy->PushMessage(FString::Printf(TEXT("Tow dispatched: $%d. Damage will be preserved; workshop estimate $%d remains separate."), TowQuote, RepairQuote), 6.0f);
    UE_LOG(LogGTT, Display, TEXT("NATIVE_ROADSIDE_TOW_REQUESTED vehicle=%s tow_quote=%d repair_quote=%d player_authorized=YES"), *Vehicle->GetPersistentVehicleId().ToString(), TowQuote, RepairQuote);
    return true;
}

void UGTTRoadsideRecoverySubsystem::UpdateVehicle(AGTTRoadVehicleNativePawn* Vehicle, float DeltaSeconds)
{
    if (!Vehicle) return;
    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f || !IsRecoveryEligible(Vehicle))
    {
        Runtime.StrandedSeconds = 0.0f;
        Runtime.Mode = EGTTRoadsideRecoveryMode::None;
        Runtime.bAnnounced = false;
        Runtime.bTowRequested = false;
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
        Runtime.Mode = EGTTRoadsideRecoveryMode::PoliceImpound;
        Runtime.bTowRequested = false;
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

    Runtime.Mode = EGTTRoadsideRecoveryMode::RoadsideAssistance;
    if (!Runtime.bAnnounced)
    {
        Runtime.bAnnounced = true;
        const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
        const FGTTBreakdownAssessment Assessment = Decision ? Decision->AssessVehicle(Vehicle) : FGTTBreakdownAssessment();
        Economy->PushMessage(FString::Printf(TEXT("Vehicle stranded. Tow $%d; workshop estimate $%d is separate. Press T / D-Pad Up to call tow, or try to limp home."), Assessment.TowEstimate, Assessment.RepairEstimate), 8.0f);
        UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_RECOVERY_ARMED vehicle=%s tow_quote=%d repair_quote=%d severity=%.3f player_choice=REQUIRED"), *Vehicle->GetPersistentVehicleId().ToString(), Assessment.TowEstimate, Assessment.RepairEstimate, Assessment.Severity);
    }

    // Wanted 0 never auto-tows. The player may stay put, attempt a limp-home drive, or explicitly authorize a tow.
    if (!Runtime.bTowRequested)
    {
        Runtime.StrandedSeconds = 0.0f;
        return;
    }

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