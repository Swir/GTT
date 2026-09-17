#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTDemoVisualEvidenceSubsystem.generated.h"

UCLASS()
class GTT_API UGTTDemoVisualEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    void RequestNextCapture();
    void PollPendingCapture();
    void FailCapture(const TCHAR* Reason);

    bool bEnabled = false;
    bool bFinished = false;
    float ElapsedSeconds = 0.0f;
    float PendingSinceSeconds = 0.0f;
    int32 NextSceneIndex = 0;
    FString EvidenceDirectory;
    FString PendingScene;
    FString PendingPath;
};
