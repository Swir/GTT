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

void AGTTGameHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!PlayerOwner)
    {
        return;
    }

    APawn* ControlledPawn = PlayerOwner->GetPawn();
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(ControlledPawn);
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(ControlledPawn);
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;

    const FLinearColor WantedColor = WantedLevel > 0
        ? FLinearColor(1.0f, 0.18f, 0.08f, 1.0f)
        : FLinearColor(0.72f, 0.72f, 0.72f, 1.0f);

    DrawText(BuildWantedBar(WantedLevel), WantedColor, 36.0f, 34.0f, GEngine->GetSmallFont(), 1.35f, false);

    if (Economy)
    {
        const FString EconomyLine = FString::Printf(
            TEXT("CASH $%d  |  FISH %d  |  %.2f kg"),
            Economy->GetCash(),
            Economy->GetFishCount(),
            Economy->GetFishWeightKg());
        DrawText(EconomyLine, FLinearColor(0.35f, 1.0f, 0.45f, 1.0f), 36.0f, 64.0f, GEngine->GetSmallFont(), 1.0f, false);
    }

    float VehicleLineY = 94.0f;
    if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        const FString VehicleLine = FString::Printf(
            TEXT("%s  |  CONDITION %.0f%%  |  FUEL %.0f%% (%.1fL)  |  %.0f km/h%s"),
            *Vehicle->GetVehicleDisplayName().ToString(),
            Vehicle->GetConditionPercent() * 100.0f,
            Vehicle->GetFuelPercent() * 100.0f,
            Vehicle->GetFuelLiters(),
            Vehicle->GetSpeedKmh(),
            Vehicle->WasReportedStolen() ? TEXT("  |  STOLEN") : TEXT(""));

        DrawText(VehicleLine, FLinearColor::White, 36.0f, VehicleLineY, GEngine->GetSmallFont(), 1.05f, false);
        VehicleLineY += 30.0f;
    }

    const FString MissionText = BuildMissionText();
    if (!MissionText.IsEmpty())
    {
        DrawText(MissionText, FLinearColor(1.0f, 0.82f, 0.18f, 1.0f), 36.0f, VehicleLineY, GEngine->GetSmallFont(), 1.0f, false);
        VehicleLineY += 30.0f;
    }

    if (Economy && !Economy->GetActivityMessage().IsEmpty())
    {
        DrawText(
            Economy->GetActivityMessage(),
            FLinearColor(0.35f, 0.88f, 1.0f, 1.0f),
            36.0f,
            VehicleLineY,
            GEngine->GetSmallFont(),
            1.0f,
            false);
        VehicleLineY += 30.0f;
    }

    DrawText(
        TEXT("CONTROLS  |  WASD move/drive  |  Mouse look  |  E enter/interact/fish/shop  |  F exit  |  Space jump"),
        FLinearColor(0.72f, 0.82f, 0.95f, 1.0f),
        36.0f,
        VehicleLineY,
        GEngine->GetSmallFont(),
        0.85f,
        false);
}

FString AGTTGameHUD::BuildWantedBar(int32 WantedLevel) const
{
    FString Stars;
    for (int32 Index = 0; Index < 5; ++Index)
    {
        Stars += Index < WantedLevel ? TEXT("*") : TEXT("-");
    }

    return FString::Printf(TEXT("WANTED [%s]"), *Stars);
}

FString AGTTGameHUD::BuildMissionText() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const UGTTMissionComponent* Mission = GameMode ? GameMode->GetMissionComponent() : nullptr;
    if (!Mission)
    {
        return FString();
    }

    if (Mission->GetActiveMissionId() == FName(TEXT("BorrowedTractor")))
    {
        if (Mission->GetMissionState() == EGTTMissionState::Completed)
        {
            return TEXT("MISSION COMPLETE: BORROWED TRACTOR  |  $300 earned. Free roam unlocked.");
        }

        if (Mission->GetMissionState() == EGTTMissionState::Failed)
        {
            return TEXT("MISSION FAILED: BORROWED TRACTOR");
        }

        if (Mission->GetMissionState() == EGTTMissionState::Active)
        {
            switch (Mission->GetMissionStage())
            {
            case 0:
                return TEXT("MISSION: BORROWED TRACTOR  |  Reach the neighbour farm and 'borrow' the tractor.");
            case 1:
                return TEXT("MISSION: BORROWED TRACTOR  |  Lose wanted, then drive into the BARN - MISSION GOAL zone.");
            default:
                return TEXT("MISSION: BORROWED TRACTOR");
            }
        }
    }

    if (Mission->GetMissionState() == EGTTMissionState::Active)
    {
        return FString::Printf(TEXT("MISSION: %s"), *Mission->GetActiveMissionId().ToString());
    }

    return FString();
}
