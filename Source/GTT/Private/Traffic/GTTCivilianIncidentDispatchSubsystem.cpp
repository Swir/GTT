#include "Traffic/GTTCivilianIncidentDispatchSubsystem.h"

#include "Components/TextRenderComponent.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Ranger/GTTRangerRoadStopSubsystem.h"
#include "Save/GTTCivilianIncidentSaveGame.h"
#include "Traffic/GTTTrafficCarPawn.h"
#include "GTT.h"

namespace
{
    const FString CivilianIncidentSaveSlot = TEXT("GTT_CivilianIncident_01");
}

void UGTTCivilianIncidentDispatchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadCheckpoint();
}

void UGTTCivilianIncidentDispatchSubsystem::Deinitialize()
{
    if (State != EGTTCivilianIncidentDispatchState::None)
    {
        SaveCheckpoint();
    }
    DestroyWorldMarker();
    TrackedVehicle.Reset();
    Super::Deinitialize();
}

TStatId UGTTCivilianIncidentDispatchSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTCivilianIncidentDispatchSubsystem, STATGROUP_Tickables);
}

void UGTTCivilianIncidentDispatchSubsystem::Tick(float DeltaSeconds)
{
    ScanAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (ScanAccumulator < ScanIntervalSeconds)
    {
        return;
    }

    const float Elapsed = ScanAccumulator;
    ScanAccumulator = 0.0f;

    if (State != EGTTCivilianIncidentDispatchState::None)
    {
        if (TrackedVehicle.IsValid())
        {
            OrphanSeconds = 0.0f;
            UpdateLifecycleFromVehicle();
        }
        else
        {
            OrphanSeconds += Elapsed;
            TryRebindSavedIncident();
            if (State != EGTTCivilianIncidentDispatchState::None &&
                !TrackedVehicle.IsValid() &&
                OrphanSeconds >= OrphanExpirySeconds)
            {
                ResolveDispatch(TEXT("saved-scene-expired-without-compatible-vehicle"), false);
            }
        }
    }

    ScanForIncidents();
}

bool UGTTCivilianIncidentDispatchSubsystem::IsTrackedVehicle(const AGTTTrafficCarPawn* Vehicle) const
{
    return Vehicle && TrackedVehicle.Get() == Vehicle && State != EGTTCivilianIncidentDispatchState::None;
}

FGTTCivilianIncidentDispatchPresentation UGTTCivilianIncidentDispatchSubsystem::GetPresentationSnapshot(const APawn* Viewer) const
{
    FGTTCivilianIncidentDispatchPresentation Snapshot;
    if (State == EGTTCivilianIncidentDispatchState::None)
    {
        return Snapshot;
    }

    Snapshot.bVisible = true;
    Snapshot.IncidentId = ActiveIncidentId;
    Snapshot.WorldLocation = ActiveLocation;
    Snapshot.Severity = ActiveSeverity;
    Snapshot.State = State;
    Snapshot.bBoundToLiveVehicle = TrackedVehicle.IsValid();
    Snapshot.bWardenTrafficControl = IsWardenTrafficControlBlocking(ActiveLocation);

    if (Viewer)
    {
        Snapshot.DistanceMeters = FVector::Dist2D(Viewer->GetActorLocation(), ActiveLocation) / 100.0f;
    }

    if (Snapshot.bWardenTrafficControl)
    {
        Snapshot.StateLabel = TEXT("WARDEN TRAFFIC CONTROL");
        Snapshot.Instruction = TEXT("Warden traffic control owns this roadside scene. Wait for the stop to clear.");
    }
    else if (!Snapshot.bBoundToLiveVehicle)
    {
        Snapshot.StateLabel = TEXT("REACQUIRING SCENE");
        Snapshot.Instruction = TEXT("Dispatch restored from save. Searching for the compatible disabled traffic vehicle near the saved scene.");
    }
    else if (State == EGTTCivilianIncidentDispatchState::AssistanceInProgress)
    {
        Snapshot.StateLabel = TEXT("ROADSIDE ASSIST");
        Snapshot.Instruction = TEXT("Stay within 5 m until the 6-second field assist completes.");
    }
    else
    {
        Snapshot.StateLabel = TEXT("CIVILIAN INCIDENT");
        Snapshot.Instruction = TEXT("Reach the marked disabled vehicle, interact, and stay within 5 m for 6 seconds.");
    }

    return Snapshot;
}

void UGTTCivilianIncidentDispatchSubsystem::ScanForIncidents()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    AGTTTrafficCarPawn* BestCandidate = nullptr;
    float BestSeverity = -1.0f;

    for (TActorIterator<AGTTTrafficCarPawn> It(World); It; ++It)
    {
        AGTTTrafficCarPawn* Candidate = *It;
        if (!Candidate ||
            !Candidate->IsIncidentDisabled() ||
            Candidate->WasRoadsideAssistanceCompletedForIncident())
        {
            continue;
        }

        if (IsTrackedVehicle(Candidate))
        {
            continue;
        }

        const float Severity = FMath::Clamp(Candidate->GetLastIncidentSeverity(), 0.0f, 1.0f);
        if (!BestCandidate || Severity > BestSeverity)
        {
            BestCandidate = Candidate;
            BestSeverity = Severity;
        }
    }

    if (!BestCandidate)
    {
        return;
    }

    if (State == EGTTCivilianIncidentDispatchState::None)
    {
        BeginDispatch(BestCandidate);
        return;
    }

    if (TrackedVehicle.IsValid() && BestSeverity >= ActiveSeverity + SupersedeSeverityDelta)
    {
        BeginDispatch(BestCandidate);
    }
}

void UGTTCivilianIncidentDispatchSubsystem::BeginDispatch(AGTTTrafficCarPawn* Vehicle)
{
    if (!Vehicle || !Vehicle->IsIncidentDisabled())
    {
        return;
    }

    DestroyWorldMarker();

    ActiveIncidentId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
    ActiveLocation = Vehicle->GetActorLocation();
    ActiveSeverity = FMath::Clamp(Vehicle->GetLastIncidentSeverity(), 0.0f, 1.0f);
    State = Vehicle->IsRoadsideAssistanceActive()
        ? EGTTCivilianIncidentDispatchState::AssistanceInProgress
        : EGTTCivilianIncidentDispatchState::Active;
    TrackedVehicle = Vehicle;
    OrphanSeconds = 0.0f;
    bRestoredFromCheckpoint = false;

    EnsureWorldMarker();
    SaveCheckpoint();

    const int32 SeverityPercent = FMath::RoundToInt(ActiveSeverity * 100.0f);
    NotifyPlayer(
        FString::Printf(
            TEXT("ROADSIDE DISPATCH: disabled civilian vehicle reported (%d%% severity). Look for ROADSIDE SOS."),
            SeverityPercent),
        6.0f);

    GTT_LOG( Log,
        TEXT("CIVILIAN_INCIDENT_DISPATCH_OPEN id=%s car=%s severity=%.2f location=%s"),
        *ActiveIncidentId.ToString(),
        *Vehicle->GetName(),
        ActiveSeverity,
        *ActiveLocation.ToCompactString());
}

bool UGTTCivilianIncidentDispatchSubsystem::TryRebindSavedIncident()
{
    if (State == EGTTCivilianIncidentDispatchState::None || TrackedVehicle.IsValid() || !GetWorld())
    {
        return false;
    }

    AGTTTrafficCarPawn* BestCandidate = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();
    const float RebindRadiusSq = FMath::Square(RebindRadiusCm);

    for (TActorIterator<AGTTTrafficCarPawn> It(GetWorld()); It; ++It)
    {
        AGTTTrafficCarPawn* Candidate = *It;
        if (!Candidate ||
            !Candidate->IsIncidentDisabled() ||
            Candidate->WasRoadsideAssistanceCompletedForIncident())
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared2D(Candidate->GetActorLocation(), ActiveLocation);
        const float CandidateSeverity = FMath::Clamp(Candidate->GetLastIncidentSeverity(), 0.0f, 1.0f);
        const bool bSeverityCompatible = CandidateSeverity + 0.35f >= ActiveSeverity;
        if (DistanceSq <= RebindRadiusSq && bSeverityCompatible && DistanceSq < BestDistanceSq)
        {
            BestCandidate = Candidate;
            BestDistanceSq = DistanceSq;
        }
    }

    if (!BestCandidate)
    {
        return false;
    }

    TrackedVehicle = BestCandidate;
    ActiveLocation = BestCandidate->GetActorLocation();
    ActiveSeverity = FMath::Max(ActiveSeverity, FMath::Clamp(BestCandidate->GetLastIncidentSeverity(), 0.0f, 1.0f));
    State = EGTTCivilianIncidentDispatchState::Active;
    OrphanSeconds = 0.0f;
    EnsureWorldMarker();
    SaveCheckpoint();

    if (bRestoredFromCheckpoint)
    {
        NotifyPlayer(TEXT("ROADSIDE DISPATCH RESTORED: disabled vehicle reacquired near the saved incident scene."), 6.0f);
        bRestoredFromCheckpoint = false;
    }

    GTT_LOG( Log,
        TEXT("CIVILIAN_INCIDENT_DISPATCH_REBOUND id=%s car=%s distance_cm=%.0f"),
        *ActiveIncidentId.ToString(),
        *BestCandidate->GetName(),
        FMath::Sqrt(BestDistanceSq));
    return true;
}

bool UGTTCivilianIncidentDispatchSubsystem::IsWardenTrafficControlBlocking(const FVector& SceneLocation) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    UGTTRangerRoadStopSubsystem* RoadStop = World->GetSubsystem<UGTTRangerRoadStopSubsystem>();
    if (!RoadStop || !RoadStop->HasTrafficControl())
    {
        return false;
    }

    return FVector::DistSquared2D(SceneLocation, RoadStop->GetPullOverTargetLocation()) <= FMath::Square(RangerAuthorityRadiusCm);
}

void UGTTCivilianIncidentDispatchSubsystem::UpdateLifecycleFromVehicle()
{
    AGTTTrafficCarPawn* Vehicle = TrackedVehicle.Get();
    if (!Vehicle)
    {
        return;
    }

    ActiveLocation = Vehicle->GetActorLocation();
    EnsureWorldMarker();

    if (Vehicle->WasRoadsideAssistanceCompletedForIncident())
    {
        ResolveDispatch(TEXT("roadside-assistance-complete"), true);
        return;
    }

    if (!Vehicle->IsIncidentDisabled())
    {
        ResolveDispatch(TEXT("civilian-vehicle-recovered"), true);
        return;
    }

    if (Vehicle->IsRoadsideAssistanceActive() && IsWardenTrafficControlBlocking(ActiveLocation))
    {
        if (Vehicle->CancelRoadsideAssistanceForTrafficControl())
        {
            State = EGTTCivilianIncidentDispatchState::Active;
            SaveCheckpoint();
            NotifyPlayer(TEXT("ROADSIDE ASSIST PAUSED: warden traffic control has priority at this scene."), 5.0f);
            GTT_LOG( Log,
                TEXT("CIVILIAN_INCIDENT_DISPATCH_WARDEN_PRIORITY id=%s car=%s"),
                *ActiveIncidentId.ToString(),
                *Vehicle->GetName());
        }
        return;
    }

    const EGTTCivilianIncidentDispatchState DesiredState = Vehicle->IsRoadsideAssistanceActive()
        ? EGTTCivilianIncidentDispatchState::AssistanceInProgress
        : EGTTCivilianIncidentDispatchState::Active;

    if (DesiredState != State)
    {
        State = DesiredState;
        SaveCheckpoint();

        if (State == EGTTCivilianIncidentDispatchState::AssistanceInProgress)
        {
            NotifyPlayer(TEXT("ROADSIDE DISPATCH: assistance started. Stay within 5 m for 6 seconds."), 4.0f);
        }
        else
        {
            NotifyPlayer(TEXT("ROADSIDE DISPATCH: assistance interrupted. The incident remains active."), 4.0f);
        }
    }
}

void UGTTCivilianIncidentDispatchSubsystem::ResolveDispatch(const TCHAR* Reason, bool bNotifyPlayer)
{
    if (State == EGTTCivilianIncidentDispatchState::None)
    {
        return;
    }

    GTT_LOG( Log,
        TEXT("CIVILIAN_INCIDENT_DISPATCH_CLOSE id=%s reason=%s"),
        *ActiveIncidentId.ToString(),
        Reason ? Reason : TEXT("unknown"));

    DestroyWorldMarker();
    TrackedVehicle.Reset();
    State = EGTTCivilianIncidentDispatchState::None;
    ActiveIncidentId = NAME_None;
    ActiveLocation = FVector::ZeroVector;
    ActiveSeverity = 0.0f;
    OrphanSeconds = 0.0f;
    bRestoredFromCheckpoint = false;
    ClearCheckpoint();

    if (bNotifyPlayer)
    {
        NotifyPlayer(TEXT("ROADSIDE DISPATCH CLOSED: civilian vehicle can continue. No duplicate reward is issued."), 4.5f);
    }
}

void UGTTCivilianIncidentDispatchSubsystem::SaveCheckpoint() const
{
    if (State == EGTTCivilianIncidentDispatchState::None)
    {
        ClearCheckpoint();
        return;
    }

    UGTTCivilianIncidentSaveGame* Save = Cast<UGTTCivilianIncidentSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UGTTCivilianIncidentSaveGame::StaticClass()));
    if (!Save)
    {
        return;
    }

    Save->SchemaVersion = 1;
    Save->bActive = true;
    Save->IncidentId = ActiveIncidentId;
    Save->IncidentLocation = ActiveLocation;
    Save->Severity = ActiveSeverity;
    Save->bAssistanceWasInProgress = State == EGTTCivilianIncidentDispatchState::AssistanceInProgress;
    UGameplayStatics::SaveGameToSlot(Save, CivilianIncidentSaveSlot, 0);
}

void UGTTCivilianIncidentDispatchSubsystem::LoadCheckpoint()
{
    if (!UGameplayStatics::DoesSaveGameExist(CivilianIncidentSaveSlot, 0))
    {
        return;
    }

    const UGTTCivilianIncidentSaveGame* Save = Cast<UGTTCivilianIncidentSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CivilianIncidentSaveSlot, 0));

    if (!Save ||
        Save->SchemaVersion != 1 ||
        !Save->bActive ||
        Save->IncidentId.IsNone())
    {
        ClearCheckpoint();
        return;
    }

    ActiveIncidentId = Save->IncidentId;
    ActiveLocation = Save->IncidentLocation;
    ActiveSeverity = FMath::Clamp(Save->Severity, 0.0f, 1.0f);

    // Helper identity and the 6-second work timer are intentionally not persisted.
    // A loaded in-progress assist safely resumes as an active dispatch and must be
    // started again after a compatible disabled actor is reacquired.
    State = EGTTCivilianIncidentDispatchState::Active;
    TrackedVehicle.Reset();
    OrphanSeconds = 0.0f;
    bRestoredFromCheckpoint = true;

    GTT_LOG( Log,
        TEXT("CIVILIAN_INCIDENT_DISPATCH_LOAD id=%s severity=%.2f location=%s assist_was_in_progress=%s"),
        *ActiveIncidentId.ToString(),
        ActiveSeverity,
        *ActiveLocation.ToCompactString(),
        Save->bAssistanceWasInProgress ? TEXT("YES") : TEXT("NO"));
}

void UGTTCivilianIncidentDispatchSubsystem::ClearCheckpoint() const
{
    if (UGameplayStatics::DoesSaveGameExist(CivilianIncidentSaveSlot, 0))
    {
        UGameplayStatics::DeleteGameInSlot(CivilianIncidentSaveSlot, 0);
    }
}

void UGTTCivilianIncidentDispatchSubsystem::EnsureWorldMarker()
{
    AGTTTrafficCarPawn* Vehicle = TrackedVehicle.Get();
    if (!Vehicle || MarkerComponent.IsValid())
    {
        return;
    }

    UTextRenderComponent* Marker = NewObject<UTextRenderComponent>(
        Vehicle,
        UTextRenderComponent::StaticClass(),
        TEXT("CivilianIncidentDispatchMarker"));
    if (!Marker)
    {
        return;
    }

    Marker->SetupAttachment(Vehicle->GetRootComponent());
    Marker->SetRelativeLocation(FVector(0.0f, 0.0f, 245.0f));
    Marker->SetText(NSLOCTEXT("GTT", "CivilianIncidentDispatchMarker", "ROADSIDE SOS"));
    Marker->SetWorldSize(42.0f);
    Marker->SetHorizontalAlignment(EHTA_Center);
    Marker->SetTextRenderColor(FColor(70, 215, 255));
    Marker->SetVisibility(true, true);
    Vehicle->AddInstanceComponent(Marker);
    Marker->RegisterComponent();
    MarkerComponent = Marker;
}

void UGTTCivilianIncidentDispatchSubsystem::DestroyWorldMarker()
{
    if (UTextRenderComponent* Marker = MarkerComponent.Get())
    {
        Marker->DestroyComponent();
    }
    MarkerComponent.Reset();
}

void UGTTCivilianIncidentDispatchSubsystem::NotifyPlayer(const FString& Message, float Duration) const
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
