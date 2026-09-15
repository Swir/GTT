#include "Core/GTTDemoSmokeScenarioSubsystem.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Combat/GTTCombatComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Missions/GTTMissionComponent.h"
#include "NPC/GTTCitizenPawn.h"
#include "Police/GTTPoliceDirector.h"
#include "Traffic/GTTTrafficDirector.h"
#include "UI/GTTGameHUD.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"

void UGTTDemoSmokeScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEnabled)
    {
        UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_BEGIN version=2 mode=vehicle-crime-route"));
    }
}

void UGTTDemoSmokeScenarioSubsystem::Pass(const TCHAR* Step)
{
    const FName Key(Step);
    if (Passed.Contains(Key)) return;
    Passed.Add(Key);
    UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_STEP step=%s result=PASS elapsed=%.2f"), Step, Elapsed);
}

void UGTTDemoSmokeScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    UWorld* World = GetWorld();
    if (!World) return;

    AGTTGameMode* GM = World->GetAuthGameMode<AGTTGameMode>();
    APlayerController* PC = World->GetFirstPlayerController();
    APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;

    if (GM) Pass(TEXT("WORLD"));
    if (PC && Cast<AGTTGameHUD>(PC->GetHUD())) Pass(TEXT("HUD"));
    if (GM)
    {
        if (UGTTMissionComponent* Mission = GM->GetMissionComponent())
        {
            if (!Mission->GetActiveMissionId().IsNone()) Pass(TEXT("MISSION"));
        }
    }
    for (TActorIterator<AGTTTrafficDirector> It(World); It; ++It) { Pass(TEXT("TRAFFIC")); break; }
    for (TActorIterator<AGTTCitizenPawn> It(World); It; ++It) { Pass(TEXT("NPC")); break; }

    if (PlayerPawn && PlayerPawn->FindComponentByClass<UGTTCombatComponent>()) Pass(TEXT("COMBAT"));

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeFieldmasterReady() && It->IsLegacyTakeoverActive()) Pass(TEXT("FIELDMASTER"));
        break;
    }
    for (TActorIterator<AGTTRattlebackNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady() && It->IsLegacyTakeoverActive()) Pass(TEXT("RATTLEBACK"));
        break;
    }
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady() && It->IsLegacyTakeoverActive()) Pass(TEXT("MULEBOX"));
        break;
    }

    UGTTWantedComponent* Wanted = PlayerPawn ? UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn) : nullptr;
    if (Wanted)
    {
        Pass(TEXT("WANTED_COMPONENT"));
        if (Elapsed >= 10.0f && !bCrimeInjected)
        {
            bCrimeInjected = true;
            Wanted->AddHeat(55.0f);
            UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_ACTION action=INJECT_TEST_CRIME heat=55.0"));
        }
        if (bCrimeInjected && Wanted->GetWantedLevel() > 0) Pass(TEXT("WANTED_ESCALATION"));
    }

    if (Passed.Contains(TEXT("WANTED_ESCALATION")))
    {
        for (TActorIterator<AGTTPoliceDirector> It(World); It; ++It)
        {
            Pass(TEXT("POLICE_RESPONSE"));
            break;
        }
    }

    if (Elapsed >= 14.0f && !Passed.Contains(TEXT("SAVE")) && GM)
    {
        if (GM->SaveProgress()) Pass(TEXT("SAVE"));
    }

    static const FName Required[] = {
        TEXT("WORLD"), TEXT("HUD"), TEXT("TRAFFIC"), TEXT("NPC"), TEXT("MISSION"), TEXT("COMBAT"),
        TEXT("FIELDMASTER"), TEXT("RATTLEBACK"), TEXT("MULEBOX"), TEXT("WANTED_COMPONENT"),
        TEXT("WANTED_ESCALATION"), TEXT("POLICE_RESPONSE"), TEXT("SAVE")
    };
    bool bAll = true;
    for (const FName& Step : Required) bAll &= Passed.Contains(Step);
    if (bAll)
    {
        UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_COMPLETE result=PASS steps=13 elapsed=%.2f"), Elapsed);
        bFinished = true;
    }
    else if (Elapsed > 28.0f)
    {
        FString Missing;
        for (const FName& Step : Required)
        {
            if (!Passed.Contains(Step))
            {
                if (!Missing.IsEmpty()) Missing += TEXT(",");
                Missing += Step.ToString();
            }
        }
        UE_LOG(LogTemp, Error, TEXT("DEMO_SCENARIO_COMPLETE result=FAIL missing=%s elapsed=%.2f"), *Missing, Elapsed);
        bFinished = true;
    }
}
