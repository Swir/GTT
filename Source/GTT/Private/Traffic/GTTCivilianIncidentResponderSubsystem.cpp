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
    if (!TrackedIncidentId.IsNone() && (!bRecoveryCompletedThisSession || Phase == EGTTCivilianResponderPhase::ClearingScene))
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

    if (Phase == EGTTCivilianResponderPhase::ClearingScene)
    {
        MissingDispatchSeconds = 0.0f;
        AdvanceSceneClearance(Elapsed);
        if (Phase == EGTTCivilianResponderPhase::ClearingScene && CheckpointAccumulator >= CheckpointIntervalSeconds)
        {
            CheckpointAccumulator = 0.0f;
            SaveCheckpoint();
        }
        return;
    }

    if (!Dispatch || !Dispatch->HasActiveDispatch())
    {
        if (bRecoveryCompletedThisSession && Phase == EGTTCivilianResponderPhase::None)
        {
            TrackedIncidentId = NAME_None;
            LastSceneLocation = FVector::ZeroVector;
            MissingDispatchSeconds = 0.0f;
            bRecoveryCompletedThisSession = false;
            bRestoredCheckpoint = false;
            ClearCheckpoint();
            return;
        }

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
        GTT_LOG( Log,
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
                NotifyPlayer(TEXT("COUNTY ROAD SERVICE: civilian vehicle recovered. Safety crew is reopening the lane; no player reward issued."), 5.0f);
                GTT_LOG( Log,
                    TEXT("CIVILIAN_RESPONDER_HANDOFF_COMPLETE incident=%s car=%s"),
                    *TrackedIncidentId.ToString(), *Vehicle->GetName());
                BeginSceneClearance(Vehicle);
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
    LastSceneLocation = SceneLocation;
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
        GTT_LOG( Warning, TEXT("CIVILIAN_RESPONDER_SPAWN_FAILED incident=%s"), *TrackedIncidentId.ToString());
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

    GTT_LOG( Log,
        TEXT("CIVILIAN_RESPONDER_DISPATCH incident=%s phase=%s scene=%s"),
        *TrackedIncidentId.ToString(),
        bStartAtScene ? TEXT("ON_SCENE") : TEXT("EN_ROUTE"),
        *SceneLocation.ToCompactString());

    bRestoredCheckpoint = false;
}

void UGTTCivilianIncidentResponderSubsystem::RestoreClearingResponder()
{
    if (!GetWorld() || TrackedIncidentId.IsNone() || ResponderVehicle.IsValid())
    {
        return;
    }

    const FVector SpawnLocation = LastSceneLocation + FVector(-360.0f, 240.0f, 90.0f);
    const FRotator SpawnRotation = (LastSceneLocation - SpawnLocation).Rotation();
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGTTRoadsideResponderVehicle* Spawned = GetWorld()->SpawnActor<AGTTRoadsideResponderVehicle>(
        AGTTRoadsideResponderVehicle::StaticClass(), SpawnLocation, SpawnRotation, Params);
    if (!Spawned)
    {
        GTT_LOG( Warning, TEXT("CIVILIAN_RESPONDER_CLEARANCE_RESTORE_FAILED incident=%s"), *TrackedIncidentId.ToString());
        return;
    }

    Spawned->InitializeIncidentResponse(TrackedIncidentId, LastSceneLocation, true);
    Spawned->BeginSceneClearance();
    ResponderVehicle = Spawned;
    GTT_LOG( Log,
        TEXT("CIVILIAN_RESPONDER_CLEARANCE_RESTORED incident=%s remaining=%.1f scene=%s"),
        *TrackedIncidentId.ToString(), SceneClearanceRemaining, *LastSceneLocation.ToCompactString());
}

void UGTTCivilianIncidentResponderSubsystem::BeginSceneClearance(AGTTTrafficCarPawn* Vehicle)
{
    ClearResponderSceneAuthority();
    if (Vehicle)
    {
        LastSceneLocation = Vehicle->GetActorLocation();
    }

    Phase = EGTTCivilianResponderPhase::ClearingScene;
    SceneHoldRemaining = 0.0f;
    SceneClearanceRemaining = ResponderSceneClearanceSeconds;
    bRecoveryCompletedThisSession = true;

    if (AGTTRoadsideResponderVehicle* Responder = ResponderVehicle.Get())
    {
        Responder->BeginSceneClearance();
    }
    else
    {
        RestoreClearingResponder();
    }

    SaveCheckpoint();
    GTT_LOG( Log,
        TEXT("CIVILIAN_RESPONDER_CLEARANCE_BEGIN incident=%s remaining=%.1f"),
        *TrackedIncidentId.ToString(), SceneClearanceRemaining);
}

void UGTTCivilianIncidentResponderSubsystem::AdvanceSceneClearance(float Elapsed)
{
    if (Phase != EGTTCivilianResponderPhase::ClearingScene)
    {
        return;
    }

    if (!ResponderVehicle.IsValid())
    {
        RestoreClearingResponder();
    }

    if (AGTTRoadsideResponderVehicle* Responder = ResponderVehicle.Get())
    {
        if (!Responder->IsSceneClearing())
        {
            Responder->BeginSceneClearance();
        }
    }
    else
    {
        return;
    }

    SceneClearanceRemaining = FMath::Max(0.0f, SceneClearanceRemaining - FMath::Max(0.0f, Elapsed));
    if (SceneClearanceRemaining > 0.0f)
    {
        return;
    }

    NotifyPlayer(TEXT("COUNTY ROAD SERVICE: lane reopened; responder clear of the civilian scene."), 4.0f);
    GTT_LOG( Log,
        TEXT("CIVILIAN_RESPONDER_CLEARANCE_COMPLETE incident=%s"),
        *TrackedIncidentId.ToString());

    DestroyResponderVehicle();
    Phase = EGTTCivilianResponderPhase::None;
    SceneClearanceRemaining = 0.0f;
    LastSceneLocation = FVector::ZeroVector;
    ClearCheckpoint();
}

void UGTTCivilianIncidentResponderSubsystem::CancelResponder(const TCHAR* Reason, bool bResetGrace)
{
    ClearResponderSceneAuthority();

    if (Phase != EGTTCivilianResponderPhase::None || ResponderVehicle.IsValid())
    {
        GTT_LOG( Log,
            TEXT("CIVILIAN_RESPONDER_CANCEL incident=%s reason=%s"),
            *TrackedIncidentId.ToString(), Reason ? Reason : TEXT("unknown"));
    }

    DestroyResponderVehicle();
    Phase = EGTTCivilianResponderPhase::None;
    SceneHoldRemaining = 0.0f;
    SceneClearanceRemaining = 0.0f;
    LastSceneLocation = FVector::ZeroVector;
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
    if (TrackedIncidentId.IsNone() || (bRecoveryCompletedThisSession && Phase != EGTTCivilianResponderPhase::ClearingScene))
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

    Save->SchemaVersion = 2;
    Save->bActive = true;
    Save->IncidentId = TrackedIncidentId;
    Save->Phase = static_cast<uint8>(Phase);
    Save->PlayerGraceElapsed = FMath::Clamp(PlayerGraceElapsed, 0.0f, PlayerAssistGraceSeconds);
    Save->SceneHoldRemaining = FMath::Clamp(SceneHoldRemaining, 0.0f, ResponderSceneHoldSeconds);
    Save->SceneClearanceRemaining = FMath::Clamp(SceneClearanceRemaining, 0.0f, ResponderSceneClearanceSeconds);
    Save->SceneLocation = LastSceneLocation;
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
    if (!Save || !Save->bActive || Save->IncidentId.IsNone() || (Save->SchemaVersion != 1 && Save->SchemaVersion != 2))
    {
        ClearCheckpoint();
        return;
    }

    const uint8 MaxSupportedPhase = Save->SchemaVersion >= 2
        ? static_cast<uint8>(EGTTCivilianResponderPhase::ClearingScene)
        : static_cast<uint8>(EGTTCivilianResponderPhase::OnScene);
    if (Save->Phase > MaxSupportedPhase)
    {
        ClearCheckpoint();
        return;
    }

    TrackedIncidentId = Save->IncidentId;
    Phase = static_cast<EGTTCivilianResponderPhase>(Save->Phase);
    PlayerGraceElapsed = FMath::Clamp(Save->PlayerGraceElapsed, 0.0f, PlayerAssistGraceSeconds);
    SceneHoldRemaining = FMath::Clamp(Save->SceneHoldRemaining, 0.0f, ResponderSceneHoldSeconds);
    SceneClearanceRemaining = Save->SchemaVersion >= 2
        ? FMath::Clamp(Save->SceneClearanceRemaining, 0.0f, ResponderSceneClearanceSeconds)
        : 0.0f;
    LastSceneLocation = Save->SchemaVersion >= 2 ? Save->SceneLocation : FVector::ZeroVector;
    bRestoredCheckpoint = true;
    bRecoveryCompletedThisSession = Phase == EGTTCivilianResponderPhase::ClearingScene;
    MissingDispatchSeconds = 0.0f;

    if (Phase == EGTTCivilianResponderPhase::ClearingScene)
    {
        SceneClearanceRemaining = FMath::Max(0.5f, SceneClearanceRemaining);
        RestoreClearingResponder();
    }

    GTT_LOG( Log,
        TEXT("CIVILIAN_RESPONDER_LOAD incident=%s phase=%d grace=%.1f scene_hold=%.1f clearance=%.1f"),
        *TrackedIncidentId.ToString(), static_cast<int32>(Phase), PlayerGraceElapsed, SceneHoldRemaining, SceneClearanceRemaining);
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
