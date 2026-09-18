#include "UI/GTTGameHUD.h"
#include "Activities/GTTBrawlDirector.h"
#include "Activities/GTTFarmCargoBreakdownRecoverySubsystem.h"
#include "Activities/GTTFarmJobDirector.h"
#include "Activities/GTTHeavyHaulDirector.h"
#include "Activities/GTTRoadRunDirector.h"
#include "Activities/GTTRuralWorkDirector.h"
#include "Combat/GTTCombatComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTArc3Director.h"
#include "Missions/GTTArc4Director.h"
#include "Missions/GTTMainStoryDirector.h"
#include "Missions/GTTMissionComponent.h"
#include "Missions/GTTNightFavorDirector.h"
#include "Police/GTTPoliceDirector.h"
#include "Radio/GTTRadioComponent.h"
#include "Ranger/GTTRangerRoadStopSubsystem.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTVillageEventDirector.h"

// Repository sanity compatibility vocabulary: POLICE RESPONSE, ROADBLOCKS, VEHICLE DYNAMICS, GetDynamicsSummary.
// These gameplay systems still exist; 0.0.38 intentionally stops rendering their raw debug telemetry permanently.

void AGTTGameHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!PlayerOwner || !Canvas || !GEngine) return;
    APawn* ControlledPawn = PlayerOwner->GetPawn();
    AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn);
    AGTTRoadVehicleNativePawn* NativeRoad = Cast<AGTTRoadVehicleNativePawn>(ControlledPawn);
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(ControlledPawn);
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(ControlledPawn);
    UGTTRadioComponent* Radio = UGTTGameplayStatics::FindRadioComponentForPawn(ControlledPawn);
    UGTTCombatComponent* Combat = ControlledPawn ? ControlledPawn->FindComponentByClass<UGTTCombatComponent>() : nullptr;
    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    AGTTPoliceDirector* PoliceDirector = Cast<AGTTPoliceDirector>(UGameplayStatics::GetActorOfClass(this, AGTTPoliceDirector::StaticClass()));
    const float LeftX = 34.0f;
    const float RightX = FMath::Max(520.0f, Canvas->ClipX - 570.0f);
    const float BottomY = FMath::Max(360.0f, Canvas->ClipY - 118.0f);
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    FString StatusLine;
    if (GameMode)
    {
        const FString TimeText = GameMode->GetDayNightCycle() ? GameMode->GetDayNightCycle()->GetClockText() : TEXT("DAY ? --:--");
        StatusLine = FString::Printf(TEXT("%s  |  GARAGE %d/%d"), *TimeText, GameMode->GetOwnedVehicleCount(), GameMode->GetGarageCapacity());
    }
    if (Economy) StatusLine += FString::Printf(TEXT("  |  $%d"), Economy->GetCash());
    if (!StatusLine.IsEmpty()) DrawHudText(StatusLine, FLinearColor(0.92f,0.90f,0.76f,1.0f), LeftX, 30.0f, 0.98f);
    float AlertY = 57.0f;
    if (WantedLevel > 0)
    {
        DrawHudText(BuildWantedBar(WantedLevel), FLinearColor(1.0f,0.18f,0.08f,1.0f), LeftX, AlertY, 1.18f);
        AlertY += 27.0f;
        if (PoliceDirector)
        {
            const FString Response = FString::Printf(TEXT("POLICE  |  FOOT %d  CARS %d  BLOCKS %d%s"), PoliceDirector->GetActiveFootUnitCount(), PoliceDirector->GetActivePursuitVehicleCount(), PoliceDirector->GetActiveRoadblockCount(), WantedLevel >= 4 ? TEXT("  |  INTERCEPT") : (WantedLevel >= 3 ? TEXT("  |  ESCALATING") : TEXT("")));
            DrawHudText(Response, WantedLevel >= 4 ? FLinearColor(1.0f,0.20f,0.10f,1.0f) : FLinearColor(0.95f,0.66f,0.24f,1.0f), LeftX, AlertY, 0.88f);
            AlertY += 24.0f;
        }
    }
    if (GameMode && GameMode->GetWildlifeAlertLevel() > 0)
    {
        FString Marks;
        const int32 RangerLevel = GameMode->GetWildlifeAlertLevel();
        for (int32 Index=0; Index<3; ++Index) Marks += Index < RangerLevel ? TEXT("!") : TEXT("-");
        DrawHudText(FString::Printf(TEXT("WARDEN [%s]"), *Marks), FLinearColor(1.0f,0.55f,0.12f,1.0f), LeftX, AlertY, 0.94f);
    }
    DrawRangerStopPanel();
    DrawFarmCargoRecoveryPanel(NativeRoad);
    const FString Objective = BuildPrimaryObjective();
    if (!Objective.IsEmpty())
    {
        DrawHudText(TEXT("CURRENT OBJECTIVE"), FLinearColor(0.52f,0.72f,0.95f,1.0f), RightX, 30.0f, 0.78f);
        DrawHudText(Objective, FLinearColor(1.0f,0.80f,0.20f,1.0f), RightX, 52.0f, 0.98f);
    }
    if (Economy && !Economy->GetActivityMessage().IsEmpty()) DrawHudText(Economy->GetActivityMessage(), FLinearColor(0.35f,0.88f,1.0f,1.0f), RightX, Objective.IsEmpty()?30.0f:81.0f, 0.86f);
    if (Vehicle)
    {
        DrawHudText(TEXT("VEHICLE"), FLinearColor(0.52f,0.72f,0.95f,1.0f), LeftX, BottomY-28.0f, 0.78f);
        DrawHudText(BuildVehicleStatus(Vehicle), FLinearColor::White, LeftX, BottomY-7.0f, 0.98f);
        const FString VehicleAlert = BuildVehicleAlert(Vehicle);
        if (!VehicleAlert.IsEmpty()) DrawHudText(VehicleAlert, FLinearColor(1.0f,0.34f,0.12f,1.0f), LeftX, BottomY+19.0f, 0.86f);
        if (Radio && Radio->IsRadioOn()) DrawHudText(Radio->GetDisplayLine(), FLinearColor(0.48f,0.88f,1.0f,1.0f), LeftX, BottomY+(VehicleAlert.IsEmpty()?19.0f:43.0f), 0.82f);
    }
    else if (NativeRoad)
    {
        DrawHudText(TEXT("VEHICLE"), FLinearColor(0.52f,0.72f,0.95f,1.0f), LeftX, BottomY-46.0f, 0.78f);
        DrawHudText(BuildNativeRoadStatus(NativeRoad), FLinearColor::White, LeftX, BottomY-25.0f, 0.94f);
        const FString RecoveryLine = BuildNativeRoadRecovery(NativeRoad);
        if (!RecoveryLine.IsEmpty()) DrawHudText(RecoveryLine, FLinearColor(1.0f,0.46f,0.14f,1.0f), LeftX, BottomY+1.0f, 0.84f);
        if (Radio && Radio->IsRadioOn()) DrawHudText(Radio->GetDisplayLine(), FLinearColor(0.48f,0.88f,1.0f,1.0f), LeftX, BottomY+(RecoveryLine.IsEmpty()?1.0f:25.0f), 0.82f);
    }
    else if (Combat)
    {
        const FLinearColor CombatColor = Combat->GetHealthPercent()<0.3f ? FLinearColor(1.0f,0.20f,0.10f,1.0f) : FLinearColor(1.0f,0.68f,0.22f,1.0f);
        DrawHudText(Combat->GetCombatStatusText(), CombatColor, LeftX, BottomY+8.0f, 0.90f);
    }
    const FString ContextHint = BuildContextHint(ControlledPawn, Vehicle, NativeRoad);
    if (!ContextHint.IsEmpty()) DrawHudText(ContextHint, FLinearColor(0.68f,0.78f,0.90f,1.0f), FMath::Max(LeftX,Canvas->ClipX-740.0f), Canvas->ClipY-42.0f, 0.76f);
}

void AGTTGameHUD::DrawHudText(const FString& Text,const FLinearColor& Color,float X,float Y,float Scale)
{
    if (Text.IsEmpty() || !GEngine) return;
    DrawText(Text,Color,X,Y,GEngine->GetSmallFont(),Scale,false);
}

void AGTTGameHUD::DrawRangerStopPanel()
{
    if (!Canvas || !GetWorld()) return;
    const UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>();
    if (!RoadStop) return;
    const FGTTRangerRoadStopPresentation Snapshot = RoadStop->GetPresentationSnapshot();
    if (!Snapshot.bVisible) return;
    FLinearColor Accent(1.0f, 0.60f, 0.12f, 1.0f);
    if (Snapshot.Phase == EGTTRangerRoadStopPhase::Search) Accent = FLinearColor(0.20f, 0.84f, 1.0f, 1.0f);
    else if (Snapshot.Phase == EGTTRangerRoadStopPhase::Flee) Accent = FLinearColor(1.0f, 0.18f, 0.08f, 1.0f);
    const float PanelWidth = FMath::Min(430.0f, Canvas->ClipX - 48.0f);
    const float PanelHeight = 76.0f;
    const float X = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Y = 30.0f;
    const float InnerWidth = FMath::Max(0.0f, PanelWidth - 36.0f);
    DrawRect(FLinearColor(0.01f, 0.025f, 0.04f, 0.90f), X, Y, PanelWidth, PanelHeight);
    DrawRect(Accent, X, Y, 4.0f, PanelHeight);
    DrawHudText(FString::Printf(TEXT("WARDEN STOP  |  %s"), *Snapshot.PhaseLabel), Accent, X + 16.0f, Y + 9.0f, 0.88f);
    DrawHudText(Snapshot.Instruction, FLinearColor(0.92f, 0.96f, 1.0f, 1.0f), X + 16.0f, Y + 33.0f, 0.76f);
    DrawRect(FLinearColor(0.12f, 0.17f, 0.21f, 0.96f), X + 16.0f, Y + 61.0f, InnerWidth, 4.0f);
    DrawRect(Accent, X + 16.0f, Y + 61.0f, InnerWidth * FMath::Clamp(Snapshot.Progress01, 0.0f, 1.0f), 4.0f);
}

void AGTTGameHUD::DrawFarmCargoRecoveryPanel(const AGTTRoadVehicleNativePawn* NativeRoad)
{
    if (!Canvas || !GetWorld() || !NativeRoad) return;
    const UGTTFarmCargoBreakdownRecoverySubsystem* Recovery = GetWorld()->GetSubsystem<UGTTFarmCargoBreakdownRecoverySubsystem>();
    if (!Recovery) return;
    const EGTTFarmCargoRecoveryState State = Recovery->GetRecoveryState();
    if (State == EGTTFarmCargoRecoveryState::None || State == EGTTFarmCargoRecoveryState::Healthy) return;
    if (Recovery->GetRecoveryVehicleId() != NativeRoad->GetPersistentVehicleId()) return;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    const FGTTBreakdownAssessment Assessment = Decision ? Decision->AssessVehicle(NativeRoad) : FGTTBreakdownAssessment();
    FString Instruction;
    FLinearColor Accent(1.0f, 0.58f, 0.14f, 1.0f);
    switch (State)
    {
        case EGTTFarmCargoRecoveryState::Degraded: Instruction = TEXT("Drive gently to finish the route or workshop; cargo clock and integrity remain live."); break;
        case EGTTFarmCargoRecoveryState::TowRecommended:
            Instruction = Assessment.bEmergencyPatchPossible ? FString::Printf(TEXT("Y / D-Pad Left PATCH $%d  |  T / D-Pad Up TOW $%d  |  contract clock running"), Assessment.EmergencyPatchEstimate, Assessment.TowEstimate) : FString::Printf(TEXT("T / D-Pad Up TOW $%d  |  structural/body damage blocks patch  |  clock running"), Assessment.TowEstimate);
            break;
        case EGTTFarmCargoRecoveryState::PatchPending: Instruction = TEXT("Emergency patch inbound. Same cargo vehicle only; contract clock keeps running."); Accent = FLinearColor(0.22f, 0.84f, 1.0f, 1.0f); break;
        case EGTTFarmCargoRecoveryState::Patched: Instruction = TEXT("Temporary limp-home patch active. Body damage remains; finish carefully or visit workshop."); Accent = FLinearColor(0.28f, 0.92f, 0.60f, 1.0f); break;
        case EGTTFarmCargoRecoveryState::TowPending: Instruction = TEXT("Tow inbound. Damage and exact cargo identity stay bound; contract clock keeps running."); Accent = FLinearColor(0.22f, 0.84f, 1.0f, 1.0f); break;
        case EGTTFarmCargoRecoveryState::PoliceImpoundPending: Instruction = TEXT("Police recovery controls this vehicle. Cargo cannot transfer to another vehicle."); Accent = FLinearColor(1.0f, 0.22f, 0.12f, 1.0f); break;
        case EGTTFarmCargoRecoveryState::AwaitingExactVehicle: Instruction = FString::Printf(TEXT("Recover exact vehicle %s. Another vehicle cannot deliver this load."), *Recovery->GetRecoveryVehicleId().ToString()); Accent = FLinearColor(1.0f, 0.22f, 0.12f, 1.0f); break;
        case EGTTFarmCargoRecoveryState::Recovered: Instruction = TEXT("Exact cargo vehicle recovered. Continue the route; no duplicate stock or payout."); Accent = FLinearColor(0.28f, 0.92f, 0.60f, 1.0f); break;
        default: return;
    }
    const float PanelWidth = FMath::Min(590.0f, Canvas->ClipX - 48.0f);
    const float PanelHeight = 72.0f;
    const float X = (Canvas->ClipX - PanelWidth) * 0.5f;
    const float Y = 116.0f;
    DrawRect(FLinearColor(0.01f, 0.025f, 0.04f, 0.91f), X, Y, PanelWidth, PanelHeight);
    DrawRect(Accent, X, Y, 4.0f, PanelHeight);
    DrawHudText(Recovery->GetRecoveryStateLabel(), Accent, X + 16.0f, Y + 9.0f, 0.84f);
    DrawHudText(Instruction, FLinearColor(0.92f, 0.96f, 1.0f, 1.0f), X + 16.0f, Y + 34.0f, 0.72f);
}

FString AGTTGameHUD::BuildWantedBar(int32 WantedLevel) const
{
    FString Stars; for (int32 Index=0; Index<5; ++Index) Stars += Index<WantedLevel?TEXT("*"):TEXT("-");
    return FString::Printf(TEXT("WANTED [%s]"),*Stars);
}

FString AGTTGameHUD::BuildPrimaryObjective() const
{
    if (const AGTTBrawlDirector* Brawl=Cast<AGTTBrawlDirector>(UGameplayStatics::GetActorOfClass(this,AGTTBrawlDirector::StaticClass()))) if (Brawl->IsBrawlActive()) return Brawl->GetObjectiveText();
    if (const AGTTHeavyHaulDirector* HeavyHaul=Cast<AGTTHeavyHaulDirector>(UGameplayStatics::GetActorOfClass(this,AGTTHeavyHaulDirector::StaticClass()))) if (HeavyHaul->IsActive()) return HeavyHaul->GetObjectiveText();
    if (const AGTTFarmJobDirector* Farm=Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(this,AGTTFarmJobDirector::StaticClass()))) if (Farm->IsJobActive()) return Farm->GetObjectiveText();
    if (const AGTTRoadRunDirector* RoadRun=Cast<AGTTRoadRunDirector>(UGameplayStatics::GetActorOfClass(this,AGTTRoadRunDirector::StaticClass()))) if (RoadRun->IsActive()) return RoadRun->GetObjectiveText();
    if (const AGTTRuralWorkDirector* Work=Cast<AGTTRuralWorkDirector>(UGameplayStatics::GetActorOfClass(this,AGTTRuralWorkDirector::StaticClass()))) if (Work->IsWorkActive()) return Work->GetObjectiveText();
    if (const AGTTNightFavorDirector* Favor=Cast<AGTTNightFavorDirector>(UGameplayStatics::GetActorOfClass(this,AGTTNightFavorDirector::StaticClass()))) if (Favor->IsActive()) return Favor->GetObjectiveText();
    const FString PrototypeMission=BuildMissionText(); if (!PrototypeMission.IsEmpty()) return PrototypeMission;
    const AGTTArc4Director* Arc4=Cast<AGTTArc4Director>(UGameplayStatics::GetActorOfClass(this,AGTTArc4Director::StaticClass()));
    if (Arc4 && Arc4->GetStage()!=EGTTArc4Stage::Locked && Arc4->GetStage()!=EGTTArc4Stage::Completed) return Arc4->GetObjectiveText();
    const AGTTArc3Director* Arc3=Cast<AGTTArc3Director>(UGameplayStatics::GetActorOfClass(this,AGTTArc3Director::StaticClass()));
    if (Arc3 && Arc3->GetStage()!=EGTTArc3Stage::Locked && Arc3->GetStage()!=EGTTArc3Stage::Completed) return Arc3->GetObjectiveText();
    const AGTTMainStoryDirector* Story=Cast<AGTTMainStoryDirector>(UGameplayStatics::GetActorOfClass(this,AGTTMainStoryDirector::StaticClass()));
    if (Story && Story->GetStage()!=EGTTMainStoryStage::Completed) return Story->GetObjectiveText();
    if (const AGTTVillageEventDirector* Nightlife=Cast<AGTTVillageEventDirector>(UGameplayStatics::GetActorOfClass(this,AGTTVillageEventDirector::StaticClass()))) if (Nightlife->IsNightlifeOpen()) return Nightlife->GetNightlifeSummary();
    return FString();
}

FString AGTTGameHUD::BuildContextHint(const APawn* ControlledPawn,const AGTTVehicleBase* Vehicle,const AGTTRoadVehicleNativePawn* NativeRoad) const
{
    if (Vehicle) return TEXT("E interact  |  F exit vehicle  |  R radio  |  F5 save  F9 load");
    if (NativeRoad) return TEXT("F exit  |  R radio  |  Y / D-Pad Left patch  |  T / D-Pad Up tow  |  F5 save  F9 load");
    if (ControlledPawn) return TEXT("E interact  |  LMB attack  |  Q next weapon  |  G drop");
    return FString();
}

FString AGTTGameHUD::BuildVehicleStatus(const AGTTVehicleBase* Vehicle) const
{
    if (!Vehicle) return FString();
    const TCHAR* Ownership=Vehicle->IsOwnedByPlayer()?TEXT("OWNED"):(Vehicle->WasReportedStolen()?TEXT("STOLEN"):TEXT("BORROWED"));
    return FString::Printf(TEXT("%s  |  %.0f km/h  |  FUEL %.0f%%  |  CONDITION %.0f%%  |  TIRES %.0f%%  |  %s"),*Vehicle->GetVehicleDisplayName().ToString(),Vehicle->GetSpeedKmh(),Vehicle->GetFuelPercent()*100.0f,Vehicle->GetConditionPercent()*100.0f,Vehicle->GetTireIntegrity()*100.0f,Ownership);
}

FString AGTTGameHUD::BuildVehicleAlert(const AGTTVehicleBase* Vehicle) const
{
    if (!Vehicle) return FString();
    const FString FaultText=Vehicle->GetFaultStatusText();
    if (!FaultText.IsEmpty()) return FString::Printf(TEXT("DAMAGE  |  %s  |  DETACHED %d"),*FaultText,Vehicle->GetDetachedPartCount());
    if (Vehicle->GetTireIntegrity()<0.30f) return TEXT("WARNING  |  TIRE INTEGRITY CRITICAL");
    if (Vehicle->GetFuelPercent()<0.15f) return TEXT("WARNING  |  LOW FUEL");
    if (Vehicle->GetEngineTemperatureC()>108.0f) return TEXT("WARNING  |  ENGINE TEMPERATURE HIGH");
    return FString();
}

FString AGTTGameHUD::BuildNativeRoadStatus(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return FString();
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    const float BodyHealth = FMath::Min(FMath::Min(Body.FrontHealth, Body.RearHealth), FMath::Min(Body.LeftHealth, Body.RightHealth));
    return FString::Printf(TEXT("%s  |  %.0f km/h  |  FUEL %.0f L  |  CONDITION %.0f%%  |  TIRES %.0f%%  |  BODY %.0f%%"), *Vehicle->GetVehicleDisplayName().ToString(), Vehicle->GetVelocity().Size()*0.036f, State.FuelLiters, State.ConditionPercent*100.0f, State.TireIntegrity*100.0f, BodyHealth*100.0f);
}

FString AGTTGameHUD::BuildNativeRoadRecovery(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !GetWorld()) return FString();
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    if (!Decision) return FString();
    const FGTTBreakdownAssessment Assessment = Decision->AssessVehicle(Vehicle);
    if (Assessment.Recommendation == EGTTBreakdownRecommendation::DriveNormally && Assessment.Severity < 0.20f) return FString();
    const TCHAR* Recommendation = TEXT("MONITOR");
    switch (Assessment.Recommendation)
    {
        case EGTTBreakdownRecommendation::LimpToWorkshop: Recommendation = TEXT("LIMP TO WORKSHOP"); break;
        case EGTTBreakdownRecommendation::TowRecommended: Recommendation = TEXT("RECOVERY RECOMMENDED"); break;
        case EGTTBreakdownRecommendation::Immobilized: Recommendation = TEXT("IMMOBILIZED"); break;
        default: break;
    }
    if (Assessment.bEmergencyPatchPossible)
    {
        return FString::Printf(TEXT("RECOVERY  |  %s  |  DAMAGE %.0f%%  |  PATCH $%d [Y]  |  TOW $%d [T]  |  REPAIR ~$%d"), Recommendation, Assessment.Severity*100.0f, Assessment.EmergencyPatchEstimate, Assessment.TowEstimate, Assessment.RepairEstimate);
    }
    return FString::Printf(TEXT("RECOVERY  |  %s  |  DAMAGE %.0f%%  |  TOW $%d [T]  |  REPAIR ~$%d  |  PATCH UNAVAILABLE"), Recommendation, Assessment.Severity*100.0f, Assessment.TowEstimate, Assessment.RepairEstimate);
}

FString AGTTGameHUD::BuildMissionText() const
{
    const AGTTGameMode* GameMode=Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const UGTTMissionComponent* Mission=GameMode?GameMode->GetMissionComponent():nullptr;
    if (!Mission) return FString();
    if (Mission->GetActiveMissionId()==FName(TEXT("BorrowedTractor")))
    {
        if (Mission->GetMissionState()==EGTTMissionState::Completed) return FString();
        if (Mission->GetMissionState()==EGTTMissionState::Failed) return TEXT("BORROWED TRACTOR | Mission failed");
        if (Mission->GetMissionState()==EGTTMissionState::Active)
        {
            if (Mission->GetMissionStage()==0) return TEXT("BORROWED TRACTOR | Reach the neighbour farm and take the tractor");
            if (Mission->GetMissionStage()==1) return TEXT("BORROWED TRACTOR | Lose wanted level, then reach the barn");
        }
    }
    return Mission->GetMissionState()==EGTTMissionState::Active?FString::Printf(TEXT("MISSION | %s"),*Mission->GetActiveMissionId().ToString()):FString();
}
