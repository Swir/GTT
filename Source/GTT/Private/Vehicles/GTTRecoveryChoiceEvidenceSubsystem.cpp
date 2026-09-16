#include "Vehicles/GTTRecoveryChoiceEvidenceSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadsideRecoverySubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTStructuralDriveConsequenceSubsystem.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTServiceTerminal.h"
#include "GTT.h"

namespace
{
    constexpr float EvidenceTimeoutSeconds = 215.0f;
    constexpr float NoAutoTowProofSeconds = 8.0f;
    constexpr float LocationToleranceCm = 250.0f;
    const FVector WorkshopBaseLocation(-400.0f, 2650.0f, 105.0f);
    float MinBodyHealth(const FGTTRoadBodyDamageSnapshot& Body)
    {
        return FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth));
    }
    const TCHAR* RecommendationName(EGTTBreakdownRecommendation Value)
    {
        switch (Value)
        {
            case EGTTBreakdownRecommendation::LimpToWorkshop: return TEXT("LIMP_TO_WORKSHOP");
            case EGTTBreakdownRecommendation::TowRecommended: return TEXT("TOW_RECOMMENDED");
            case EGTTBreakdownRecommendation::Immobilized: return TEXT("IMMOBILIZED");
            default: return TEXT("DRIVE_NORMALLY");
        }
    }
}

void UGTTRecoveryChoiceEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEvidenceEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEvidenceEnabled) UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_RECOVERY_CHOICE_BEGIN version=1 route=stranded-offer-manual-tow-paid-repair"));
}

AGTTRoadVehicleNativePawn* UGTTRecoveryChoiceEvidenceSubsystem::FindEvidenceVehicle() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->IsNativeReady() || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetDriverPawn()) continue;
        if (!Vehicle->GetMigrationSnapshot().bOwnedByPlayer) continue;
        const FName Id = Vehicle->GetPersistentVehicleId();
        if (Id == FName(TEXT("Rattleback82")) || Id == FName(TEXT("Mulebox1200"))) return Vehicle;
    }
    return nullptr;
}

AGTTServiceTerminal* UGTTRecoveryChoiceEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    const FVector Reference = EvidenceVehicle.IsValid() ? EvidenceVehicle->GetActorLocation() : WorkshopBaseLocation;
    AGTTServiceTerminal* Best = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (!IsValid(*It)) continue;
        const float DistanceSq = FVector::DistSquared2D(Reference, It->GetActorLocation());
        if (DistanceSq < BestDistanceSq) { BestDistanceSq = DistanceSq; Best = *It; }
    }
    return Best;
}

void UGTTRecoveryChoiceEvidenceSubsystem::Fail(const FString& Reason)
{
    UE_LOG(LogGTT, Error, TEXT("DEMO_SCENARIO_RECOVERY_CHOICE_COMPLETE result=FAIL phase=%d elapsed=%.2f reason=%s"), static_cast<int32>(Phase), Elapsed, *Reason);
    bFinished = true;
    Phase = EPhase::Complete;
}

void UGTTRecoveryChoiceEvidenceSubsystem::Tick(float DeltaTime)
{
    if (!bEvidenceEnabled || bFinished) return;
    Elapsed += DeltaTime;
    UWorld* World = GetWorld();
    if (!World) return;
    if (Elapsed > EvidenceTimeoutSeconds) { Fail(TEXT("recovery choice evidence timeout")); return; }

    UGTTBreakdownDecisionSubsystem* Decision = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    UGTTRoadsideRecoverySubsystem* Recovery = World->GetSubsystem<UGTTRoadsideRecoverySubsystem>();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;

    switch (Phase)
    {
        case EPhase::WaitForStructuralDrive:
        {
            const UGTTStructuralDriveConsequenceSubsystem* Structural = World->GetSubsystem<UGTTStructuralDriveConsequenceSubsystem>();
            if (!Structural || !Structural->IsDemoEvidenceComplete()) return;
            EvidenceVehicle = FindEvidenceVehicle();
            if (!EvidenceVehicle.IsValid()) return;
            EvidenceVehicleId = EvidenceVehicle->GetPersistentVehicleId();
            Phase = EPhase::StageStrandedVehicle;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::StageStrandedVehicle:
        {
            if (Elapsed - PhaseStarted < 0.5f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!Vehicle || !Decision || !Recovery || !Economy) { Fail(TEXT("recovery dependencies unavailable")); return; }

            // The preceding wanted-4 evidence route may still be cooling down. Reset it explicitly so this
            // phase proves voluntary civilian recovery rather than accidentally entering police impound.
            if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
            {
                if (Wanted->GetWantedLevel() > 0)
                {
                    Wanted->ClearWanted();
                    UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_ACTION action=RECOVERY_CHOICE_CLEAR_WANTED"));
                }
            }
            if (USkeletalMeshComponent* Mesh = Vehicle->GetMesh())
            {
                Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
                Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
            }
            if (!Vehicle->ApplyPoliceSpikeDamage(0.94f, 0.34f)) { Fail(TEXT("unable to stage immobilizing tire damage")); return; }
            const FGTTBreakdownAssessment Assessment = Decision->AssessVehicle(Vehicle);
            if (Assessment.Recommendation != EGTTBreakdownRecommendation::Immobilized || Assessment.TowEstimate <= 0 || Assessment.RepairEstimate <= 0)
            {
                Fail(TEXT("staged vehicle did not produce an immobilized recovery assessment")); return;
            }
            TowQuote = Assessment.TowEstimate;
            RepairQuote = Assessment.RepairEstimate;
            const int32 RequiredReserve = TowQuote + RepairQuote + 500;
            if (Economy->GetCash() < RequiredReserve)
            {
                const int32 Reserve = RequiredReserve - Economy->GetCash();
                Economy->AddCash(Reserve, TEXT("Demo recovery-choice reserve"));
                UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_ACTION action=RECOVERY_CHOICE_RESERVE amount=%d"), Reserve);
            }
            const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
            const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
            SavedCondition = State.ConditionPercent;
            SavedTires = State.TireIntegrity;
            SavedBodyMin = MinBodyHealth(Body);
            StrandedLocation = Vehicle->GetActorLocation();
            OfferCash = Economy->GetCash();
            UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_RECOVERY_OFFER vehicle=%s result=PASS recommendation=%s tow_quote=%d repair_quote=%d severity=%.3f"), *EvidenceVehicleId.ToString(), RecommendationName(Assessment.Recommendation), TowQuote, RepairQuote, Assessment.Severity);
            Phase = EPhase::VerifyOffer;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::VerifyOffer:
        {
            if (Elapsed - PhaseStarted < 1.0f) return;
            if (!EvidenceVehicle.IsValid() || !Economy || Recovery->IsRoadsideTowPending(EvidenceVehicle.Get())) { Fail(TEXT("recovery offer did not remain an uncommitted player choice")); return; }
            Phase = EPhase::VerifyNoAutoTow;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::VerifyNoAutoTow:
        {
            if (Elapsed - PhaseStarted < NoAutoTowProofSeconds) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!Vehicle || !Economy) { Fail(TEXT("vehicle/economy missing during no-auto-tow proof")); return; }
            const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
            const float Travel = FVector::Dist2D(StrandedLocation, Vehicle->GetActorLocation());
            if (Travel > LocationToleranceCm || Economy->GetCash() != OfferCash || !Vehicle->GetDriverPawn() || Recovery->IsRoadsideTowPending(Vehicle) ||
                !FMath::IsNearlyEqual(State.ConditionPercent, SavedCondition, 0.002f) || !FMath::IsNearlyEqual(State.TireIntegrity, SavedTires, 0.002f))
            {
                Fail(TEXT("roadside assistance auto-resolved without explicit player authorization")); return;
            }
            UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_RECOVERY_CHOICE vehicle=%s result=PASS waited=%.2f auto_tow=NO cash=%d tire=%.3f"), *EvidenceVehicleId.ToString(), NoAutoTowProofSeconds, Economy->GetCash(), State.TireIntegrity);
            Phase = EPhase::RequestTow;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::RequestTow:
        {
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!Vehicle || !Economy || !Recovery) { Fail(TEXT("tow request dependencies unavailable")); return; }
            CashBeforeTow = Economy->GetCash();
            if (!Recovery->RequestRoadsideTow(Vehicle)) { Fail(TEXT("explicit tow request was rejected")); return; }
            Phase = EPhase::VerifyTow;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::VerifyTow:
        {
            if (Elapsed - PhaseStarted < 4.0f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Vehicle || !Economy || !Decision) { Fail(TEXT("vehicle/economy missing after tow")); return; }
            const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
            const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
            const int32 TowPaid = CashBeforeTow - Economy->GetCash();
            const bool bDamagePreserved = FMath::IsNearlyEqual(State.ConditionPercent, SavedCondition, 0.002f) && FMath::IsNearlyEqual(State.TireIntegrity, SavedTires, 0.002f) && FMath::IsNearlyEqual(MinBodyHealth(Body), SavedBodyMin, 0.002f);
            if (TowPaid != TowQuote || !bDamagePreserved || Vehicle->GetDriverPawn() || FVector::Dist2D(Vehicle->GetActorLocation(), WorkshopBaseLocation) > 650.0f)
            {
                Fail(TEXT("player-authorized tow did not preserve damage or charge the quoted amount")); return;
            }
            RepairQuote = Decision->CalculateRepairEstimate(Vehicle);
            UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_PLAYER_TOW vehicle=%s result=PASS requested=YES tow_paid=%d cash_before=%d cash_after=%d damage_preserved=YES serviced=NO repair_quote=%d"), *EvidenceVehicleId.ToString(), TowPaid, CashBeforeTow, Economy->GetCash(), RepairQuote);
            Phase = EPhase::InvokeRepair;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::InvokeRepair:
        {
            if (Elapsed - PhaseStarted < 0.5f) return;
            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Terminal || !PlayerPawn || !Economy) { Fail(TEXT("workshop unavailable after tow")); return; }
            Terminal->SetServiceType(EGTTServiceType::Workshop);
            CashBeforeRepair = Economy->GetCash();
            Terminal->Interact_Implementation(PlayerPawn);
            Phase = EPhase::VerifyRepair;
            PhaseStarted = Elapsed;
            return;
        }
        case EPhase::VerifyRepair:
        {
            if (Elapsed - PhaseStarted < 1.0f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Vehicle || !Economy) { Fail(TEXT("vehicle/economy missing after workshop")); return; }
            const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
            const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
            const int32 RepairPaid = CashBeforeRepair - Economy->GetCash();
            const float BodyMinAfter = MinBodyHealth(Body);
            if (RepairPaid != RepairQuote || State.ConditionPercent < 0.999f || State.TireIntegrity < 0.999f || BodyMinAfter < 0.999f || Body.CoolingStress > 0.01f || Body.DetachedPanelCount != 0)
            {
                Fail(TEXT("separate paid workshop service did not fully restore the towed vehicle")); return;
            }
            UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_SEPARATE_REPAIR vehicle=%s result=PASS repair_paid=%d cash_before=%d cash_after=%d condition_after=%.3f tire_after=%.3f body_min_after=%.3f"), *EvidenceVehicleId.ToString(), RepairPaid, CashBeforeRepair, Economy->GetCash(), State.ConditionPercent, State.TireIntegrity, BodyMinAfter);
            UE_LOG(LogGTT, Display, TEXT("DEMO_SCENARIO_RECOVERY_CHOICE_COMPLETE result=PASS vehicle=%s route=stranded-offer-manual-tow-paid-repair"), *EvidenceVehicleId.ToString());
            bFinished = true;
            Phase = EPhase::Complete;
            return;
        }
        case EPhase::Complete:
        default:
            bFinished = true;
            return;
    }
}