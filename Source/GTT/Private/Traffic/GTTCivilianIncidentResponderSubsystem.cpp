#include "Traffic/GTTCivilianIncidentResponderSubsystem.h"

#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTCivilianResponderSaveGame.h"
#include "Traffic/GTTCivilianIncidentDispatchSubsystem.h"
#include "Traffic/GTTRoadsideResponderVehicle.h"
#include "Traffic/GTTTrafficCarPawn.h"
#include "GTT.h"

namespace
{
    const FString CivilianResponderSaveSlot = TEXT("GTT_CivilianResponder_01");
}

void UGTTCivilianIncidentResponderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadCheckpoint();
}

void UGTTCivilianIncidentResponderSubsystem::Deinitialize()
{
    if (!TrackedIncidentId.IsNone() && !bRecoveryCompletedThisSession)
    {
        SaveCheckpoint();
    }
    ClearResponderSceneAuthority();
    DestroyResponderVehicle();
    Super::Deinitialize();
}

TStatId UGTTCivilianIncidentResponderSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTCivilianIncidentResponderSubsystem, STATGROUP_Tickables);
}

void UGTTCivilianIncidentResponderSubsystem::Tick(float DeltaSeconds)
{
    ScanAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (ScanAccumulator < ScanIntervalSeconds)
    {
        return;
    }

    const float Elapsed = ScanAccumulator;
    ScanAccumulator = 0.0f;
    CheckpointAccumulator += Elapsed;

    UWorld* World = GetWorld();
    UGTTCivilianIncidentDispatchSubsystem* Dispatch = World ? World->GetSubsystem<UGTTCivilianIncidentDispatchSubsystem>() : nullptr;
    if (!Dispatch || !Dispatch->HasActiveDispatch())
    {
        if (!TrackedIncidentId.IsNone())
        {
            MissingDispatchSeconds += Elapsed;
            if (MissingDispatchSeconds >= MissingDispatchExpirySeconds)
            {
                CancelResponder(TEXT("authoritative-dispatch-ended"), true);
                TrackedIncidentId = NAME_None;
                bRecoveryCompletedThisSession = false;
                bRestoredCheckpoint = false;
                MissingDispatchSeconds = 0.0f;
                ClearCheckpoint();
            }
        }
        return;
    }

    MissingDispatchSeconds = 0.0f;
    const FName DispatchIncidentId = Dispatch->GetActiveIncidentId();
    if (DispatchIncidentId.IsNone())
    {
        return;
    }

    if (TrackedIncidentId != DispatchIncidentId)
    {
        CancelResponder(TEXT("authoritative-dispatch-changed"), true);
        TrackedIncidentId = DispatchIncidentId;
        PlayerGraceElapsed = 0.0f;
        bRecoveryCompletedThisSession = false;
        bRestoredCheckpoint = false;
        SaveCheckpoint();
    }

    if (bRecoveryCompletedThisSession)
    {
        return;
    }

    AGTTTrafficCarPawn* Vehicle = FindDispatchVehicle(Dispatch);
    const FGTTCivilianIncidentDispatchPresentation DispatchPresentation = Dispatch->GetPresentationSnapshot(nullptr);

    if (Dispatch->GetActiveIncidentSeverity() < SevereIncidentThreshold)
    {
        CancelResponder(TEXT("incident-below-responder-threshold"), true);
        ClearCheckpoint();
        return;
    }

    if (DispatchPresentation.bWardenTrafficControl)
    {
        if (Phase != EGTTCivilianResponderPhase::None)
        {
            CancelResponder(TEXT("warden-traffic-control-priority"), true);
            NotifyPlayer(TEXT("COUNTY ROAD SERVICE: responder yielding to warden traffic control."), 4.5f);
        }
        PlayerGraceElapsed = 0.0f;
        if (CheckpointAccumulator >= CheckpointIntervalSeconds)
        {
            CheckpointAccumulator = 0.0f;
            SaveCheckpoint();
        }
        return;
    }

    if (!Vehicle)
    {
        if (CheckpointAccumulator >= CheckpointIntervalSeconds)
        {
            CheckpointAccumulator = 0.0f;
            SaveCheckpoint();
        }
        return;
    }

    if (Vehicle->IsRoadsideAssistanceActive())
    {
        if (Phase != EGTTCivilianResponderPhase::None)
        {
            CancelResponder(TEXT("player-assistance-took-scene"), true);
            NotifyPlayer(TEXT("COUNTY ROAD SERVICE: response cancelled; player roadside assist has scene priority."), 4.5f);
        }
        PlayerGraceElapsed = 0.0f;
        return;
    }

    if (Phase == EGTTCivilianResponderPhase::None)
    {
        PlayerGraceElapsed += Elapsed;
        if (PlayerGraceElapsed >= PlayerAssistGraceSeconds)
        {
            RequestResponder(Vehicle, false);
        }
        else if (CheckpointAccumulator >= CheckpointIntervalSeconds)
        {
            CheckpointAccumulator = 0.0f;
            SaveCheckpoint();
        }
        return;
    }

    if (!ResponderVehicle.IsValid())
    {
        RequestResponder(Vehicle, Phase == EGTTCivilianResponderPhase::OnScene);
    }

    AGTTRoadsideResponderVehicle* Responder = ResponderVehicle.Get();
    if (!Responder)
    {
        return;
    }

    if (Phase == EGTTCivilianResponderPhase::EnRoute && Responder->IsParkedAtScene())
    {
        Phase = EGTTCivilianResponderPhase::OnScene;
        SceneHoldRemaining = ResponderSceneHoldSeconds;
        ClearResponderSceneAuthority();
        AuthorityVehicle = Vehicle;
        Vehicle->SetRoadsideResponderSceneAuthority(true);
        SaveCheckpoint();
        NotifyPlayer(TEXT("COUNTY ROAD SERVICE ON SCENE: responder has the disabled vehicle. Player payout is no longer available for this incident."), 5.5f);
        UE_LOG(LogGTT, Log,
            TEXT("CIVILIAN_RESPONDER_ON_SCENE incident=%s car=%s"),
            *TrackedIncidentId.ToString(), *Vehicle->GetName());
    }

    if (Phase == EGTTCivilianResponderPhase::OnScene)
    {
        if (AuthorityVehicle.Get() != Vehicle)
        {
            ClearResponderSceneAuthority();
            AuthorityVehicle = Vehicle;
        }
        Vehicle->SetRoadsideResponderSceneAuthority(true);
        SceneHoldRemaining = FMath::Max(0.0f, SceneHoldRemaining - Elapsed);
        if (SceneHoldRemaining <= 0.0f)
        {
            if (Vehicle->CompleteRoadsideResponderRecovery())
            {
                NotifyPlayer(TEXT("COUNTY ROAD SERVICE: civilian vehicle recovered and returning to traffic. No player reward issued."), 5.0f);
                UE_LOG(LogGTT, Log,
                    TEXT("CIVILIAN_RESPONDER_HANDOFF_COMPLETE incident=%s car=%s"),
                    *TrackedIncidentId.ToString(), *Vehicle->GetName());
                AuthorityVehicle.Reset();
                DestroyResponderVehicle();
                Phase = EGTTCivilianResponderPhase::None;
                SceneHoldRemaining = 0.0f;
                bRecoveryCompletedThisSession = true;
                ClearCheckpoint();
                return;
            }

            SceneHoldRemaining = 2.0f;
            SaveCheckpoint();
        }
    }

    if (CheckpointAccumulator >= CheckpointIntervalSeconds)
    {
        CheckpointAccumulator = 0.0f;
        SaveCheckpoint();
    }
}

AGTTTrafficCarPawn* UGTTCivilianIncidentResponderSubsystem::FindDispatchVehicle(const UGTTCivilianIncidentDispatchSubsystem* Dispatch) const
{
    UWorld* World = GetWorld();
    if (!World || !Dispatch)
    {
        return nullptr;
    }

    for (TActorIterator<AGTTTrafficCarPawn> It(World); It; ++It)
    {
        AGTTTrafficCarPawn* Candidate = *It;
        if (Candidate && Dispatch->IsTrackedVehicle(Candidate))
        {
            return Candidate;
        }
    }
    return nullptr;
}

void UGTTCivilianIncidentResponderSubsystem::RequestResponder(AGTTTrafficCarPawn* Vehicle, bool bStartAtScene)
{
    if (!GetWorld() || !Vehicle || TrackedIncidentId.IsNone())
    {
        return;
    }

    if (ResponderVehicle.IsValid())
    {
        if (bStartAtScene && !ResponderVehicle->IsParkedAtScene())
        {
            DestroyResponderVehicle();
        }
        else
        {
            return;
        }
    }

    const FVector SceneLocation = Vehicle->GetActorLocation();
    FVector SpawnLocation = SceneLocation + FVector(-ResponderSpawnDistanceCm, 260.0f, 110.0f);
    if (bStartAtScene)
    {
        SpawnLocation = SceneLocation + FVector(-360.0f, 240.0f, 90.0f);
    }

    const FRotator SpawnRotation = (SceneLocation - SpawnLocation).Rotation();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGTTRoadsideResponderVehicle* Spawned = GetWorld()->SpawnActor<AGTTRoadsideResponderVehicle>(
        AGTTRoadsideResponderVehicle::StaticClass(), SpawnLocation, SpawnRotation, Params);
    if (!Spawned)
    {
        UE_LOG(LogGTT, Warning, TEXT("CIVILIAN_RESPONDER_SPAWN_FAILED incident=%s"), *TrackedIncidentId.ToString());
        return;
    }

    Spawned->InitializeIncidentResponse(TrackedIncidentId, SceneLocation, bStartAtScene);
    ResponderVehicle = Spawned;
    Phase = bStartAtScene ? EGTTCivilianResponderPhase::OnScene : EGTTCivilianResponderPhase::EnRoute;
    if (bStartAtScene)
    {
        SceneHoldRemaining = FMath::Max(0.5f, SceneHoldRemaining);
        ClearResponderSceneAuthority();
        AuthorityVehicle = Vehicle;
        Vehicle->SetRoadsideResponderSceneAuthority(true);
    }
    SaveCheckpoint();

    NotifyPlayer(
        bStartAtScene
            ? TEXT("COUNTY ROAD SERVICE RESTORED: responder scene reacquired after load.")
            : TEXT("COUNTY ROAD SERVICE DISPATCHED: severe civilian incident still unresolved; responder en route."),
        5.0f);

    UE_LOG(LogGTT, Log,
        TEXT("CIVILIAN_RESPONDER_DISPATCH incident=%s phase=%s scene=%s"),
        *TrackedIncidentId.ToString(),
        bStartAtScene ? TEXT("ON_SCENE") : TEXT("EN_ROUTE"),
        *SceneLocation.ToCompactString());

    bRestoredCheckpoint = false;
}

void UGTTCivilianIncidentResponderSubsystem::CancelResponder(const TCHAR* Reason, bool bResetGrace)
{
    ClearResponderSceneAuthority();

    if (Phase != EGTTCivilianResponderPhase::None || ResponderVehicle.IsValid())
    {
        UE_LOG(LogGTT, Log,
            TEXT("CIVILIAN_RESPONDER_CANCEL incident=%s reason=%s"),
            *TrackedIncidentId.ToString(), Reason ? Reason : TEXT("unknown"));
    }

    DestroyResponderVehicle();
    Phase = EGTTCivilianResponderPhase::None;
    SceneHoldRemaining = 0.0f;
    if (bResetGrace)
    {
        PlayerGraceElapsed = 0.0f;
    }
}

void UGTTCivilianIncidentResponderSubsystem::ClearResponderSceneAuthority()
{
    if (AGTTTrafficCarPawn* Vehicle = AuthorityVehicle.Get())
    {
        Vehicle->SetRoadsideResponderSceneAuthority(false);
    }
    AuthorityVehicle.Reset();

    // Defensive fail-closed cleanup for travel/rebind cases where the weak pointer
    // could have expired while a traffic actor still carries the transient flag.
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<AGTTTrafficCarPawn> It(World); It; ++It)
        {
            AGTTTrafficCarPawn* Candidate = *It;
            if (Candidate && Candidate->IsRoadsideResponderSceneAuthority())
            {
                Candidate->SetRoadsideResponderSceneAuthority(false);
            }
        }
    }
}

void UGTTCivilianIncidentResponderSubsystem::DestroyResponderVehicle()
{
    if (AGTTRoadsideResponderVehicle* Responder = ResponderVehicle.Get())
    {
        Responder->Destroy();
    }
    ResponderVehicle.Reset();
}

void UGTTCivilianIncidentResponderSubsystem::SaveCheckpoint() const
{
    if (TrackedIncidentId.IsNone() || bRecoveryCompletedThisSession)
    {
        ClearCheckpoint();
        return;
    }

    UGTTCivilianResponderSaveGame* Save = Cast<UGTTCivilianResponderSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UGTTCivilianResponderSaveGame::StaticClass()));
    if (!Save)
    {
        return;
    }

    Save->SchemaVersion = 1;
    Save->bActive = true;
    Save->IncidentId = TrackedIncidentId;
    Save->Phase = static_cast<uint8>(Phase);
    Save->PlayerGraceElapsed = FMath::Clamp(PlayerGraceElapsed, 0.0f, PlayerAssistGraceSeconds);
    Save->SceneHoldRemaining = FMath::Clamp(SceneHoldRemaining, 0.0f, ResponderSceneHoldSeconds);
    UGameplayStatics::SaveGameToSlot(Save, CivilianResponderSaveSlot, 0);
}

void UGTTCivilianIncidentResponderSubsystem::LoadCheckpoint()
{
    if (!UGameplayStatics::DoesSaveGameExist(CivilianResponderSaveSlot, 0))
    {
        return;
    }

    const UGTTCivilianResponderSaveGame* Save = Cast<UGTTCivilianResponderSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CivilianResponderSaveSlot, 0));
    if (!Save || Save->SchemaVersion != 1 || !Save->bActive || Save->IncidentId.IsNone() || Save->Phase > static_cast<uint8>(EGTTCivilianResponderPhase::OnScene))
    {
        ClearCheckpoint();
        return;
    }

    TrackedIncidentId = Save->IncidentId;
    Phase = static_cast<EGTTCivilianResponderPhase>(Save->Phase);
    PlayerGraceElapsed = FMath::Clamp(Save->PlayerGraceElapsed, 0.0f, PlayerAssistGraceSeconds);
    SceneHoldRemaining = FMath::Clamp(Save->SceneHoldRemaining, 0.0f, ResponderSceneHoldSeconds);
    bRestoredCheckpoint = true;
    bRecoveryCompletedThisSession = false;
    MissingDispatchSeconds = 0.0f;

    UE_LOG(LogGTT, Log,
        TEXT("CIVILIAN_RESPONDER_LOAD incident=%s phase=%d grace=%.1f scene_hold=%.1f"),
        *TrackedIncidentId.ToString(), static_cast<int32>(Phase), PlayerGraceElapsed, SceneHoldRemaining);
}

void UGTTCivilianIncidentResponderSubsystem::ClearCheckpoint() const
{
    if (UGameplayStatics::DoesSaveGameExist(CivilianResponderSaveSlot, 0))
    {
        UGameplayStatics::DeleteGameInSlot(CivilianResponderSaveSlot, 0);
    }
}

void UGTTCivilianIncidentResponderSubsystem::NotifyPlayer(const FString& Message, float Duration) const
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn)
    {
        return;
    }

    if (UGTTPlayerEconomyComponent* Economy = PlayerPawn->FindComponentByClass<UGTTPlayerEconomyComponent>())
    {
        Economy->PushMessage(Message, Duration);
    }
}
