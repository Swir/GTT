#include "Core/GTTDemoVisualEvidenceSubsystem.h"
#include "GTT.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Police/GTTPolicePursuitVehicle.h"
#include "Police/GTTRoadblock.h"
#include "UnrealClient.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "World/GTTDayNightCycle.h"

namespace GTTVisualEvidence
{
    struct FSceneSpec
    {
        const TCHAR* Id;
        float CaptureAtSeconds;
    };

    static const FSceneSpec Scenes[] =
    {
        { TEXT("world_gameplay"), 8.0f },
        { TEXT("law_pressure"), 24.0f },
        { TEXT("native_vehicle"), 92.0f },
        { TEXT("loaded_trailer"), 145.0f },
        { TEXT("hud_overview"), 174.0f },
    };

    static constexpr int32 SceneCount = UE_ARRAY_COUNT(Scenes);
    static constexpr float ScreenshotWriteTimeoutSeconds = 3.5f;
    static constexpr float SceneSettleSeconds = 0.35f;
    static constexpr float SceneTargetResolveTimeoutSeconds = 8.0f;
}

void UGTTDemoVisualEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTVisualEvidence"));
    if (!bEnabled)
    {
        return;
    }

    if (!FParse::Value(FCommandLine::Get(), TEXT("GTTVisualEvidenceDir="), EvidenceDirectory))
    {
        EvidenceDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DemoVisualEvidence"));
    }

    EvidenceDirectory.TrimQuotesInline();
    EvidenceDirectory = FPaths::ConvertRelativePathToFull(EvidenceDirectory);

    if (!IFileManager::Get().MakeDirectory(*EvidenceDirectory, true) && !IFileManager::Get().DirectoryExists(*EvidenceDirectory))
    {
        FailCapture(TEXT("unable_to_create_evidence_directory"));
        return;
    }

    GTT_LOG( Display, TEXT("DEMO_VISUAL_CAPTURE_PLAN scenes=%d directory=\"%s\" show_ui=1 rendered_rhi_required=1"), GTTVisualEvidence::SceneCount, *EvidenceDirectory);
}

bool UGTTDemoVisualEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

TStatId UGTTDemoVisualEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTDemoVisualEvidenceSubsystem, STATGROUP_Tickables);
}

void UGTTDemoVisualEvidenceSubsystem::Tick(float DeltaTime)
{
    if (!IsTickable())
    {
        return;
    }

    ElapsedSeconds += FMath::Max(0.0f, DeltaTime);

    if (!PendingPath.IsEmpty())
    {
        PollPendingCapture();
        return;
    }

    if (NextSceneIndex >= GTTVisualEvidence::SceneCount)
    {
        GTT_LOG( Display, TEXT("DEMO_VISUAL_CAPTURE_COMPLETE scenes=%d elapsed=%.2f directory=\"%s\""), GTTVisualEvidence::SceneCount, ElapsedSeconds, *EvidenceDirectory);
        bFinished = true;
        return;
    }

    const GTTVisualEvidence::FSceneSpec& Scene = GTTVisualEvidence::Scenes[NextSceneIndex];
    if (ElapsedSeconds >= Scene.CaptureAtSeconds)
    {
        if (StagedSceneIndex != NextSceneIndex)
        {
            if (!StageScene(Scene.Id))
            {
                if (ElapsedSeconds - Scene.CaptureAtSeconds > GTTVisualEvidence::SceneTargetResolveTimeoutSeconds)
                {
                    FailCapture(TEXT("scene_target_unavailable"));
                }
                return;
            }

            StagedSceneIndex = NextSceneIndex;
            SceneStagedAtSeconds = ElapsedSeconds;
            return;
        }

        if (ElapsedSeconds - SceneStagedAtSeconds >= GTTVisualEvidence::SceneSettleSeconds)
        {
            RequestNextCapture();
        }
    }
}

bool UGTTDemoVisualEvidenceSubsystem::StageScene(const TCHAR* SceneId)
{
    UWorld* World = GetWorld();
    APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    if (!World || !Controller)
    {
        return false;
    }

    AActor* FocusActor = nullptr;
    FString TargetLabel;
    const FString Scene(SceneId);

    if (Scene == TEXT("world_gameplay"))
    {
        FocusActor = Controller->GetPawn();
        TargetLabel = TEXT("PLAYER");
    }
    else if (Scene == TEXT("law_pressure"))
    {
        for (TActorIterator<AGTTRoadblock> It(World); It; ++It)
        {
            FocusActor = *It;
            break;
        }
        if (!FocusActor)
        {
            for (TActorIterator<AGTTPolicePursuitVehicle> It(World); It; ++It)
            {
                FocusActor = *It;
                break;
            }
        }
        TargetLabel = TEXT("LAW");
    }
    else if (Scene == TEXT("native_vehicle") || Scene == TEXT("hud_overview"))
    {
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            if (It->IsNativeFieldmasterReady() && It->IsLegacyTakeoverActive())
            {
                FocusActor = *It;
                break;
            }
        }
        TargetLabel = Scene == TEXT("hud_overview") ? TEXT("FIELDMASTER_HUD") : TEXT("FIELDMASTER");
        if (Scene == TEXT("hud_overview") && FocusActor && Controller->GetPawn() != FocusActor)
        {
            Controller->Possess(CastChecked<APawn>(FocusActor));
        }
    }
    else if (Scene == TEXT("loaded_trailer"))
    {
        for (TActorIterator<AGTTFarmTrailer> It(World); It; ++It)
        {
            if (It->IsAttachedToNativeFieldmaster() && It->HasCargo())
            {
                FocusActor = *It;
                break;
            }
        }
        TargetLabel = TEXT("LOADED_TRAILER");
    }

    if (!FocusActor)
    {
        return false;
    }

    for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It)
    {
        It->RestoreTime(1, 10.5f);
        break;
    }

    if (!EvidenceCamera.IsValid())
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.ObjectFlags |= RF_Transient;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        EvidenceCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SpawnParameters);
    }
    ACameraActor* Camera = EvidenceCamera.Get();
    if (!Camera)
    {
        return false;
    }

    FVector Center = FocusActor->GetActorLocation();
    FVector Extent(200.0f, 200.0f, 150.0f);
    const FBox Bounds = FocusActor->GetComponentsBoundingBox(true);
    if (Bounds.IsValid)
    {
        Center = Bounds.GetCenter();
        Extent = Bounds.GetExtent();
    }

    const float SubjectRadius = FMath::Clamp(FMath::Max(Extent.Size2D(), Extent.Z), 150.0f, 900.0f);
    float Distance = FMath::Max(700.0f, SubjectRadius * 2.8f);
    float Height = FMath::Max(260.0f, SubjectRadius * 0.8f);
    if (Scene == TEXT("world_gameplay"))
    {
        Distance = FMath::Max(Distance, 900.0f);
        Height = FMath::Max(Height, 420.0f);
    }
    else if (Scene == TEXT("loaded_trailer"))
    {
        Distance = FMath::Max(Distance, 1050.0f);
        Height = FMath::Max(Height, 330.0f);
    }
    else if (Scene == TEXT("hud_overview"))
    {
        Distance = FMath::Max(Distance, 760.0f);
        Height = FMath::Max(Height, 280.0f);
    }

    const FVector Forward = FocusActor->GetActorForwardVector().GetSafeNormal();
    const FVector Right = FocusActor->GetActorRightVector().GetSafeNormal();
    const FVector CameraLocation = Center - Forward * Distance + Right * Distance * 0.38f + FVector::UpVector * Height;
    const FRotator CameraRotation = (Center + FVector::UpVector * FMath::Min(120.0f, Extent.Z * 0.25f) - CameraLocation).Rotation();
    Camera->SetActorLocationAndRotation(CameraLocation, CameraRotation, false, nullptr, ETeleportType::TeleportPhysics);
    if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
    {
        CameraComponent->SetFieldOfView(Scene == TEXT("world_gameplay") ? 68.0f : 58.0f);
    }
    Controller->SetViewTarget(Camera);

    GTT_LOG(Display, TEXT("DEMO_VISUAL_SCENE_STAGED scene=%s target=%s focus_actor=%s daylight=YES"),
        SceneId, *TargetLabel, *FocusActor->GetName());
    return true;
}

void UGTTDemoVisualEvidenceSubsystem::RequestNextCapture()
{
    if (NextSceneIndex < 0 || NextSceneIndex >= GTTVisualEvidence::SceneCount)
    {
        FailCapture(TEXT("invalid_scene_index"));
        return;
    }

    const GTTVisualEvidence::FSceneSpec& Scene = GTTVisualEvidence::Scenes[NextSceneIndex];
    PendingScene = Scene.Id;
    PendingPath = FPaths::Combine(EvidenceDirectory, FString::Printf(TEXT("GTT_visual_%s.png"), Scene.Id));
    PendingSinceSeconds = ElapsedSeconds;

    IFileManager::Get().Delete(*PendingPath, false, true, true);
    FScreenshotRequest::RequestScreenshot(PendingPath, true, false, false, FIntRect(), true);

    GTT_LOG( Display, TEXT("DEMO_VISUAL_CAPTURE_REQUEST scene=%s elapsed=%.2f file=\"%s\" show_ui=1"), *PendingScene, ElapsedSeconds, *PendingPath);
}

void UGTTDemoVisualEvidenceSubsystem::PollPendingCapture()
{
    const int64 Size = IFileManager::Get().FileSize(*PendingPath);
    if (Size > 0)
    {
        GTT_LOG( Display, TEXT("DEMO_VISUAL_CAPTURE_WRITTEN scene=%s elapsed=%.2f bytes=%lld file=\"%s\""), *PendingScene, ElapsedSeconds, static_cast<long long>(Size), *PendingPath);
        ++NextSceneIndex;
        StagedSceneIndex = INDEX_NONE;
        SceneStagedAtSeconds = 0.0f;
        PendingScene.Reset();
        PendingPath.Reset();
        PendingSinceSeconds = 0.0f;
        return;
    }

    if ((ElapsedSeconds - PendingSinceSeconds) > GTTVisualEvidence::ScreenshotWriteTimeoutSeconds)
    {
        FailCapture(TEXT("screenshot_write_timeout"));
    }
}

void UGTTDemoVisualEvidenceSubsystem::FailCapture(const TCHAR* Reason)
{
    GTT_LOG( Error, TEXT("DEMO_VISUAL_CAPTURE_FAIL reason=%s scene=%s elapsed=%.2f file=\"%s\""), Reason, PendingScene.IsEmpty() ? TEXT("none") : *PendingScene, ElapsedSeconds, PendingPath.IsEmpty() ? TEXT("") : *PendingPath);
    bFinished = true;
}
