#include "GTT.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY(LogGTT);

namespace
{
FString GRuntimeEvidenceFilename;
FCriticalSection GRuntimeEvidenceWriteMutex;

class FGTTGameModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();

        FString RuntimeLogPath = FPlatformMisc::GetEnvironmentVariable(TEXT("GTT_RUNTIME_EVIDENCE_LOG"));
        const bool bHasRuntimeEvidencePath = !RuntimeLogPath.IsEmpty() ||
            FParse::Value(FCommandLine::Get(), TEXT("GTTRuntimeEvidenceLog="), RuntimeLogPath) ||
            FParse::Value(FCommandLine::Get(), TEXT("abslog="), RuntimeLogPath);
        if (bHasRuntimeEvidencePath && !RuntimeLogPath.IsEmpty())
        {
            GRuntimeEvidenceFilename = FPaths::ConvertRelativePathToFull(RuntimeLogPath);
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(GRuntimeEvidenceFilename), true);
            FFileHelper::SaveStringToFile(TEXT(""), *GRuntimeEvidenceFilename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        }
    }

    virtual void ShutdownModule() override
    {
        GRuntimeEvidenceFilename.Reset();
        FDefaultGameModuleImpl::ShutdownModule();
    }
};
}

void GTTWriteRuntimeEvidence(const FString& Message)
{
    if (GRuntimeEvidenceFilename.IsEmpty())
    {
        return;
    }

    FScopeLock Lock(&GRuntimeEvidenceWriteMutex);
    FFileHelper::SaveStringToFile(
        Message + LINE_TERMINATOR,
        *GRuntimeEvidenceFilename,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),
        FILEWRITE_Append);
}

IMPLEMENT_PRIMARY_GAME_MODULE(FGTTGameModule, GTT, "GTT");
