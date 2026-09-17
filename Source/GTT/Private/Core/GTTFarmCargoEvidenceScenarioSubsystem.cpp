#include "Core/GTTFarmCargoEvidenceScenarioSubsystem.h"

#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "Activities/GTTFarmJobDirector.h"
#include "Activities/GTTFarmJobTerminal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTLogisticsReputationSubsystem.h"

void UGTTFarmCargoEvidenceScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoScenario"));
    if (bEnabled)
    {
        UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_SCENARIO_BEGIN version=1 mode=exact-loaded-vehicle-authority"));
    }
}

void UGTTFarmCargoEvidenceScenarioSubsystem::Pass(const TCHAR* StepName)
{
    const FName Key(StepName);
    if (Passed.Contains(Key)) return;
    Passed.Add(Key);
    UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_SCENARIO_STEP step=%s result=PASS elapsed=%.2f"), StepName, Elapsed);
}

void UGTTFarmCargoEvidenceScenarioSubsystem::Fail(const FString& Reason)
{
    if (bFinished) return;
    UE_LOG(LogTemp, Error, TEXT("FARM_CARGO_SCENARIO_COMPLETE result=FAIL step=%d elapsed=%.2f reason=%s"), Step, Elapsed, *Reason);
    bFinished = true;
}

bool UGTTFarmCargoEvidenceScenarioSubsystem::ResolveActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return false;
    if (!OriginalPlayerPawn.IsValid()) OriginalPlayerPawn = PC->GetPawn();
    if (!OriginalPlayerPawn.IsValid()) return false;

    if (!FarmDirector.IsValid())
    {
        for (TActorIterator<AGTTFarmJobDirector> It(World); It; ++It)
        {
            FarmDirector = *It;
            break;
        }
    }
    if (!FarmDirector.IsValid()) return false;

    bHasPickup = bHasHillFarm = bHasFinalStop = false;
    for (TActorIterator<AGTTFarmJobTerminal> It(World); It; ++It)
    {
        switch (It->GetTerminalType())
        {
            case EGTTFarmJobTerminalType::Pickup:
                PickupTerminal = *It;
                PickupLocation = It->GetActorLocation();
                bHasPickup = true;
                break;
            case EGTTFarmJobTerminalType::Finish:
                HillFarmTerminal = *It;
                HillFarmLocation = It->GetActorLocation();
                bHasHillFarm = true;
                break;
            case EGTTFarmJobTerminalType::FinalFinish:
                FinalStopTerminal = *It;
                FinalStopLocation = It->GetActorLocation();
                bHasFinalStop = true;
                break;
            default:
                break;
        }
    }

    if (!CargoVehicle.IsValid())
    {
        for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
        {
            if (It->IsNativeReady() && It->IsLegacyTakeoverActive())
            {
                CargoVehicle = *It;
                break;
            }
        }
    }

    return CargoVehicle.IsValid() && bHasPickup && bHasHillFarm;
}

void UGTTFarmCargoEvidenceScenarioSubsystem::StageVehicleAt(const FVector& Location)
{
    AGTTMuleboxNativePawn* Vehicle = CargoVehicle.Get();
    if (!Vehicle) return;

    if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent()))
    {
        Movement->SetThrottleInput(0.0f);
        Movement->SetSteeringInput(0.0f);
        Movement->SetBrakeInput(1.0f);
    }
    Vehicle->SetActorLocation(Location + FVector(140.0f, 0.0f, 95.0f), false, nullptr, ETeleportType::TeleportPhysics);
    if (USkeletalMeshComponent* Mesh = Vehicle->GetMesh())
    {
        Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
}

void UGTTFarmCargoEvidenceScenarioSubsystem::CompleteScenario()
{
    UWorld* World = GetWorld();
    APawn* PlayerPawn = OriginalPlayerPawn.Get();
    if (!World || !PlayerPawn)
    {
        Fail(TEXT("player/world disappeared before completion"));
        return;
    }

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    UGTTLogisticsReputationSubsystem* Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Economy || !Logistics)
    {
        Fail(TEXT("economy/logistics authority missing at completion"));
        return;
    }
    if (Economy->GetCash() <= CashBefore)
    {
        Fail(FString::Printf(TEXT("contract completed without positive payout cash_before=%d cash_after=%d"), CashBefore, Economy->GetCash()));
        return;
    }
    if (Logistics->GetCargoCompletedRuns() <= CargoRunsBefore)
    {
        Fail(TEXT("cargo completion counter did not advance"));
        return;
    }
    Pass(TEXT("PAYOUT_REPUTATION"));

    if (AGTTGameMode* GM = World->GetAuthGameMode<AGTTGameMode>())
    {
        if (!GM->SaveProgress())
        {
            Fail(TEXT("SaveProgress failed after cargo completion"));
            return;
        }
        Pass(TEXT("SAVE"));
    }

    if (APlayerController* PC = World->GetFirstPlayerController()) PC->Possess(PlayerPawn);
    if (UGTTFarmCargoAuthoritySubsystem* Authority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>())
    {
        Authority->ClearLoadedVehicle(TEXT("scenario-complete"));
    }

    Pass(TEXT("COMPLETE"));
    UE_LOG(LogTemp, Display, TEXT("FARM_CARGO_SCENARIO_COMPLETE result=PASS steps=%d elapsed=%.2f cash_delta=%d cargo_runs_delta=%d"),
        Passed.Num(), Elapsed, Economy->GetCash() - CashBefore, Logistics->GetCargoCompletedRuns() - CargoRunsBefore);
    bFinished = true;
}

void UGTTFarmCargoEvidenceScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed > 55.0f)
    {
        Fail(TEXT("scenario timeout"));
        return;
    }
    if (!ResolveActors()) return;

    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    APawn* PlayerPawn = OriginalPlayerPawn.Get();
    AGTTFarmJobDirector* Director = FarmDirector.Get();
    AGTTMuleboxNativePawn* Vehicle = CargoVehicle.Get();
    UGTTFarmCargoAuthoritySubsystem* Authority = World ? World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>() : nullptr;
    if (!World || !PC || !PlayerPawn || !Director || !Vehicle || !Authority) return;

    if (!bEnvironmentPrepared)
    {
        if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn)) Wanted->ClearWanted();
        if (AGTTGameMode* GM = World->GetAuthGameMode<AGTTGameMode>())
        {
            if (AGTTDayNightCycle* DayNight = GM->GetDayNightCycle()) DayNight->RestoreTime(1, 8.0f);
        }
        UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
        UGTTLogisticsReputationSubsystem* Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
        if (!Economy || !Logistics) return;
        CashBefore = Economy->GetCash();
        CargoRunsBefore = Logistics->GetCargoCompletedRuns();
        Authority->ResetForNewContract();
        bEnvironmentPrepared = true;
        StepStartedAt = Elapsed;
        Pass(TEXT("WORLD"));
    }

    if (Step == 0)
    {
        if (Elapsed - StepStartedAt < 1.0f) return;
        if (!Director->TryStartJob(PlayerPawn)) return;
        if (Director->GetStage() != EGTTFarmJobStage::ReachPickup)
        {
            Fail(TEXT("TryStartJob returned true without ReachPickup stage"));
            return;
        }
        Pass(TEXT("CONTRACT"));
        Step = 1;
        StepStartedAt = Elapsed;
        return;
    }

    if (Step == 1)
    {
        StageVehicleAt(PickupLocation);
        PC->Possess(Vehicle);
        AGTTFarmJobTerminal* Pickup = PickupTerminal.Get();
        if (!Pickup)
        {
            Fail(TEXT("Feed Depot pickup terminal disappeared"));
            return;
        }
        Pickup->Interact_Implementation(PlayerPawn);
        if (!Authority->HasLoadedVehicle())
        {
            Fail(TEXT("Feed Depot terminal did not lock the exact loaded vehicle"));
            return;
        }
        if (Director->GetStage() != EGTTFarmJobStage::DeliverCargo)
        {
            Fail(TEXT("pickup terminal did not advance to DeliverCargo"));
            return;
        }
        Pass(TEXT("LOAD_LOCK"));
        Step = 2;
        StepStartedAt = Elapsed;
        return;
    }

    if (Step == 2)
    {
        // Put the real loaded Mulebox well outside the Hill Farm yard. A decoy can be parked
        // at the terminal by a future manual test; the authority decision must depend only on
        // the locked cargo actor, never on "any healthy nearby vehicle".
        StageVehicleAt(HillFarmLocation + FVector(1800.0f, 0.0f, 0.0f));
        FString FailureReason;
        float SpeedKmh = 0.0f;
        float DistanceCm = 0.0f;
        if (Authority->ValidateHandoff(HillFarmLocation, FailureReason, SpeedKmh, DistanceCm))
        {
            Fail(TEXT("handoff incorrectly accepted while exact loaded vehicle was outside the yard"));
            return;
        }
        if (DistanceCm <= 750.0f)
        {
            Fail(TEXT("wrong-vehicle rejection did not exercise the distance authority gate"));
            return;
        }
        if (AGTTFarmJobTerminal* HillTerminal = HillFarmTerminal.Get()) HillTerminal->Interact_Implementation(PlayerPawn);
        if (Director->GetStage() != EGTTFarmJobStage::DeliverCargo)
        {
            Fail(TEXT("Hill Farm terminal bypassed exact loaded-vehicle authority while loaded vehicle was away"));
            return;
        }
        Pass(TEXT("WRONG_VEHICLE_REJECT"));
        Step = 3;
        StepStartedAt = Elapsed;
        return;
    }

    if (Step == 3)
    {
        StageVehicleAt(HillFarmLocation);
        Step = 4;
        StepStartedAt = Elapsed;
        return;
    }

    if (Step == 4)
    {
        if (Elapsed - StepStartedAt < 0.75f) return;
        FString FailureReason;
        float SpeedKmh = 0.0f;
        float DistanceCm = 0.0f;
        if (!Authority->ValidateHandoff(HillFarmLocation, FailureReason, SpeedKmh, DistanceCm)) return;
        AGTTFarmJobTerminal* HillTerminal = HillFarmTerminal.Get();
        if (!HillTerminal)
        {
            Fail(TEXT("Hill Farm terminal disappeared before accepted handoff"));
            return;
        }
        HillTerminal->Interact_Implementation(PlayerPawn);
        if (Director->GetStage() == EGTTFarmJobStage::DeliverCargo)
        {
            Fail(TEXT("Hill Farm terminal did not advance after exact loaded-vehicle authority passed"));
            return;
        }
        const bool bCompleteAtHill = Director->GetStage() == EGTTFarmJobStage::Idle;
        Pass(TEXT("HILL_HANDOFF"));
        if (bCompleteAtHill)
        {
            CompleteScenario();
            return;
        }
        if (Director->GetStage() != EGTTFarmJobStage::DeliverFinalStop || !bHasFinalStop)
        {
            Fail(TEXT("extended cargo route did not expose a valid final stop"));
            return;
        }
        Step = 5;
        StepStartedAt = Elapsed;
        return;
    }

    if (Step == 5)
    {
        StageVehicleAt(FinalStopLocation);
        Step = 6;
        StepStartedAt = Elapsed;
        return;
    }

    if (Step == 6)
    {
        if (Elapsed - StepStartedAt < 0.75f) return;
        FString FailureReason;
        float SpeedKmh = 0.0f;
        float DistanceCm = 0.0f;
        if (!Authority->ValidateHandoff(FinalStopLocation, FailureReason, SpeedKmh, DistanceCm)) return;
        AGTTFarmJobTerminal* FinalTerminal = FinalStopTerminal.Get();
        if (!FinalTerminal)
        {
            Fail(TEXT("North Wood Yard terminal disappeared before final handoff"));
            return;
        }
        FinalTerminal->Interact_Implementation(PlayerPawn);
        if (Director->GetStage() != EGTTFarmJobStage::Idle)
        {
            Fail(TEXT("North Wood Yard terminal did not complete the contract"));
            return;
        }
        Pass(TEXT("FINAL_HANDOFF"));
        CompleteScenario();
    }
}
