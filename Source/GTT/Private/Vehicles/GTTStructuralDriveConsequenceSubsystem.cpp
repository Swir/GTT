#include "Vehicles/GTTStructuralDriveConsequenceSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Core/GTTStructuralDamageEvidenceSubsystem.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "World/GTTServiceTerminal.h"
#include "GTT.h"

namespace
{
    constexpr float MinPhysicalSpeedCmS = 80.0f;
    constexpr float TelemetryIntervalSeconds = 4.0f;
    constexpr float EvidenceTimeoutSeconds = 185.0f;
    constexpr float StateTolerance = 0.06f;

    float MinBodyHealth(const FGTTRoadBodyDamageSnapshot& Body)
    {
        return FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth));
    }
}

void UGTTStructuralDriveConsequenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEvidenceEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEvidenceEnabled)
    {
        GTT_LOG( Display,
            TEXT("DEMO_SCENARIO_STRUCTURAL_DRIVE_BEGIN version=11 route=damage-physics-save-load-workshop"));
    }
}

FGTTStructuralDriveState UGTTStructuralDriveConsequenceSubsystem::GetDriveStateForVehicle(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    FGTTStructuralDriveState State;
    if (!Vehicle || !Vehicle->IsNativeReady() || !Vehicle->IsLegacyTakeoverActive())
    {
        return State;
    }

    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    const float FrontLoss = 1.0f - FMath::Clamp(Body.FrontHealth, 0.0f, 1.0f);
    const float RearLoss = 1.0f - FMath::Clamp(Body.RearHealth, 0.0f, 1.0f);
    const float LeftLoss = 1.0f - FMath::Clamp(Body.LeftHealth, 0.0f, 1.0f);
    const float RightLoss = 1.0f - FMath::Clamp(Body.RightHealth, 0.0f, 1.0f);
    const float SideLoss = FMath::Max(LeftLoss, RightLoss);
    const float PanelLoss = FMath::Clamp(static_cast<float>(Body.DetachedPanelCount) / 4.0f, 0.0f, 1.0f);

    State.CoolingStress = FMath::Clamp(Body.CoolingStress, 0.0f, 1.0f);
    State.DetachedPanels = FMath::Clamp(Body.DetachedPanelCount, 0, 4);
    State.DamageSeverity = FMath::Clamp(
        FrontLoss * 0.34f + SideLoss * 0.24f + RearLoss * 0.10f +
        State.CoolingStress * 0.24f + PanelLoss * 0.16f,
        0.0f, 1.0f);

    State.bLimpHomeActive = State.CoolingStress >= 0.42f || Body.FrontHealth <= 0.48f || State.DamageSeverity >= 0.58f;
    State.PowerRetention = FMath::Clamp(
        1.0f - FrontLoss * 0.24f - State.CoolingStress * 0.32f - PanelLoss * 0.08f,
        State.bLimpHomeActive ? 0.42f : 0.58f,
        1.0f);
    State.SteeringRetention = FMath::Clamp(
        1.0f - SideLoss * 0.34f - PanelLoss * 0.07f,
        0.54f, 1.0f);

    if (State.DamageSeverity > 0.015f)
    {
        State.DragRatePerSecond = FMath::Clamp(0.08f + State.DamageSeverity * 0.58f +
            State.CoolingStress * 0.20f + (State.bLimpHomeActive ? 0.26f : 0.0f),
            0.0f, 0.95f);
    }

    const float SideImbalance = FMath::Clamp(Body.LeftHealth - Body.RightHealth, -1.0f, 1.0f);
    State.LateralPullRate = FMath::Clamp(SideImbalance * 0.18f, -0.16f, 0.16f);
    return State;
}

void UGTTStructuralDriveConsequenceSubsystem::ApplyDriveConsequences(AGTTRoadVehicleNativePawn* Vehicle, float DeltaTime)
{
    if (!Vehicle || !Vehicle->IsNativeReady() || !Vehicle->IsLegacyTakeoverActive()) return;

    USkeletalMeshComponent* Mesh = Vehicle->GetMesh();
    if (!Mesh || !Mesh->IsSimulatingPhysics()) return;

    const FGTTStructuralDriveState State = GetDriveStateForVehicle(Vehicle);
    if (State.DamageSeverity <= 0.015f) return;

    const FVector Velocity = Vehicle->GetVelocity();
    const float SpeedCmS = Velocity.Size();
    if (SpeedCmS < MinPhysicalSpeedCmS) return;

    const float MassKg = FMath::Max(1.0f, Mesh->GetMass());
    const FVector DragForce = -Velocity * MassKg * State.DragRatePerSecond;
    Mesh->AddForce(DragForce, NAME_None, false);

    if (!FMath::IsNearlyZero(State.LateralPullRate, 0.002f))
    {
        const FVector LateralForce = Vehicle->GetActorRightVector() * MassKg * SpeedCmS * State.LateralPullRate;
        Mesh->AddForce(LateralForce, NAME_None, false);
    }

    if (State.bLimpHomeActive && State.CoolingStress >= 0.75f)
    {
        const FVector CoolingDrag = -Velocity.GetSafeNormal() * MassKg * 110.0f * State.CoolingStress;
        Mesh->AddForce(CoolingDrag, NAME_None, false);
    }

    (void)DeltaTime;
}

AGTTRoadVehicleNativePawn* UGTTStructuralDriveConsequenceSubsystem::FindActiveOwnedRoadVehicle() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->IsNativeReady() || !Vehicle->IsLegacyTakeoverActive()) continue;
        if (!Vehicle->GetMigrationSnapshot().bOwnedByPlayer) continue;
        const FName Id = Vehicle->GetPersistentVehicleId();
        if (Id == FName(TEXT("Rattleback82")) || Id == FName(TEXT("Mulebox1200"))) return Vehicle;
    }
    return nullptr;
}

AGTTServiceTerminal* UGTTStructuralDriveConsequenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It)) return *It;
    }
    return nullptr;
}

void UGTTStructuralDriveConsequenceSubsystem::FailEvidence(const FString& Reason)
{
    GTT_LOG( Error,
        TEXT("DEMO_SCENARIO_STRUCTURAL_DRIVE result=FAIL phase=%d reason=%s elapsed=%.2f"),
        static_cast<int32>(EvidencePhase), *Reason, EvidenceElapsed);
    bEvidenceFinished = true;
    EvidencePhase = EEvidencePhase::Complete;
}

void UGTTStructuralDriveConsequenceSubsystem::TickEvidence(float DeltaTime)
{
    if (!bEvidenceEnabled || bEvidenceFinished) return;
    EvidenceElapsed += DeltaTime;
    UWorld* World = GetWorld();
    if (!World) return;
    if (EvidenceElapsed > EvidenceTimeoutSeconds)
    {
        FailEvidence(TEXT("structural drive consequence evidence timeout"));
        return;
    }

    switch (EvidencePhase)
    {
        case EEvidencePhase::WaitForStructuralRecovery:
        {
            UGTTStructuralDamageEvidenceSubsystem* Previous = World->GetSubsystem<UGTTStructuralDamageEvidenceSubsystem>();
            if (!Previous || Previous->IsTickable()) return;
            EvidenceVehicle = FindActiveOwnedRoadVehicle();
            if (!EvidenceVehicle.IsValid())
            {
                if (EvidenceElapsed > 155.0f) FailEvidence(TEXT("structural recovery completed without an active owned Native road vehicle"));
                return;
            }
            EvidenceVehicleId = EvidenceVehicle->GetPersistentVehicleId();
            EvidencePhase = EEvidencePhase::StageDamage;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::StageDamage:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!Vehicle)
            {
                FailEvidence(TEXT("target vehicle disappeared before structural drive staging"));
                return;
            }

            const bool bFrontImpact = Vehicle->ApplyScriptedImpactDamage(110.0f, EGTTRoadDamageZone::Front);
            const bool bSideImpact = Vehicle->ApplyScriptedImpactDamage(96.0f, EGTTRoadDamageZone::Right);
            if (!bFrontImpact || !bSideImpact)
            {
                FailEvidence(TEXT("scripted impacts failed to create drive-affecting structural damage"));
                return;
            }
            EvidencePhase = EEvidencePhase::VerifyDamagedDynamics;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::VerifyDamagedDynamics:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 0.40f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!Vehicle)
            {
                FailEvidence(TEXT("target vehicle missing while verifying damaged dynamics"));
                return;
            }
            DamagedState = GetDriveStateForVehicle(Vehicle);
            const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
            if (!DamagedState.bLimpHomeActive || DamagedState.DamageSeverity < 0.30f ||
                DamagedState.DragRatePerSecond < 0.28f || FMath::Abs(DamagedState.LateralPullRate) < 0.025f ||
                DamagedState.PowerRetention >= 0.90f || DamagedState.SteeringRetention >= 0.94f)
            {
                FailEvidence(TEXT("structural damage did not create measurable limp-home drag, pull and control loss"));
                return;
            }

            GTT_LOG( Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_HANDLING vehicle=%s result=PASS severity=%.3f drag_rate=%.3f pull_rate=%.3f power_retention=%.3f steering_retention=%.3f cooling=%.3f limp=YES front=%.3f right=%.3f panels=%d"),
                *EvidenceVehicleId.ToString(), DamagedState.DamageSeverity, DamagedState.DragRatePerSecond,
                DamagedState.LateralPullRate, DamagedState.PowerRetention, DamagedState.SteeringRetention,
                DamagedState.CoolingStress, Body.FrontHealth, Body.RightHealth, Body.DetachedPanelCount);
            EvidencePhase = EEvidencePhase::SaveDamagedDynamics;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::SaveDamagedDynamics:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 0.25f) return;
            AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>();
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!GameMode || !Vehicle || !GameMode->SaveProgress())
            {
                FailEvidence(TEXT("SaveProgress failed for structural handling consequence"));
                return;
            }

            FGTTRoadBodyDamageSnapshot Pristine;
            Vehicle->RestorePersistentBodyDamage(Pristine, 0);
            if (!GameMode->LoadProgress())
            {
                FailEvidence(TEXT("LoadProgress failed for structural handling consequence"));
                return;
            }
            EvidencePhase = EEvidencePhase::VerifyReloadedDynamics;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::VerifyReloadedDynamics:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 0.75f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            if (!Vehicle || !Vehicle->IsLegacyTakeoverActive())
            {
                FailEvidence(TEXT("Native takeover did not resume for structural handling verification"));
                return;
            }
            const FGTTStructuralDriveState Reloaded = GetDriveStateForVehicle(Vehicle);
            if (!Reloaded.bLimpHomeActive ||
                FMath::Abs(Reloaded.DamageSeverity - DamagedState.DamageSeverity) > StateTolerance ||
                FMath::Abs(Reloaded.DragRatePerSecond - DamagedState.DragRatePerSecond) > StateTolerance ||
                FMath::Abs(Reloaded.LateralPullRate - DamagedState.LateralPullRate) > StateTolerance ||
                FMath::Abs(Reloaded.PowerRetention - DamagedState.PowerRetention) > StateTolerance ||
                FMath::Abs(Reloaded.SteeringRetention - DamagedState.SteeringRetention) > StateTolerance)
            {
                FailEvidence(TEXT("persisted structural damage did not restore the same limp-home drive consequence"));
                return;
            }

            GTT_LOG( Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_RELOAD_HANDLING vehicle=%s result=PASS severity_before=%.3f severity_after=%.3f drag_before=%.3f drag_after=%.3f pull_before=%.3f pull_after=%.3f power_before=%.3f power_after=%.3f steering_before=%.3f steering_after=%.3f limp=YES"),
                *EvidenceVehicleId.ToString(), DamagedState.DamageSeverity, Reloaded.DamageSeverity,
                DamagedState.DragRatePerSecond, Reloaded.DragRatePerSecond,
                DamagedState.LateralPullRate, Reloaded.LateralPullRate,
                DamagedState.PowerRetention, Reloaded.PowerRetention,
                DamagedState.SteeringRetention, Reloaded.SteeringRetention);
            EvidencePhase = EEvidencePhase::PrepareWorkshop;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::PrepareWorkshop:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 0.25f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Vehicle || !Terminal || !PlayerPawn || !Economy)
            {
                FailEvidence(TEXT("workshop integration unavailable for structural drive recovery"));
                return;
            }
            if (Economy->GetCash() < 550)
            {
                const int32 Reserve = 550 - Economy->GetCash();
                Economy->AddCash(Reserve, TEXT("Demo structural drive recovery reserve"));
                GTT_LOG( Display, TEXT("DEMO_SCENARIO_ACTION action=STRUCTURAL_DRIVE_WORKSHOP_RESERVE amount=%d"), Reserve);
            }
            Terminal->SetServiceType(EGTTServiceType::Workshop);
            Vehicle->SetActorLocation(Terminal->GetActorLocation() + Terminal->GetActorForwardVector() * 260.0f + FVector(0.0f, 0.0f, 85.0f),
                false, nullptr, ETeleportType::TeleportPhysics);
            CashBeforeWorkshop = Economy->GetCash();
            EvidencePhase = EEvidencePhase::InvokeWorkshop;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::InvokeWorkshop:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 0.50f) return;
            AGTTServiceTerminal* Terminal = FindWorkshopTerminal();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            if (!Terminal || !PlayerPawn)
            {
                FailEvidence(TEXT("workshop/player missing during structural drive recovery"));
                return;
            }
            Terminal->SetServiceType(EGTTServiceType::Workshop);
            Terminal->Interact_Implementation(PlayerPawn);
            EvidencePhase = EEvidencePhase::VerifyRecoveredDynamics;
            EvidencePhaseStarted = EvidenceElapsed;
            return;
        }

        case EEvidencePhase::VerifyRecoveredDynamics:
        {
            if (EvidenceElapsed - EvidencePhaseStarted < 1.0f) return;
            AGTTRoadVehicleNativePawn* Vehicle = EvidenceVehicle.Get();
            APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
            UGTTPlayerEconomyComponent* Economy = PlayerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn) : nullptr;
            if (!Vehicle || !Economy)
            {
                FailEvidence(TEXT("vehicle/economy missing after structural drive workshop"));
                return;
            }
            const FGTTStructuralDriveState Recovered = GetDriveStateForVehicle(Vehicle);
            const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
            const int32 Paid = CashBeforeWorkshop - Economy->GetCash();
            if (Recovered.bLimpHomeActive || Recovered.DamageSeverity > 0.015f ||
                Recovered.DragRatePerSecond > 0.01f || FMath::Abs(Recovered.LateralPullRate) > 0.01f ||
                Recovered.PowerRetention < 0.995f || Recovered.SteeringRetention < 0.995f ||
                MinBodyHealth(Body) < 0.999f || Body.CoolingStress > 0.01f || Body.DetachedPanelCount != 0 || Paid <= 0)
            {
                FailEvidence(TEXT("paid workshop did not restore pristine structural driving behavior"));
                return;
            }

            GTT_LOG( Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_DRIVE_RECOVERY vehicle=%s result=PASS cash_before=%d cash_after=%d paid=%d severity_before=%.3f severity_after=%.3f drag_before=%.3f drag_after=%.3f pull_before=%.3f pull_after=%.3f power_after=%.3f steering_after=%.3f limp_after=NO"),
                *EvidenceVehicleId.ToString(), CashBeforeWorkshop, Economy->GetCash(), Paid,
                DamagedState.DamageSeverity, Recovered.DamageSeverity,
                DamagedState.DragRatePerSecond, Recovered.DragRatePerSecond,
                DamagedState.LateralPullRate, Recovered.LateralPullRate,
                Recovered.PowerRetention, Recovered.SteeringRetention);
            GTT_LOG( Display,
                TEXT("DEMO_SCENARIO_STRUCTURAL_DRIVE result=PASS vehicle=%s route=damage-physics-save-load-workshop"),
                *EvidenceVehicleId.ToString());
            bEvidenceFinished = true;
            EvidencePhase = EEvidencePhase::Complete;
            return;
        }

        case EEvidencePhase::Complete:
        default:
            bEvidenceFinished = true;
            return;
    }
}

void UGTTStructuralDriveConsequenceSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || World->IsNetMode(NM_DedicatedServer)) return;

    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        ApplyDriveConsequences(*It, DeltaTime);
    }

    TelemetryAccumulator += DeltaTime;
    if (TelemetryAccumulator >= TelemetryIntervalSeconds)
    {
        TelemetryAccumulator = 0.0f;
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            AGTTRoadVehicleNativePawn* Vehicle = *It;
            if (!IsValid(Vehicle) || !Vehicle->IsLegacyTakeoverActive()) continue;
            const FGTTStructuralDriveState State = GetDriveStateForVehicle(Vehicle);
            if (State.DamageSeverity <= 0.015f) continue;
            GTT_LOG( Log,
                TEXT("NATIVE_STRUCTURAL_DRIVE_STATE vehicle=%s severity=%.3f drag_rate=%.3f pull_rate=%.3f power_retention=%.3f steering_retention=%.3f cooling=%.3f panels=%d limp=%s"),
                *Vehicle->GetPersistentVehicleId().ToString(), State.DamageSeverity, State.DragRatePerSecond,
                State.LateralPullRate, State.PowerRetention, State.SteeringRetention,
                State.CoolingStress, State.DetachedPanels, State.bLimpHomeActive ? TEXT("YES") : TEXT("NO"));
        }
    }

    TickEvidence(DeltaTime);
}
