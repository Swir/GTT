#include "Core/GTTDemoSmokeScenarioSubsystem.h"
#include "GTT.h"
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
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

namespace
{
bool HasLiveNativeMotion(AWheeledVehiclePawn* Pawn)
{
    if (!Pawn) return false;
    UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(Pawn->GetVehicleMovementComponent());
    if (!Movement || !Movement->IsActive() || Movement->GetNumWheels() < 4) return false;
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
    bool bApplied=false;
    if(AGTTFieldmasterNativePawn* Fieldmaster=Cast<AGTTFieldmasterNativePawn>(Pawn))bApplied=Fieldmaster->ApplyAcceptanceDriveCommand(Throttle,Steering,Brake);
    else if(AGTTRoadVehicleNativePawn* RoadVehicle=Cast<AGTTRoadVehicleNativePawn>(Pawn))bApplied=RoadVehicle->ApplyAcceptanceDriveCommand(Throttle,Steering,Brake);
    if(!bApplied)return false;
    if(HasLiveNativeMotion(Pawn)){GTT_LOG(Display,TEXT("DEMO_SCENARIO_CONTROL vehicle=%s throttle=%.2f steering=%.2f brake=%.2f speed_cm_s=%.1f"),VehicleId,Throttle,Steering,Brake,Pawn->GetVelocity().Size2D());return true;}return false;
}
}

void UGTTDemoSmokeScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);bEnabled=FParse::Param(FCommandLine::Get(),TEXT("GTTDemoSmokeScenario"));if(bEnabled)GTT_LOG(Display,TEXT("DEMO_SCENARIO_BEGIN version=8 mode=post-spike-escape-dynamics"));
}
void UGTTDemoSmokeScenarioSubsystem::Pass(const TCHAR* Step){const FName Key(Step);if(Passed.Contains(Key))return;Passed.Add(Key);GTT_LOG(Display,TEXT("DEMO_SCENARIO_STEP step=%s result=PASS elapsed=%.2f"),Step,Elapsed);}

void UGTTDemoSmokeScenarioSubsystem::PrepareAcceptanceFleet()
{
    if(bAcceptanceFleetPrepared)return;
    UWorld* World=GetWorld();if(!World)return;
    static const TSet<FName> RequiredVehicleIds={TEXT("RustyFieldmaster60"),TEXT("Rattleback82"),TEXT("Mulebox1200")};
    TSet<FName> PreparedIds;
    for(TActorIterator<AGTTVehicleBase> It(World);It;++It)
    {
        AGTTVehicleBase* Vehicle=*It;if(!Vehicle||!RequiredVehicleIds.Contains(Vehicle->GetPersistentVehicleId()))continue;
        if(!Vehicle->IsOwnedByPlayer())Vehicle->MarkOwnedByPlayer();
        Vehicle->RepairVehicle(100000.f);Vehicle->RefuelVehicle(100000.f);Vehicle->RepairTires();
        PreparedIds.Add(Vehicle->GetPersistentVehicleId());
    }
    if(PreparedIds.Num()==RequiredVehicleIds.Num())
    {
        for(TActorIterator<AGTTFieldmasterNativePawn> It(World);It;++It)
        {
            FGTTVehicleMigrationSnapshot State=It->GetMigrationSnapshot();State.ConditionPercent=1.f;State.FuelLiters=FMath::Max(State.FuelLiters,10.f);State.bOwnedByPlayer=true;State.TireIntegrity=1.f;It->ApplyMigrationSnapshot(State);
        }
        for(TActorIterator<AGTTRoadVehicleNativePawn> It(World);It;++It)
        {
            FGTTRoadVehicleMigrationSnapshot State=It->GetMigrationSnapshot();State.ConditionPercent=1.f;State.FuelLiters=FMath::Max(State.FuelLiters,10.f);State.bOwnedByPlayer=true;State.TireIntegrity=1.f;It->RestorePersistentMigrationSnapshot(State);It->RestorePersistentBodyDamage(FGTTRoadBodyDamageSnapshot(),0);
        }
        bAcceptanceFleetPrepared=true;
        GTT_LOG(Display,TEXT("DEMO_SCENARIO_FLEET_PREP result=PASS owned=RustyFieldmaster60,Rattleback82,Mulebox1200 condition=1.0 tires=1.0"));
    }
}

void UGTTDemoSmokeScenarioSubsystem::DriveNativeRoadblockCrossing()
{
    if(!Passed.Contains(TEXT("ROADBLOCK_ACTIVE"))||Passed.Contains(TEXT("POST_SPIKE_ESCAPE")))return;
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
        Vehicle->ApplyAcceptanceDriveCommand(0.85f,0.f,0.f);
        RoadblockBaselineTires=Vehicle->GetMigrationSnapshot().TireIntegrity;RoadblockBaselineWheelRisk=Vehicle->GetRuntimeWheelRisk();RoadblockBaselineThrottleLimit=Vehicle->GetRuntimeThrottleLimit();RoadblockBaselineSteeringLimit=Vehicle->GetRuntimeSteeringLimit();RoadblockCrossingStartSeconds=Elapsed;bRoadblockCrossingStaged=true;
        GTT_LOG(Display,TEXT("DEMO_SCENARIO_ROADBLOCK_CROSSING vehicle=%s phase=STAGED tire_before=%.3f wheel_risk_before=%.3f throttle_limit_before=%.3f steering_limit_before=%.3f distance_cm=900"),*Vehicle->GetPersistentVehicleId().ToString(),RoadblockBaselineTires,RoadblockBaselineWheelRisk,RoadblockBaselineThrottleLimit,RoadblockBaselineSteeringLimit);return;
    }
    Vehicle->ApplyAcceptanceDriveCommand(0.85f,0.f,0.f);
    if(Roadblock->HasProvenSpikeConsequence()&&Roadblock->GetLastSpikedVehicleId()==Vehicle->GetPersistentVehicleId())
    {
        if(!Passed.Contains(TEXT("ROADBLOCK_PHYSICAL_CROSSING"))){Pass(TEXT("ROADBLOCK_PHYSICAL_CROSSING"));GTT_LOG(Display,TEXT("DEMO_SCENARIO_ROADBLOCK_CROSSING vehicle=%s result=PASS roadblock_hits=%d"),*Vehicle->GetPersistentVehicleId().ToString(),Roadblock->GetSpikeHitCount());}
        const float TireAfter=Vehicle->GetMigrationSnapshot().TireIntegrity;const float RiskAfter=Vehicle->GetRuntimeWheelRisk();const float ThrottleAfter=Vehicle->GetRuntimeThrottleLimit();const float SteeringAfter=Vehicle->GetRuntimeSteeringLimit();
        if(!Passed.Contains(TEXT("HANDLING_CONSEQUENCE"))&&TireAfter<RoadblockBaselineTires&&RiskAfter>RoadblockBaselineWheelRisk+KINDA_SMALL_NUMBER&&(ThrottleAfter<RoadblockBaselineThrottleLimit-KINDA_SMALL_NUMBER||SteeringAfter<RoadblockBaselineSteeringLimit-KINDA_SMALL_NUMBER))
        {
            Pass(TEXT("HANDLING_CONSEQUENCE"));GTT_LOG(Display,TEXT("DEMO_SCENARIO_HANDLING_CONSEQUENCE vehicle=%s result=PASS tire_before=%.3f tire_after=%.3f wheel_risk_before=%.3f wheel_risk_after=%.3f throttle_limit_before=%.3f throttle_limit_after=%.3f steering_limit_before=%.3f steering_limit_after=%.3f"),*Vehicle->GetPersistentVehicleId().ToString(),RoadblockBaselineTires,TireAfter,RoadblockBaselineWheelRisk,RiskAfter,RoadblockBaselineThrottleLimit,ThrottleAfter,RoadblockBaselineSteeringLimit,SteeringAfter);
            bPostSpikeEscapeStarted=true;PostSpikeEscapeStartSeconds=Elapsed;PostSpikeStartSpeedCmS=Vehicle->GetVelocity().Size2D();
        }
        if(bPostSpikeEscapeStarted)
        {
            const float Phase=Elapsed-PostSpikeEscapeStartSeconds;Vehicle->ApplyAcceptanceDriveCommand(1.f,FMath::Sin(Phase*2.2f)*0.65f,0.f);
            if(Phase>=3.f&&HasLiveNativeMotion(Vehicle))
            {
                Pass(TEXT("POST_SPIKE_ESCAPE"));GTT_LOG(Display,TEXT("DEMO_SCENARIO_POST_SPIKE_ESCAPE vehicle=%s result=PASS duration=%.2f start_speed_cm_s=%.1f current_speed_cm_s=%.1f wheel_risk=%.3f throttle_limit=%.3f steering_limit=%.3f"),*Vehicle->GetPersistentVehicleId().ToString(),Phase,PostSpikeStartSpeedCmS,Vehicle->GetVelocity().Size2D(),Vehicle->GetRuntimeWheelRisk(),Vehicle->GetRuntimeThrottleLimit(),Vehicle->GetRuntimeSteeringLimit());Vehicle->ApplyAcceptanceDriveCommand(0.f,0.f,1.f);
            }
        }
    }
    else if(Elapsed-RoadblockCrossingStartSeconds>12.f){GTT_LOG(Error,TEXT("DEMO_SCENARIO_ROADBLOCK_CROSSING vehicle=%s result=TIMEOUT elapsed=%.2f"),*Vehicle->GetPersistentVehicleId().ToString(),Elapsed-RoadblockCrossingStartSeconds);}
}

void UGTTDemoSmokeScenarioSubsystem::Tick(float DeltaTime)
{
    Elapsed+=DeltaTime;UWorld* World=GetWorld();if(!World)return;PrepareAcceptanceFleet();AGTTGameMode* GM=World->GetAuthGameMode<AGTTGameMode>();APlayerController* PC=World->GetFirstPlayerController();APawn* PlayerPawn=PC?PC->GetPawn():nullptr;
    if(GM)Pass(TEXT("WORLD"));if(PC&&Cast<AGTTGameHUD>(PC->GetHUD()))Pass(TEXT("HUD"));if(GM)if(UGTTMissionComponent* Mission=GM->GetMissionComponent())if(!Mission->GetActiveMissionId().IsNone())Pass(TEXT("MISSION"));
    for(TActorIterator<AGTTTrafficDirector> It(World);It;++It){Pass(TEXT("TRAFFIC"));break;}for(TActorIterator<AGTTCitizenPawn> It(World);It;++It){Pass(TEXT("NPC"));break;}if(PlayerPawn&&PlayerPawn->FindComponentByClass<UGTTCombatComponent>())Pass(TEXT("COMBAT"));
    for(TActorIterator<AGTTFieldmasterNativePawn> It(World);It;++It){if(It->IsNativeFieldmasterReady()&&It->IsLegacyTakeoverActive())Pass(TEXT("FIELDMASTER"));if(HasLiveNativeMotion(*It))Pass(TEXT("FIELDMASTER_MOTION"));if(Elapsed>=4.f&&ExerciseNativeControls(*It,Elapsed,TEXT("Fieldmaster")))Pass(TEXT("FIELDMASTER_CONTROL"));break;}
    for(TActorIterator<AGTTRattlebackNativePawn> It(World);It;++It){if(It->IsNativeReady()&&It->IsLegacyTakeoverActive())Pass(TEXT("RATTLEBACK"));if(HasLiveNativeMotion(*It))Pass(TEXT("RATTLEBACK_MOTION"));if(Elapsed>=5.f&&ExerciseNativeControls(*It,Elapsed+1.f,TEXT("Rattleback82")))Pass(TEXT("RATTLEBACK_CONTROL"));break;}
    for(TActorIterator<AGTTMuleboxNativePawn> It(World);It;++It){if(It->IsNativeReady()&&It->IsLegacyTakeoverActive())Pass(TEXT("MULEBOX"));if(HasLiveNativeMotion(*It))Pass(TEXT("MULEBOX_MOTION"));if(Elapsed>=6.f&&ExerciseNativeControls(*It,Elapsed+2.f,TEXT("Mulebox1200")))Pass(TEXT("MULEBOX_CONTROL"));break;}
    UGTTWantedComponent* Wanted=PlayerPawn?UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn):nullptr;
    if(Wanted){Pass(TEXT("WANTED_COMPONENT"));if(Elapsed>=10.f&&!bCrimeInjected){bCrimeInjected=true;Wanted->AddHeat(130.f);GTT_LOG(Display,TEXT("DEMO_SCENARIO_ACTION action=INJECT_TEST_CRIME heat=130.0 target_wanted=4"));}if(bCrimeInjected&&Wanted->GetWantedLevel()>=4)Pass(TEXT("WANTED_ESCALATION"));}
    if(Passed.Contains(TEXT("WANTED_ESCALATION")))for(TActorIterator<AGTTPoliceDirector> It(World);It;++It){if(It->GetActiveFootUnitCount()>0)Pass(TEXT("POLICE_RESPONSE"));if(It->GetActivePursuitVehicleCount()>0)Pass(TEXT("PURSUIT_ACTIVE"));if(It->GetActiveRoadblockCount()>0){Pass(TEXT("ROADBLOCK_ACTIVE"));GTT_LOG(Display,TEXT("DEMO_SCENARIO_ROADBLOCK active=PASS count=%d"),It->GetActiveRoadblockCount());}if(It->IsRoadNodeInterceptionActive()){Pass(TEXT("INTERCEPTION_ACTIVE"));GTT_LOG(Display,TEXT("DEMO_SCENARIO_INTERCEPTION active=PASS node=%s"),*It->GetLastInterceptionNodeLabel());}break;}
    if(PlayerPawn&&Passed.Contains(TEXT("PURSUIT_ACTIVE")))for(TActorIterator<AGTTPolicePursuitVehicle> It(World);It;++It){const float Distance=FVector::Dist2D(It->GetActorLocation(),PlayerPawn->GetActorLocation());if(!ObservedPursuitVehicle.IsValid()){ObservedPursuitVehicle=*It;PursuitStartDistance=Distance;GTT_LOG(Display,TEXT("DEMO_SCENARIO_PURSUIT baseline_cm=%.1f tier=%d"),Distance,It->GetResponseTier());}if(ObservedPursuitVehicle.Get()==*It&&PursuitStartDistance>0.f&&Distance+250.f<PursuitStartDistance){Pass(TEXT("PURSUIT_CLOSING"));GTT_LOG(Display,TEXT("DEMO_SCENARIO_PURSUIT closing=PASS baseline_cm=%.1f current_cm=%.1f delta_cm=%.1f"),PursuitStartDistance,Distance,PursuitStartDistance-Distance);}break;}
    DriveNativeRoadblockCrossing();
    if(Elapsed>=30.f&&!Passed.Contains(TEXT("SAVE"))&&GM)if(GM->SaveProgress())Pass(TEXT("SAVE"));
    static const FName Required[]={TEXT("WORLD"),TEXT("HUD"),TEXT("TRAFFIC"),TEXT("NPC"),TEXT("MISSION"),TEXT("COMBAT"),TEXT("FIELDMASTER"),TEXT("FIELDMASTER_MOTION"),TEXT("FIELDMASTER_CONTROL"),TEXT("RATTLEBACK"),TEXT("RATTLEBACK_MOTION"),TEXT("RATTLEBACK_CONTROL"),TEXT("MULEBOX"),TEXT("MULEBOX_MOTION"),TEXT("MULEBOX_CONTROL"),TEXT("WANTED_COMPONENT"),TEXT("WANTED_ESCALATION"),TEXT("POLICE_RESPONSE"),TEXT("PURSUIT_ACTIVE"),TEXT("PURSUIT_CLOSING"),TEXT("ROADBLOCK_ACTIVE"),TEXT("INTERCEPTION_ACTIVE"),TEXT("ROADBLOCK_PHYSICAL_CROSSING"),TEXT("HANDLING_CONSEQUENCE"),TEXT("POST_SPIKE_ESCAPE"),TEXT("SAVE")};
    bool bAll=true;for(const FName& Step:Required)bAll&=Passed.Contains(Step);if(bAll){GTT_LOG(Display,TEXT("DEMO_SCENARIO_COMPLETE result=PASS steps=26 elapsed=%.2f"),Elapsed);bFinished=true;}else if(Elapsed>75.f){FString Missing;for(const FName& Step:Required)if(!Passed.Contains(Step)){if(!Missing.IsEmpty())Missing+=TEXT(",");Missing+=Step.ToString();}GTT_LOG(Error,TEXT("DEMO_SCENARIO_COMPLETE result=FAIL missing=%s elapsed=%.2f"),*Missing,Elapsed);bFinished=true;}
}
