#include "Vehicles/GTTRoadsideRecoverySubsystem.h"

#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    constexpr float RecoveryScanIntervalSeconds = 0.5f;
    constexpr float RecoveryArmSeconds = 7.0f;
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
    ScanAccumulator += DeltaSeconds;
    if (ScanAccumulator < RecoveryScanIntervalSeconds) return;
    const float Step = ScanAccumulator; ScanAccumulator = 0.0f;
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

void UGTTRoadsideRecoverySubsystem::UpdateVehicle(AGTTRoadVehicleNativePawn* Vehicle, float DeltaSeconds)
{
    if (!Vehicle) return;
    FGTTRoadsideRecoveryRuntime& Runtime = RuntimeByVehicle.FindOrAdd(Vehicle);
    if (Runtime.CooldownSeconds > 0.0f || !IsRecoveryEligible(Vehicle))
    {
        Runtime.StrandedSeconds = 0.0f; Runtime.Mode = EGTTRoadsideRecoveryMode::None; Runtime.bAnnounced = false; return;
    }
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTWantedComponent* Wanted = Driver ? UGTTGameplayStatics::FindWantedComponentForPawn(Driver) : nullptr;
    UGTTPlayerEconomyComponent* Economy = Driver ? UGTTGameplayStatics::FindEconomyComponentForPawn(Driver) : nullptr;
    if (!Driver || !Economy) return;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    if (WantedLevel == 1)
    {
        Runtime.StrandedSeconds = 0.0f; Runtime.Mode = EGTTRoadsideRecoveryMode::None;
        if (!Runtime.bAnnounced)
        {
            Runtime.bAnnounced = true;
            Economy->PushMessage(TEXT("Roadside assistance unavailable while police are searching. Lose the heat or surrender."), 5.0f);
            UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_RECOVERY_BLOCKED vehicle=%s wanted=1"), *Vehicle->GetPersistentVehicleId().ToString());
        }
        return;
    }
    Runtime.Mode = WantedLevel >= 2 ? EGTTRoadsideRecoveryMode::PoliceImpound : EGTTRoadsideRecoveryMode::RoadsideAssistance;
    Runtime.StrandedSeconds += DeltaSeconds;
    if (!Runtime.bAnnounced)
    {
        Runtime.bAnnounced = true;
        if (Runtime.Mode == EGTTRoadsideRecoveryMode::PoliceImpound)
        {
            Economy->PushMessage(TEXT("Vehicle disabled during an active pursuit. Police impound response inbound."), 5.0f);
            UE_LOG(LogGTT, Warning, TEXT("NATIVE_POLICE_IMPOUND_ARMED vehicle=%s wanted=%d"), *Vehicle->GetPersistentVehicleId().ToString(), WantedLevel);
        }
        else
        {
            const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
            const FGTTBreakdownAssessment Assessment = Decision ? Decision->AssessVehicle(Vehicle) : FGTTBreakdownAssessment();
            Economy->PushMessage(FString::Printf(TEXT("Vehicle stranded. Tow $%d; workshop estimate $%d is separate."), Assessment.TowEstimate, Assessment.RepairEstimate), 6.0f);
            UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_RECOVERY_ARMED vehicle=%s tow_quote=%d repair_quote=%d severity=%.3f"), *Vehicle->GetPersistentVehicleId().ToString(), Assessment.TowEstimate, Assessment.RepairEstimate, Assessment.Severity);
        }
    }
    if (Runtime.StrandedSeconds >= RecoveryArmSeconds)
    {
        CompleteRecovery(Vehicle, Runtime.Mode);
        Runtime.StrandedSeconds = 0.0f; Runtime.CooldownSeconds = RecoveryCooldownSeconds; Runtime.Mode = EGTTRoadsideRecoveryMode::None; Runtime.bAnnounced = false;
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

void UGTTRoadsideRecoverySubsystem::CompleteRecovery(AGTTRoadVehicleNativePawn* Vehicle, EGTTRoadsideRecoveryMode Mode)
{
    if (!Vehicle || !Vehicle->GetDriverPawn()) return;
    APawn* Driver = Vehicle->GetDriverPawn();
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Driver);
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Driver);
    if (!Economy) return;
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    const int32 Cost = Mode == EGTTRoadsideRecoveryMode::PoliceImpound ? CalculateImpoundCost(Vehicle, WantedLevel) : CalculateRoadsideCost(Vehicle);
    if (Mode == EGTTRoadsideRecoveryMode::RoadsideAssistance && !Economy->SpendCash(Cost, TEXT("Roadside tow to workshop")))
    {
        Economy->PushMessage(FString::Printf(TEXT("Roadside tow costs $%d. Earn or save enough cash first."), Cost), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_ROADSIDE_RECOVERY_DENIED vehicle=%s cost=%d reason=INSUFFICIENT_CASH"), *Vehicle->GetPersistentVehicleId().ToString(), Cost);
        return;
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
        return;
    }

    const FGTTRoadVehicleMigrationSnapshot AfterTow = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyAfterTow = Vehicle->GetBodyDamageSnapshot();
    const bool bDamagePreserved = FMath::IsNearlyEqual(BeforeTow.ConditionPercent, AfterTow.ConditionPercent, 0.001f) && FMath::IsNearlyEqual(BeforeTow.TireIntegrity, AfterTow.TireIntegrity, 0.001f) && FMath::IsNearlyEqual(BodyBeforeTow.FrontHealth, BodyAfterTow.FrontHealth, 0.001f) && BodyBeforeTow.DetachedPanelCount == BodyAfterTow.DetachedPanelCount;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    const int32 RepairEstimate = Decision ? Decision->CalculateRepairEstimate(Vehicle) : 0;
    Economy->PushMessage(FString::Printf(TEXT("Tow complete: $%d. Damage preserved; workshop estimate $%d."), Cost, RepairEstimate), 7.0f);
    UE_LOG(LogGTT, Log, TEXT("NATIVE_ROADSIDE_TOW_COMPLETE vehicle=%s tow_cost=%d repair_estimate=%d damage_preserved=%s serviced=NO destination=WORKSHOP"), *Vehicle->GetPersistentVehicleId().ToString(), Cost, RepairEstimate, bDamagePreserved ? TEXT("YES") : TEXT("NO"));
}
