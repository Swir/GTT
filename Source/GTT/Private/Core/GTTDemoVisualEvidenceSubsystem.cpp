#include "Core/GTTDemoVisualEvidenceSubsystem.h"
#include "GTT.h"

#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace GTTVisualEvidence
{
    struct FSceneSpec
    {
        const TCHAR* Id;
        float CaptureAtSeconds;
    };

    static const FSceneSpec Scenes[] =
    {
        { TEXT("world_gameplay"), 20.0f },
        { TEXT("law_pressure"), 68.0f },
        { TEXT("native_vehicle"), 116.0f },
        { TEXT("loaded_trailer"), 155.0f },
        { TEXT("hud_overview"), 174.0f },
    };

    static constexpr int32 SceneCount = UE_ARRAY_COUNT(Scenes);
    static constexpr float ScreenshotWriteTimeoutSeconds = 3.5f;
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

    UE_LOG(LogGTT, Display, TEXT("DEMO_VISUAL_CAPTURE_PLAN scenes=%d directory=\"%s\" show_ui=1 rendered_rhi_required=1"), GTTVisualEvidence::SceneCount, *EvidenceDirectory);
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
        UE_LOG(LogGTT, Display, TEXT("DEMO_VISUAL_CAPTURE_COMPLETE scenes=%d elapsed=%.2f directory=\"%s\""), GTTVisualEvidence::SceneCount, ElapsedSeconds, *EvidenceDirectory);
        bFinished = true;
        return;
    }

    if (ElapsedSeconds >= GTTVisualEvidence::Scenes[NextSceneIndex].CaptureAtSeconds)
    {
        RequestNextCapture();
    }
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

    UE_LOG(LogGTT, Display, TEXT("DEMO_VISUAL_CAPTURE_REQUEST scene=%s elapsed=%.2f file=\"%s\" show_ui=1"), *PendingScene, ElapsedSeconds, *PendingPath);
}

void UGTTDemoVisualEvidenceSubsystem::PollPendingCapture()
{
    const int64 Size = IFileManager::Get().FileSize(*PendingPath);
    if (Size > 0)
    {
        UE_LOG(LogGTT, Display, TEXT("DEMO_VISUAL_CAPTURE_WRITTEN scene=%s elapsed=%.2f bytes=%lld file=\"%s\""), *PendingScene, ElapsedSeconds, static_cast<long long>(Size), *PendingPath);
        ++NextSceneIndex;
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
    UE_LOG(LogGTT, Error, TEXT("DEMO_VISUAL_CAPTURE_FAIL reason=%s scene=%s elapsed=%.2f file=\"%s\""), Reason, PendingScene.IsEmpty() ? TEXT("none") : *PendingScene, ElapsedSeconds, PendingPath.IsEmpty() ? TEXT("") : *PendingPath);
    bFinished = true;
}
