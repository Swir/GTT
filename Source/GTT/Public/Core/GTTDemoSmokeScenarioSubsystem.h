#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTDemoSmokeScenarioSubsystem.generated.h"

UCLASS()
class GTT_API UGTTDemoSmokeScenarioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTDemoSmokeScenarioSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return bEnabled && !bFinished; }
private:
    void Pass(const TCHAR* Step);
    bool bEnabled=false;
    bool bFinished=false;
    float Elapsed=0.0f;
    TSet<FName> Passed;
};
