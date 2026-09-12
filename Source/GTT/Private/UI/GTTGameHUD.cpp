#include "UI/GTTGameHUD.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMissionComponent.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"

void AGTTGameHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!PlayerOwner) return;

    APawn* ControlledPawn = PlayerOwner->GetPawn();
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(ControlledPawn);
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(ControlledPawn);
    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));

    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;
    const FLinearColor WantedColor = WantedLevel > 0 ? FLinearColor(1.0f,0.18f,0.08f,1.0f) : FLinearColor(0.72f,0.72f,0.72f,1.0f);
    DrawText(BuildWantedBar(WantedLevel), WantedColor, 36.0f, 34.0f, GEngine->GetSmallFont(), 1.35f, false);

    float Y = 64.0f;
    if (GameMode)
    {
        FString WardenMarks;
        const int32 RangerLevel = GameMode->GetWildlifeAlertLevel();
        for (int32 Index=0; Index<3; ++Index) WardenMarks += Index < RangerLevel ? TEXT("!") : TEXT("-");
        DrawText(FString::Printf(TEXT("WARDEN [%s]"), *WardenMarks), RangerLevel > 0 ? FLinearColor(1.0f,0.55f,0.12f,1.0f) : FLinearColor(0.55f,0.7f,0.55f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 1.0f, false);
        Y += 28.0f;
    }

    if (Economy)
    {
        DrawText(FString::Printf(TEXT("CASH $%d  |  FISH %d  |  %.2f kg"), Economy->GetCash(), Economy->GetFishCount(), Economy->GetFishWeightKg()), FLinearColor(0.35f,1.0f,0.45f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 1.0f, false);
        Y += 30.0f;
    }

    if (GameMode)
    {
        const FString TimeText = GameMode->GetDayNightCycle() ? GameMode->GetDayNightCycle()->GetClockText() : TEXT("DAY ? --:--");
        const FString JobText = GameMode->IsFarmJobActive() ? TEXT("  |  LEGAL FARM JOB ACTIVE") : TEXT("");
        const FString GarageText = FString::Printf(TEXT("  |  GARAGE %d/%d"), GameMode->GetOwnedVehicleCount(), GameMode->GetGarageCapacity());
        DrawText(TimeText + GarageText + JobText, FLinearColor(0.95f,0.9f,0.65f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 0.95f, false);
        Y += 30.0f;
    }

    if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        const FString VehicleLine = FString::Printf(
            TEXT("%s  |  CONDITION %.0f%%  |  FUEL %.0f%% (%.1fL)  |  TEMP %.0fC  |  %.0f km/h%s%s"),
            *Vehicle->GetVehicleDisplayName().ToString(), Vehicle->GetConditionPercent()*100.0f, Vehicle->GetFuelPercent()*100.0f,
            Vehicle->GetFuelLiters(), Vehicle->GetEngineTemperatureC(), Vehicle->GetSpeedKmh(),
            Vehicle->WasReportedStolen() && !Vehicle->IsOwnedByPlayer() ? TEXT("  |  STOLEN") : TEXT(""),
            Vehicle->IsOwnedByPlayer() ? TEXT("  |  OWNED") : TEXT(""));
        DrawText(VehicleLine, FLinearColor::White, 36.0f, Y, GEngine->GetSmallFont(), 1.05f, false);
        Y += 30.0f;

        DrawText(
            FString::Printf(TEXT("TUNING | ENGINE L%d/3 | TIRES L%d/3 | TIRE HEALTH %.0f%%"),
                Vehicle->GetEngineUpgradeLevel(), Vehicle->GetTireUpgradeLevel(), Vehicle->GetTireIntegrity()*100.0f),
            Vehicle->GetTireIntegrity() < 0.25f ? FLinearColor(1.0f,0.25f,0.12f,1.0f) : FLinearColor(0.55f,0.85f,1.0f,1.0f),
            36.0f, Y, GEngine->GetSmallFont(), 0.92f, false);
        Y += 28.0f;

        const FString FaultText = Vehicle->GetFaultStatusText();
        if (!FaultText.IsEmpty())
        {
            DrawText(FString::Printf(TEXT("VEHICLE DAMAGE  |  %s  |  DETACHED PARTS %d"), *FaultText, Vehicle->GetDetachedPartCount()),
                FLinearColor(1.0f,0.34f,0.12f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 0.95f, false);
            Y += 28.0f;
        }
    }

    const FString MissionText = BuildMissionText();
    if (!MissionText.IsEmpty())
    {
        DrawText(MissionText, FLinearColor(1.0f,0.82f,0.18f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 1.0f, false);
        Y += 30.0f;
    }
    if (Economy && !Economy->GetActivityMessage().IsEmpty())
    {
        DrawText(Economy->GetActivityMessage(), FLinearColor(0.35f,0.88f,1.0f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 1.0f, false);
        Y += 30.0f;
    }
    DrawText(TEXT("CONTROLS | WASD move/drive | E interact | F exit | F5 save | F9 load | Space jump"), FLinearColor(0.72f,0.82f,0.95f,1.0f), 36.0f, Y, GEngine->GetSmallFont(), 0.85f, false);
}

FString AGTTGameHUD::BuildWantedBar(int32 WantedLevel) const
{
    FString Stars;
    for (int32 Index=0; Index<5; ++Index) Stars += Index < WantedLevel ? TEXT("*") : TEXT("-");
    return FString::Printf(TEXT("WANTED [%s]"), *Stars);
}

FString AGTTGameHUD::BuildMissionText() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const UGTTMissionComponent* Mission = GameMode ? GameMode->GetMissionComponent() : nullptr;
    if (!Mission) return FString();
    if (Mission->GetActiveMissionId() == FName(TEXT("BorrowedTractor")))
    {
        if (Mission->GetMissionState() == EGTTMissionState::Completed) return TEXT("MISSION COMPLETE: BORROWED TRACTOR | tractor owned | $300 earned | build your garage");
        if (Mission->GetMissionState() == EGTTMissionState::Failed) return TEXT("MISSION FAILED: BORROWED TRACTOR");
        if (Mission->GetMissionState() == EGTTMissionState::Active)
        {
            if (Mission->GetMissionStage() == 0) return TEXT("MISSION: BORROWED TRACTOR | Reach neighbour farm and 'borrow' the tractor.");
            if (Mission->GetMissionStage() == 1) return TEXT("MISSION: BORROWED TRACTOR | Lose wanted, then reach the barn goal.");
        }
    }
    return Mission->GetMissionState() == EGTTMissionState::Active ? FString::Printf(TEXT("MISSION: %s"), *Mission->GetActiveMissionId().ToString()) : FString();
}
