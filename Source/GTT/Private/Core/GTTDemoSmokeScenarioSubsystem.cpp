#include "Core/GTTDemoSmokeScenarioSubsystem.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Missions/GTTMissionComponent.h"
#include "NPC/GTTCitizenPawn.h"
#include "Traffic/GTTTrafficDirector.h"
#include "UI/GTTGameHUD.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTPrototypeWorld.h"

void UGTTDemoSmokeScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled=FParse::Param(FCommandLine::Get(),TEXT("GTTDemoSmokeScenario"));
    if(bEnabled) UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_BEGIN version=1 mode=packaged-core"));
}

void UGTTDemoSmokeScenarioSubsystem::Pass(const TCHAR* Step)
{
    const FName Key(Step); if(Passed.Contains(Key)) return; Passed.Add(Key);
    UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_STEP step=%s result=PASS elapsed=%.2f"),Step,Elapsed);
}

void UGTTDemoSmokeScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed+=DeltaTime; UWorld* World=GetWorld(); if(!World) return;
    if(World->GetAuthGameMode<AGTTGameMode>() && World->GetSubsystem<UGTTDemoSmokeScenarioSubsystem>()) Pass(TEXT("WORLD"));
    if(APlayerController* PC=World->GetFirstPlayerController()) if(Cast<AGTTGameHUD>(PC->GetHUD())) Pass(TEXT("HUD"));
    if(World->GetAuthGameMode<AGTTGameMode>())
    {
        if(AGTTGameMode* GM=World->GetAuthGameMode<AGTTGameMode>())
        {
            if(UGTTMissionComponent* Mission=GM->GetMissionComponent()) if(!Mission->GetActiveMissionId().IsNone()) Pass(TEXT("MISSION"));
        }
    }
    for(TActorIterator<AGTTTrafficDirector> It(World);It;++It){Pass(TEXT("TRAFFIC"));break;}
    for(TActorIterator<AGTTCitizenPawn> It(World);It;++It){Pass(TEXT("NPC"));break;}
    if(APawn* Pawn=World->GetFirstPlayerController()?World->GetFirstPlayerController()->GetPawn():nullptr)
        if(UGTTGameplayStatics::FindWantedComponentForPawn(Pawn)) Pass(TEXT("WANTED"));

    if(Elapsed>=8.0f && !Passed.Contains(TEXT("SAVE")))
        if(AGTTGameMode* GM=World->GetAuthGameMode<AGTTGameMode>()) if(GM->SaveProgress()) Pass(TEXT("SAVE"));

    static const FName Required[]={TEXT("WORLD"),TEXT("HUD"),TEXT("TRAFFIC"),TEXT("NPC"),TEXT("MISSION"),TEXT("WANTED"),TEXT("SAVE")};
    bool All=true; for(const FName& Step:Required) All&=Passed.Contains(Step);
    if(All){UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_COMPLETE result=PASS steps=7 elapsed=%.2f"),Elapsed);bFinished=true;}
    else if(Elapsed>18.0f){FString Missing;for(const FName& Step:Required)if(!Passed.Contains(Step)){if(!Missing.IsEmpty())Missing+=TEXT(",");Missing+=Step.ToString();}UE_LOG(LogTemp,Error,TEXT("DEMO_SCENARIO_COMPLETE result=FAIL missing=%s elapsed=%.2f"),*Missing,Elapsed);bFinished=true;}
}
