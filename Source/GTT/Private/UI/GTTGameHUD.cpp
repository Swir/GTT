#include "UI/GTTGameHUD.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
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
    const int32 WantedLevel = Wanted ? Wanted->GetWantedLevel() : 0;

    const FLinearColor WantedColor = WantedLevel > 0
        ? FLinearColor(1.0f, 0.18f, 0.08f, 1.0f)
        : FLinearColor(0.72f, 0.72f, 0.72f, 1.0f);

    DrawText(BuildWantedBar(WantedLevel), WantedColor, 36.0f, 34.0f, GEngine->GetSmallFont(), 1.35f, false);

    if (const AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        const FString VehicleLine = FString::Printf(
            TEXT("%s  |  CONDITION %.0f%%  |  %.0f km/h%s"),
            *Vehicle->GetVehicleDisplayName().ToString(),
            Vehicle->GetConditionPercent() * 100.0f,
            Vehicle->GetSpeedKmh(),
            Vehicle->WasReportedStolen() ? TEXT("  |  STOLEN") : TEXT(""));

        DrawText(VehicleLine, FLinearColor::White, 36.0f, 64.0f, GEngine->GetSmallFont(), 1.05f, false);
    }

    const FString MissionText = BuildMissionText();
    if (!MissionText.IsEmpty())
    {
        DrawText(MissionText, FLinearColor(1.0f, 0.82f, 0.18f, 1.0f), 36.0f, 98.0f, GEngine->GetSmallFont(), 1.0f, false);
    }
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
    if (!Mission || Mission->GetMissionState() != EGTTMissionState::Active)
    {
        return FString();
    }

    if (Mission->GetActiveMissionId() == FName(TEXT("BorrowedTractor")))
    {
        switch (Mission->GetMissionStage())
        {
        case 0:
            return TEXT("MISSION: BORROWED TRACTOR  |  Find a vehicle worth 'borrowing'.");
        case 1:
            return TEXT("MISSION: BORROWED TRACTOR  |  Lose the police and get the tractor home.");
        default:
            return TEXT("MISSION: BORROWED TRACTOR");
        }
    }

    return FString::Printf(TEXT("MISSION: %s"), *Mission->GetActiveMissionId().ToString());
}
