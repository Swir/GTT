#include "Core/GTTDemoSmokeScenarioSubsystem.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Combat/GTTCombatComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
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

namespace
{
bool HasLiveNativeMotion(AWheeledVehiclePawn* Pawn)
{
    if (!Pawn) return false;
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(Pawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive()) return false;
    int32 Valid = 0, Contacts = 0, Suspension = 0;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FWheelStatus Wheel = Movement->GetWheelState(Index);
        if (!Wheel.bIsValid) continue;
        ++Valid;
        if (Wheel.bInContact) ++Contacts;
        if (FMath::IsFinite(Wheel.NormalizedSuspensionLength) && Wheel.NormalizedSuspensionLength >= 0.f && Wheel.NormalizedSuspensionLength <= 1.f) ++Suspension;
    }
    return Valid == 4 && Contacts >= 2 && Suspension == 4 && Pawn->GetVelocity().SizeSquared2D() > FMath::Square(10.f);
}
}

void UGTTDemoSmokeScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"));
    if (bEnabled) UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_BEGIN version=3 mode=native-motion-pursuit-route"));
}

void UGTTDemoSmokeScenarioSubsystem::Pass(const TCHAR* Step)
{
    const FName Key(Step); if (Passed.Contains(Key)) return; Passed.Add(Key);
    UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_STEP step=%s result=PASS elapsed=%.2f"), Step, Elapsed);
}

void UGTTDemoSmokeScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime; UWorld* World = GetWorld(); if (!World) return;
    AGTTGameMode* GM = World->GetAuthGameMode<AGTTGameMode>();
    APlayerController* PC = World->GetFirstPlayerController(); APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
    if (GM) Pass(TEXT("WORLD")); if (PC && Cast<AGTTGameHUD>(PC->GetHUD())) Pass(TEXT("HUD"));
    if (GM) if (UGTTMissionComponent* Mission = GM->GetMissionComponent()) if (!Mission->GetActiveMissionId().IsNone()) Pass(TEXT("MISSION"));
    for (TActorIterator<AGTTTrafficDirector> It(World); It; ++It) { Pass(TEXT("TRAFFIC")); break; }
    for (TActorIterator<AGTTCitizenPawn> It(World); It; ++It) { Pass(TEXT("NPC")); break; }
    if (PlayerPawn && PlayerPawn->FindComponentByClass<UGTTCombatComponent>()) Pass(TEXT("COMBAT"));

    for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It) { if (It->IsNativeFieldmasterReady() && It->IsLegacyTakeoverActive()) Pass(TEXT("FIELDMASTER")); if (HasLiveNativeMotion(*It)) Pass(TEXT("FIELDMASTER_MOTION")); break; }
    for (TActorIterator<AGTTRattlebackNativePawn> It(World); It; ++It) { if (It->IsNativeReady() && It->IsLegacyTakeoverActive()) Pass(TEXT("RATTLEBACK")); if (HasLiveNativeMotion(*It)) Pass(TEXT("RATTLEBACK_MOTION")); break; }
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It) { if (It->IsNativeReady() && It->IsLegacyTakeoverActive()) Pass(TEXT("MULEBOX")); if (HasLiveNativeMotion(*It)) Pass(TEXT("MULEBOX_MOTION")); break; }

    UGTTWantedComponent* Wanted = PlayerPawn ? UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn) : nullptr;
    if (Wanted)
    {
        Pass(TEXT("WANTED_COMPONENT"));
        if (Elapsed >= 10.f && !bCrimeInjected) { bCrimeInjected = true; Wanted->AddHeat(80.f); UE_LOG(LogTemp, Display, TEXT("DEMO_SCENARIO_ACTION action=INJECT_TEST_CRIME heat=80.0 target_wanted=3")); }
        if (bCrimeInjected && Wanted->GetWantedLevel() >= 3) Pass(TEXT("WANTED_ESCALATION"));
    }
    if (Passed.Contains(TEXT("WANTED_ESCALATION"))) for (TActorIterator<AGTTPoliceDirector> It(World); It; ++It)
    {
        if (It->GetActiveFootUnitCount() > 0) Pass(TEXT("POLICE_RESPONSE"));
        if (It->GetActivePursuitVehicleCount() > 0) Pass(TEXT("PURSUIT_ACTIVE"));
        break;
    }
    if (Elapsed >= 24.f && !Passed.Contains(TEXT("SAVE")) && GM) if (GM->SaveProgress()) Pass(TEXT("SAVE"));

    static const FName Required[] = { TEXT("WORLD"),TEXT("HUD"),TEXT("TRAFFIC"),TEXT("NPC"),TEXT("MISSION"),TEXT("COMBAT"),TEXT("FIELDMASTER"),TEXT("FIELDMASTER_MOTION"),TEXT("RATTLEBACK"),TEXT("RATTLEBACK_MOTION"),TEXT("MULEBOX"),TEXT("MULEBOX_MOTION"),TEXT("WANTED_COMPONENT"),TEXT("WANTED_ESCALATION"),TEXT("POLICE_RESPONSE"),TEXT("PURSUIT_ACTIVE"),TEXT("SAVE") };
    bool bAll=true; for(const FName& Step:Required) bAll &= Passed.Contains(Step);
    if(bAll){ UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_COMPLETE result=PASS steps=17 elapsed=%.2f"),Elapsed); bFinished=true; }
    else if(Elapsed>45.f){ FString Missing; for(const FName& Step:Required) if(!Passed.Contains(Step)){ if(!Missing.IsEmpty()) Missing+=TEXT(","); Missing+=Step.ToString(); } UE_LOG(LogTemp,Error,TEXT("DEMO_SCENARIO_COMPLETE result=FAIL missing=%s elapsed=%.2f"),*Missing,Elapsed); bFinished=true; }
}
