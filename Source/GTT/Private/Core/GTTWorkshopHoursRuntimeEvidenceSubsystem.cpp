#include "Core/GTTWorkshopHoursRuntimeEvidenceSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Vehicles/GTTRoadsideRecoverySubsystem.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTServiceTerminal.h"
#include "World/GTTWorkshopHoursPolicy.h"
#include "GTT.h"

namespace
{
constexpr float StartDelaySeconds = 384.0f;
constexpr float GlobalDeadlineSeconds = 404.0f;
constexpr float TowCompletionTimeoutSeconds = 6.0f;
constexpr float WorkshopStageOffsetCm = 140.0f;
constexpr int32 MinimumEvidenceCash = 6000;
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoWorkshopRecoveryScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopHoursRuntimeScenario"));
    if (bEnabled)
    {
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME_BEGIN version=1 route=clock-closed-reject-tow-hold-emergency-service start_delay=%.1f deadline=%.1f opening=06:30 closing=20:00 emergency_surcharge_percent=%d exact_vehicle=required"),
            StartDelaySeconds, GlobalDeadlineSeconds, GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent);
    }
}

TStatId UGTTWorkshopHoursRuntimeEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopHoursRuntimeEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTWorkshopHoursRuntimeEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTWorkshopHoursRuntimeEvidenceSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return nullptr;
    APawn* Controlled = PC->GetPawn();
    if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Controlled))
    {
        return Native->GetDriverPawn();
    }
    return Controlled;
}

AGTTServiceTerminal* UGTTWorkshopHoursRuntimeEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It) && It->GetServiceType() == EGTTServiceType::Workshop) return *It;
    }
    return nullptr;
}

AGTTRoadVehicleNativePawn* UGTTWorkshopHoursRuntimeEvidenceSubsystem::FindEvidenceVehicle() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    AGTTRoadVehicleNativePawn* Fallback = nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetPersistentVehicleId().IsNone()) continue;
        if (Candidate->IsLegacyTakeoverActive()) return Candidate;
        if (!Fallback) Fallback = Candidate;
    }
    return Fallback;
}

bool UGTTWorkshopHoursRuntimeEvidenceSubsystem::ResolveActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (!PlayerPawn.IsValid()) PlayerPawn = ResolvePlayerPawn();
    if (!Workshop.IsValid()) Workshop = FindWorkshopTerminal();
    if (!Vehicle.IsValid()) Vehicle = FindEvidenceVehicle();
    if (!GarageFleet.IsValid()) GarageFleet = World->GetSubsystem<UGTTGarageFleetSubsystem>();
    if (!Roadside.IsValid()) Roadside = World->GetSubsystem<UGTTRoadsideRecoverySubsystem>();
    if (!Economy.IsValid() && PlayerPawn.IsValid()) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn.Get());
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    return PlayerPawn.IsValid() && Workshop.IsValid() && Vehicle.IsValid() && GarageFleet.IsValid()
        && Roadside.IsValid() && Economy.IsValid() && DayNight.IsValid();
}

bool UGTTWorkshopHoursRuntimeEvidenceSubsystem::EnsureNativeDriver()
{
    UWorld* World = GetWorld();
    if (!World || !Vehicle.IsValid() || !PlayerPawn.IsValid()) return false;
    if (UGameplayStatics::GetPlayerPawn(World, 0) == Vehicle.Get()) return true;
    if (!Vehicle->IsLegacyTakeoverActive() && !Vehicle->TryActivateLegacyTakeover()) return false;
    Vehicle->Interact_Implementation(PlayerPawn.Get());
    return UGameplayStatics::GetPlayerPawn(World, 0) == Vehicle.Get()
        && Vehicle->GetDriverPawn() == PlayerPawn.Get();
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::StageVehicle(const FVector& Location) const
{
    if (!Vehicle.IsValid()) return;
    Vehicle->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()))
    {
        if (RootPrimitive->IsSimulatingPhysics())
        {
            RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
            RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
    }
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::StageOrdinaryDamage()
{
    if (!Vehicle.IsValid()) return;
    FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    State.ConditionPercent = 0.64f;
    State.TireIntegrity = 0.72f;
    State.FuelLiters = FMath::Min(State.FuelLiters, Vehicle->GetFuelCapacityLiters() * 0.50f);
    Vehicle->RestorePersistentMigrationSnapshot(State);
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::StageTowDamage()
{
    if (!Vehicle.IsValid()) return;
    FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    State.ConditionPercent = 0.19f;
    State.TireIntegrity = 0.20f;
    State.FuelLiters = FMath::Max(State.FuelLiters, FMath::Min(Vehicle->GetFuelCapacityLiters(), 6.0f));
    Vehicle->RestorePersistentMigrationSnapshot(State);
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    GTT_LOG( Error, TEXT("WORKSHOP_HOURS_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
    if (Roadside.IsValid() && Vehicle.IsValid() && Roadside->HasPendingRoadsideService(Vehicle.Get()))
    {
        Roadside->CancelPendingRoadsideService(Vehicle.Get());
    }
    if (Vehicle.IsValid())
    {
        if (UGameplayStatics::GetPlayerPawn(World, 0) == Vehicle.Get()) Vehicle->ExitNativeVehicle();
        Vehicle->RestorePersistentMigrationSnapshot(BaselineMigration);
        Vehicle->RestorePersistentBodyDamage(BaselineBodyDamage, BaselineDetachedMask);
        Vehicle->SetActorTransform(BaselineTransform, false, nullptr, ETeleportType::ResetPhysics);
    }
    if (Economy.IsValid()) Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    if (DayNight.IsValid()) DayNight->RestoreTime(BaselineDay, BaselineHour);
    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) GameMode->SaveProgress();
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bPass = bSequenceHealthy && bBoundariesVerified && bClosedServiceRejected && bClosedNoCharge
        && bClosedNoMutation && bTowRequested && bTowCompleted && bTowSingleCharge && bHoldDetected
        && bEmergencyQuoteVerified && bEmergencySingleCharge && bEmergencyServiceApplied && bHoldCleared
        && bIdentityPreserved && bMechanicalRepaired && bRefuelled && TowLockedQuote > 0
        && BaseWorkshopQuote > 0 && EmergencyWorkshopQuote > BaseWorkshopQuote && !VehicleId.IsNone();

    GTT_LOG( Log,
        TEXT("WORKSHOP_HOURS_RUNTIME_COMPLETE result=%s boundaries=%d closed_rejected=%d closed_no_charge=%d closed_no_mutation=%d tow_requested=%d tow_completed=%d tow_single_charge=%d hold_detected=%d emergency_quote=%d emergency_single_charge=%d emergency_service=%d hold_cleared=%d identity_preserved=%d repaired=%d refuelled=%d tow_quote=%d base_quote=%d emergency_quote_total=%d surcharge_percent=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bBoundariesVerified ? 1 : 0, bClosedServiceRejected ? 1 : 0,
        bClosedNoCharge ? 1 : 0, bClosedNoMutation ? 1 : 0, bTowRequested ? 1 : 0, bTowCompleted ? 1 : 0,
        bTowSingleCharge ? 1 : 0, bHoldDetected ? 1 : 0, bEmergencyQuoteVerified ? 1 : 0,
        bEmergencySingleCharge ? 1 : 0, bEmergencyServiceApplied ? 1 : 0, bHoldCleared ? 1 : 0,
        bIdentityPreserved ? 1 : 0, bMechanicalRepaired ? 1 : 0, bRefuelled ? 1 : 0,
        TowLockedQuote, BaseWorkshopQuote, EmergencyWorkshopQuote,
        GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent, *VehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EPhase::Complete;
    bFinished = true;
}

void UGTTWorkshopHoursRuntimeEvidenceSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < StartDelaySeconds) return;
    if (Elapsed >= GlobalDeadlineSeconds)
    {
        MarkFailure(TEXT("global-sequence-timeout"));
        FinishScenario(TEXT("deadline"));
        return;
    }
    if (!ResolveActors())
    {
        if (Elapsed > StartDelaySeconds + 4.0f)
        {
            MarkFailure(TEXT("workshop-hours-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    switch (Phase)
    {
    case EPhase::Waiting:
        Phase = EPhase::Prepare;
        break;

    case EPhase::Prepare:
    {
        if (GarageFleet->IsVehicleOnWorkshopHold(Vehicle->GetPersistentVehicleId()))
        {
            MarkFailure(TEXT("evidence-vehicle-started-on-workshop-hold")); FinishScenario(TEXT("prepare-failed")); return;
        }
        VehicleId = Vehicle->GetPersistentVehicleId();
        BaselineDay = DayNight->GetDayNumber();
        BaselineHour = DayNight->GetTimeOfDayHours();
        BaselineCash = Economy->GetCash();
        BaselineFishCount = Economy->GetFishCount();
        BaselineFishWeightKg = Economy->GetFishWeightKg();
        BaselineMigration = Vehicle->GetMigrationSnapshot();
        BaselineBodyDamage = Vehicle->GetBodyDamageSnapshot();
        BaselineDetachedMask = Vehicle->GetDetachedPanelMask();
        BaselineTransform = Vehicle->GetActorTransform();
        bBaselineCaptured = true;

        if (Roadside->HasPendingRoadsideService(Vehicle.Get())) Roadside->CancelPendingRoadsideService(Vehicle.Get());
        Economy->RestoreState(FMath::Max(BaselineCash, MinimumEvidenceCash), BaselineFishCount, BaselineFishWeightKg);
        StageVehicle(Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver())
        {
            MarkFailure(TEXT("native-driver-entry-failed")); FinishScenario(TEXT("prepare-failed")); return;
        }
        GTT_LOG( Log, TEXT("WORKSHOP_HOURS_RUNTIME phase=PREPARE result=PASS vehicle=%s cash_seeded=%d"),
            *VehicleId.ToString(), Economy->GetCash());
        Phase = EPhase::Boundaries;
        break;
    }

    case EPhase::Boundaries:
    {
        DayNight->RestoreTime(BaselineDay, 6.49f);
        const bool bBeforeOpenClosed = !Workshop->IsWorkshopOpenNow();
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::OpeningHour);
        const bool bOpeningOpen = Workshop->IsWorkshopOpenNow();
        DayNight->RestoreTime(BaselineDay, 19.983f);
        const bool bLastMinuteOpen = Workshop->IsWorkshopOpenNow();
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::ClosingHour);
        const bool bClosingClosed = !Workshop->IsWorkshopOpenNow();
        bBoundariesVerified = bBeforeOpenClosed && bOpeningOpen && bLastMinuteOpen && bClosingClosed;
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME phase=BOUNDARIES result=%s pre_open_closed=%d opening_open=%d last_minute_open=%d closing_closed=%d opening=%.2f closing=%.2f"),
            bBoundariesVerified ? TEXT("PASS") : TEXT("FAIL"), bBeforeOpenClosed ? 1 : 0, bOpeningOpen ? 1 : 0,
            bLastMinuteOpen ? 1 : 0, bClosingClosed ? 1 : 0,
            GTTWorkshopHoursPolicy::OpeningHour, GTTWorkshopHoursPolicy::ClosingHour);
        if (!bBoundariesVerified) { MarkFailure(TEXT("clock-boundary-contract-failed")); FinishScenario(TEXT("boundaries-failed")); return; }
        Phase = EPhase::ClosedOrdinaryService;
        break;
    }

    case EPhase::ClosedOrdinaryService:
    {
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::ClosingHour);
        StageOrdinaryDamage();
        const FGTTRoadVehicleMigrationSnapshot Before = Vehicle->GetMigrationSnapshot();
        const int32 CashBefore = Economy->GetCash();
        const bool bHoldBefore = GarageFleet->IsVehicleOnWorkshopHold(VehicleId);
        Workshop->Interact_Implementation(PlayerPawn.Get());
        const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
        bClosedNoCharge = Economy->GetCash() == CashBefore;
        bClosedNoMutation = FMath::IsNearlyEqual(Before.ConditionPercent, After.ConditionPercent, 0.0001f)
            && FMath::IsNearlyEqual(Before.TireIntegrity, After.TireIntegrity, 0.0001f)
            && FMath::IsNearlyEqual(Before.FuelLiters, After.FuelLiters, 0.01f);
        bClosedServiceRejected = !bHoldBefore && !Workshop->IsWorkshopOpenNow() && bClosedNoCharge && bClosedNoMutation;
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME phase=CLOSED_ORDINARY result=%s closed=1 hold_before=%d rejected=%d no_charge=%d no_mutation=%d condition=%.3f tire=%.3f fuel=%.2f"),
            bClosedServiceRejected ? TEXT("PASS") : TEXT("FAIL"), bHoldBefore ? 1 : 0,
            bClosedServiceRejected ? 1 : 0, bClosedNoCharge ? 1 : 0, bClosedNoMutation ? 1 : 0,
            After.ConditionPercent, After.TireIntegrity, After.FuelLiters);
        if (!bClosedServiceRejected) { MarkFailure(TEXT("ordinary-service-did-not-fail-closed")); FinishScenario(TEXT("closed-service-failed")); return; }
        Phase = EPhase::RequestTow;
        break;
    }

    case EPhase::RequestTow:
    {
        StageTowDamage();
        CashBeforeTow = Economy->GetCash();
        bTowRequested = Roadside->RequestRoadsideTow(Vehicle.Get());
        TowLockedQuote = Roadside->GetPendingRecoveryQuote(Vehicle.Get());
        const bool bPinned = bTowRequested && TowLockedQuote > 0
            && Roadside->GetPendingRecoveryVehicleId(Vehicle.Get()) == VehicleId
            && Economy->GetCash() == CashBeforeTow;
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME phase=TOW_REQUEST result=%s requested=%d locked_quote=%d target_pinned=%d no_precharge=%d vehicle=%s"),
            bPinned ? TEXT("PASS") : TEXT("FAIL"), bTowRequested ? 1 : 0, TowLockedQuote,
            Roadside->GetPendingRecoveryVehicleId(Vehicle.Get()) == VehicleId ? 1 : 0,
            Economy->GetCash() == CashBeforeTow ? 1 : 0, *VehicleId.ToString());
        if (!bPinned) { MarkFailure(TEXT("tow-request-contract-failed")); FinishScenario(TEXT("tow-request-failed")); return; }
        PhaseStartedAt = Elapsed;
        Phase = EPhase::AwaitTow;
        break;
    }

    case EPhase::AwaitTow:
    {
        if (Roadside->IsRoadsideTowPending(Vehicle.Get()))
        {
            if (Elapsed - PhaseStartedAt > TowCompletionTimeoutSeconds)
            {
                MarkFailure(TEXT("tow-did-not-complete")); FinishScenario(TEXT("tow-timeout"));
            }
            break;
        }
        const int32 Charged = CashBeforeTow - Economy->GetCash();
        bTowCompleted = Charged > 0;
        bTowSingleCharge = Charged == TowLockedQuote;
        bHoldDetected = GarageFleet->IsVehicleOnWorkshopHold(VehicleId);
        bIdentityPreserved = Vehicle->GetPersistentVehicleId() == VehicleId;
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME phase=TOW_COMPLETE result=%s completed=%d single_charge=%d charged=%d locked_quote=%d hold_detected=%d identity_preserved=%d vehicle=%s"),
            (bTowCompleted && bTowSingleCharge && bHoldDetected && bIdentityPreserved) ? TEXT("PASS") : TEXT("FAIL"),
            bTowCompleted ? 1 : 0, bTowSingleCharge ? 1 : 0, Charged, TowLockedQuote,
            bHoldDetected ? 1 : 0, bIdentityPreserved ? 1 : 0, *VehicleId.ToString());
        if (!bTowCompleted || !bTowSingleCharge || !bHoldDetected || !bIdentityPreserved)
        {
            MarkFailure(TEXT("tow-hold-contract-failed")); FinishScenario(TEXT("tow-complete-failed")); return;
        }
        Phase = EPhase::VerifyEmergencyQuote;
        break;
    }

    case EPhase::VerifyEmergencyQuote:
    {
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::ClosingHour);
        BaseWorkshopQuote = Workshop->GetNativeRoadRepairQuote(Vehicle.Get());
        EmergencyWorkshopQuote = Workshop->GetNativeRoadCheckoutQuote(Vehicle.Get());
        const int32 ExpectedEmergency = GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(BaseWorkshopQuote);
        bEmergencyQuoteVerified = !Workshop->IsWorkshopOpenNow() && GarageFleet->IsVehicleOnWorkshopHold(VehicleId)
            && BaseWorkshopQuote > 0 && EmergencyWorkshopQuote == ExpectedEmergency
            && EmergencyWorkshopQuote > BaseWorkshopQuote;
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME phase=EMERGENCY_QUOTE result=%s closed=1 hold=1 base_quote=%d checkout_quote=%d expected_quote=%d surcharge_percent=%d exact=%d"),
            bEmergencyQuoteVerified ? TEXT("PASS") : TEXT("FAIL"), BaseWorkshopQuote, EmergencyWorkshopQuote,
            ExpectedEmergency, GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent,
            EmergencyWorkshopQuote == ExpectedEmergency ? 1 : 0);
        if (!bEmergencyQuoteVerified) { MarkFailure(TEXT("emergency-quote-contract-failed")); FinishScenario(TEXT("quote-failed")); return; }
        Phase = EPhase::ApplyEmergencyService;
        break;
    }

    case EPhase::ApplyEmergencyService:
    {
        StageVehicle(Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        CashBeforeEmergency = Economy->GetCash();
        Workshop->Interact_Implementation(PlayerPawn.Get());
        const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
        const int32 Charged = CashBeforeEmergency - Economy->GetCash();
        bEmergencySingleCharge = Charged == EmergencyWorkshopQuote;
        bEmergencyServiceApplied = Charged > 0;
        bHoldCleared = !GarageFleet->IsVehicleOnWorkshopHold(VehicleId);
        bIdentityPreserved = bIdentityPreserved && Vehicle->GetPersistentVehicleId() == VehicleId;
        bMechanicalRepaired = After.ConditionPercent >= 0.999f && After.TireIntegrity >= 0.999f;
        bRefuelled = After.FuelLiters + 0.05f >= Vehicle->GetFuelCapacityLiters();
        GTT_LOG( Log,
            TEXT("WORKSHOP_HOURS_RUNTIME phase=EMERGENCY_SERVICE result=%s charged=%d checkout_quote=%d single_charge=%d hold_cleared=%d identity_preserved=%d repaired=%d refuelled=%d closed=%d vehicle=%s"),
            (bEmergencySingleCharge && bEmergencyServiceApplied && bHoldCleared && bIdentityPreserved && bMechanicalRepaired && bRefuelled) ? TEXT("PASS") : TEXT("FAIL"),
            Charged, EmergencyWorkshopQuote, bEmergencySingleCharge ? 1 : 0, bHoldCleared ? 1 : 0,
            bIdentityPreserved ? 1 : 0, bMechanicalRepaired ? 1 : 0, bRefuelled ? 1 : 0,
            !Workshop->IsWorkshopOpenNow() ? 1 : 0, *VehicleId.ToString());
        if (!bEmergencySingleCharge || !bEmergencyServiceApplied || !bHoldCleared || !bIdentityPreserved || !bMechanicalRepaired || !bRefuelled)
        {
            MarkFailure(TEXT("after-hours-emergency-service-failed")); FinishScenario(TEXT("service-failed")); return;
        }
        FinishScenario(TEXT("complete"));
        break;
    }

    case EPhase::Complete:
        break;
    }
}
