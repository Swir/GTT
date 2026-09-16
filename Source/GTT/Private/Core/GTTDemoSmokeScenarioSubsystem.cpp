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
#include "Police/GTTPolicePursuitVehicle.h"
#include "Police/GTTRoadblock.h"
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
    int32 Valid=0, Contacts=0, Suspension=0;
    for(int32 Index=0;Index<4;++Index){const FWheelStatus Wheel=Movement->GetWheelState(Index);if(!Wheel.bIsValid)continue;++Valid;if(Wheel.bInContact)++Contacts;if(FMath::IsFinite(Wheel.NormalizedSuspensionLength)&&Wheel.NormalizedSuspensionLength>=0.f&&Wheel.NormalizedSuspensionLength<=1.f)++Suspension;}
    return Valid==4 && Contacts>=2 && Suspension==4 && Pawn->GetVelocity().SizeSquared2D()>FMath::Square(10.f);
}

bool ExerciseNativeControls(AWheeledVehiclePawn* Pawn,float Elapsed,const TCHAR* VehicleId)
{
    if(!Pawn) return false;
    UChaosWheeledVehicleMovementComponent* Movement=Cast<UChaosWheeledVehicleMovementComponent>(Pawn->GetVehicleMovementComponent());
    if(!Movement||!Movement->IsActive()) return false;
    const float Phase=FMath::Fmod(Elapsed,6.f);const float Throttle=Phase<4.5f?0.72f:0.f;const float Steering=Phase<2.f?0.35f:(Phase<4.f?-0.35f:0.f);const float Brake=Phase>=4.5f?0.65f:0.f;
    Movement->SetThrottleInput(Throttle);Movement->SetSteeringInput(Steering);Movement->SetBrakeInput(Brake);
    if(HasLiveNativeMotion(Pawn)){UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_CONTROL vehicle=%s throttle=%.2f steering=%.2f brake=%.2f speed_cm_s=%.1f"),VehicleId,Throttle,Steering,Brake,Pawn->GetVelocity().Size2D());return true;}return false;
}
}

void UGTTDemoSmokeScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);bEnabled=FParse::Param(FCommandLine::Get(),TEXT("GTTDemoSmokeScenario"));if(bEnabled)UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_BEGIN version=7 mode=physical-native-roadblock-crossing"));
}
void UGTTDemoSmokeScenarioSubsystem::Pass(const TCHAR* Step){const FName Key(Step);if(Passed.Contains(Key))return;Passed.Add(Key);UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_STEP step=%s result=PASS elapsed=%.2f"),Step,Elapsed);}

void UGTTDemoSmokeScenarioSubsystem::DriveNativeRoadblockCrossing()
{
    if(!Passed.Contains(TEXT("ROADBLOCK_ACTIVE"))||Passed.Contains(TEXT("HANDLING_CONSEQUENCE")))return;
    UWorld* World=GetWorld();if(!World)return;
    if(!RoadblockTestActor.IsValid()){for(TActorIterator<AGTTRoadblock> It(World);It;++It){RoadblockTestActor=*It;break;}}
    if(!RoadblockTestVehicle.IsValid())
    {
        for(TActorIterator<AGTTRattlebackNativePawn> It(World);It;++It){if(It->IsNativeReady()&&It->IsLegacyTakeoverActive()){RoadblockTestVehicle=*It;break;}}
        if(!RoadblockTestVehicle.IsValid())for(TActorIterator<AGTTMuleboxNativePawn> It(World);It;++It){if(It->IsNativeReady()&&It->IsLegacyTakeoverActive()){RoadblockTestVehicle=*It;break;}}
    }
    AGTTRoadblock* Roadblock=RoadblockTestActor.Get();AGTTRoadVehicleNativePawn* Vehicle=RoadblockTestVehicle.Get();if(!Roadblock||!Vehicle)return;
    UChaosWheeledVehicleMovementComponent* Movement=Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent());if(!Movement||!Movement->IsActive())return;
    if(!bRoadblockCrossingStaged)
    {
        const FVector Approach=Roadblock->GetSpikeApproachDirection();const FVector Spike=Roadblock->GetSpikeStripWorldLocation();const FVector Stage=Spike-Approach*900.f+FVector(0,0,95.f);
        Vehicle->SetActorLocation(Stage,false,nullptr,ETeleportType::TeleportPhysics);Vehicle->SetActorRotation(Approach.Rotation(),ETeleportType::TeleportPhysics);
        Movement->SetBrakeInput(0.f);Movement->SetSteeringInput(0.f);Movement->SetThrottleInput(0.85f);
        RoadblockBaselineTires=Vehicle->GetMigrationSnapshot().TireIntegrity;RoadblockBaselineWheelRisk=Vehicle->GetRuntimeWheelRisk();RoadblockCrossingStartSeconds=Elapsed;bRoadblockCrossingStaged=true;
        UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_ROADBLOCK_CROSSING vehicle=%s phase=STAGED tire_before=%.3f wheel_risk_before=%.3f distance_cm=900"),*Vehicle->GetPersistentVehicleId().ToString(),RoadblockBaselineTires,RoadblockBaselineWheelRisk);return;
    }
    Movement->SetBrakeInput(0.f);Movement->SetSteeringInput(0.f);Movement->SetThrottleInput(0.85f);
    if(Roadblock->HasProvenSpikeConsequence()&&Roadblock->GetLastSpikedVehicleId()==Vehicle->GetPersistentVehicleId())
    {
        Pass(TEXT("ROADBLOCK_PHYSICAL_CROSSING"));UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_ROADBLOCK_CROSSING vehicle=%s result=PASS roadblock_hits=%d"),*Vehicle->GetPersistentVehicleId().ToString(),Roadblock->GetSpikeHitCount());
        const float TireAfter=Vehicle->GetMigrationSnapshot().TireIntegrity;const float RiskAfter=Vehicle->GetRuntimeWheelRisk();
        if(TireAfter<RoadblockBaselineTires&&RiskAfter>RoadblockBaselineWheelRisk+KINDA_SMALL_NUMBER){Pass(TEXT("HANDLING_CONSEQUENCE"));UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_HANDLING_CONSEQUENCE vehicle=%s result=PASS tire_before=%.3f tire_after=%.3f wheel_risk_before=%.3f wheel_risk_after=%.3f"),*Vehicle->GetPersistentVehicleId().ToString(),RoadblockBaselineTires,TireAfter,RoadblockBaselineWheelRisk,RiskAfter);Movement->SetThrottleInput(0.f);Movement->SetBrakeInput(1.f);}
    }
    else if(Elapsed-RoadblockCrossingStartSeconds>12.f){UE_LOG(LogTemp,Error,TEXT("DEMO_SCENARIO_ROADBLOCK_CROSSING vehicle=%s result=TIMEOUT elapsed=%.2f"),*Vehicle->GetPersistentVehicleId().ToString(),Elapsed-RoadblockCrossingStartSeconds);}
}

void UGTTDemoSmokeScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed+=DeltaTime;UWorld* World=GetWorld();if(!World)return;AGTTGameMode* GM=World->GetAuthGameMode<AGTTGameMode>();APlayerController* PC=World->GetFirstPlayerController();APawn* PlayerPawn=PC?PC->GetPawn():nullptr;
    if(GM)Pass(TEXT("WORLD"));if(PC&&Cast<AGTTGameHUD>(PC->GetHUD()))Pass(TEXT("HUD"));if(GM)if(UGTTMissionComponent* Mission=GM->GetMissionComponent())if(!Mission->GetActiveMissionId().IsNone())Pass(TEXT("MISSION"));
    for(TActorIterator<AGTTTrafficDirector> It(World);It;++It){Pass(TEXT("TRAFFIC"));break;}for(TActorIterator<AGTTCitizenPawn> It(World);It;++It){Pass(TEXT("NPC"));break;}if(PlayerPawn&&PlayerPawn->FindComponentByClass<UGTTCombatComponent>())Pass(TEXT("COMBAT"));
    for(TActorIterator<AGTTFieldmasterNativePawn> It(World);It;++It){if(It->IsNativeFieldmasterReady()&&It->IsLegacyTakeoverActive())Pass(TEXT("FIELDMASTER"));if(HasLiveNativeMotion(*It))Pass(TEXT("FIELDMASTER_MOTION"));if(Elapsed>=4.f&&ExerciseNativeControls(*It,Elapsed,TEXT("Fieldmaster")))Pass(TEXT("FIELDMASTER_CONTROL"));break;}
    for(TActorIterator<AGTTRattlebackNativePawn> It(World);It;++It){if(It->IsNativeReady()&&It->IsLegacyTakeoverActive())Pass(TEXT("RATTLEBACK"));if(HasLiveNativeMotion(*It))Pass(TEXT("RATTLEBACK_MOTION"));if(Elapsed>=5.f&&ExerciseNativeControls(*It,Elapsed+1.f,TEXT("Rattleback82")))Pass(TEXT("RATTLEBACK_CONTROL"));break;}
    for(TActorIterator<AGTTMuleboxNativePawn> It(World);It;++It){if(It->IsNativeReady()&&It->IsLegacyTakeoverActive())Pass(TEXT("MULEBOX"));if(HasLiveNativeMotion(*It))Pass(TEXT("MULEBOX_MOTION"));if(Elapsed>=6.f&&ExerciseNativeControls(*It,Elapsed+2.f,TEXT("Mulebox1200")))Pass(TEXT("MULEBOX_CONTROL"));break;}
    UGTTWantedComponent* Wanted=PlayerPawn?UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn):nullptr;
    if(Wanted){Pass(TEXT("WANTED_COMPONENT"));if(Elapsed>=10.f&&!bCrimeInjected){bCrimeInjected=true;Wanted->AddHeat(130.f);UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_ACTION action=INJECT_TEST_CRIME heat=130.0 target_wanted=4"));}if(bCrimeInjected&&Wanted->GetWantedLevel()>=4)Pass(TEXT("WANTED_ESCALATION"));}
    if(Passed.Contains(TEXT("WANTED_ESCALATION")))for(TActorIterator<AGTTPoliceDirector> It(World);It;++It){if(It->GetActiveFootUnitCount()>0)Pass(TEXT("POLICE_RESPONSE"));if(It->GetActivePursuitVehicleCount()>0)Pass(TEXT("PURSUIT_ACTIVE"));if(It->GetActiveRoadblockCount()>0){Pass(TEXT("ROADBLOCK_ACTIVE"));UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_ROADBLOCK active=PASS count=%d"),It->GetActiveRoadblockCount());}if(It->IsRoadNodeInterceptionActive()){Pass(TEXT("INTERCEPTION_ACTIVE"));UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_INTERCEPTION active=PASS node=%s"),*It->GetLastInterceptionNodeLabel());}break;}
    if(PlayerPawn&&Passed.Contains(TEXT("PURSUIT_ACTIVE")))for(TActorIterator<AGTTPolicePursuitVehicle> It(World);It;++It){const float Distance=FVector::Dist2D(It->GetActorLocation(),PlayerPawn->GetActorLocation());if(!ObservedPursuitVehicle.IsValid()){ObservedPursuitVehicle=*It;PursuitStartDistance=Distance;UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_PURSUIT baseline_cm=%.1f tier=%d"),Distance,It->GetResponseTier());}if(ObservedPursuitVehicle.Get()==*It&&PursuitStartDistance>0.f&&Distance+250.f<PursuitStartDistance){Pass(TEXT("PURSUIT_CLOSING"));UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_PURSUIT closing=PASS baseline_cm=%.1f current_cm=%.1f delta_cm=%.1f"),PursuitStartDistance,Distance,PursuitStartDistance-Distance);}break;}
    DriveNativeRoadblockCrossing();
    if(Elapsed>=30.f&&!Passed.Contains(TEXT("SAVE"))&&GM)if(GM->SaveProgress())Pass(TEXT("SAVE"));
    static const FName Required[]={TEXT("WORLD"),TEXT("HUD"),TEXT("TRAFFIC"),TEXT("NPC"),TEXT("MISSION"),TEXT("COMBAT"),TEXT("FIELDMASTER"),TEXT("FIELDMASTER_MOTION"),TEXT("FIELDMASTER_CONTROL"),TEXT("RATTLEBACK"),TEXT("RATTLEBACK_MOTION"),TEXT("RATTLEBACK_CONTROL"),TEXT("MULEBOX"),TEXT("MULEBOX_MOTION"),TEXT("MULEBOX_CONTROL"),TEXT("WANTED_COMPONENT"),TEXT("WANTED_ESCALATION"),TEXT("POLICE_RESPONSE"),TEXT("PURSUIT_ACTIVE"),TEXT("PURSUIT_CLOSING"),TEXT("ROADBLOCK_ACTIVE"),TEXT("INTERCEPTION_ACTIVE"),TEXT("ROADBLOCK_PHYSICAL_CROSSING"),TEXT("HANDLING_CONSEQUENCE"),TEXT("SAVE")};
    bool bAll=true;for(const FName& Step:Required)bAll&=Passed.Contains(Step);if(bAll){UE_LOG(LogTemp,Display,TEXT("DEMO_SCENARIO_COMPLETE result=PASS steps=25 elapsed=%.2f"),Elapsed);bFinished=true;}else if(Elapsed>75.f){FString Missing;for(const FName& Step:Required)if(!Passed.Contains(Step)){if(!Missing.IsEmpty())Missing+=TEXT(",");Missing+=Step.ToString();}UE_LOG(LogTemp,Error,TEXT("DEMO_SCENARIO_COMPLETE result=FAIL missing=%s elapsed=%.2f"),*Missing,Elapsed);bFinished=true;}
}
