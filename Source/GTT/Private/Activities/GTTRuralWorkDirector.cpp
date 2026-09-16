#include "Activities/GTTRuralWorkDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTTractorPawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTGarageFleetSubsystem.h"

AGTTRuralWorkDirector::AGTTRuralWorkDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTRuralWorkDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage == EGTTRuralWorkStage::Idle) return;

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* ControlledVehicle = Cast<AGTTVehicleBase>(PlayerPawn))
    {
        if (ControlledVehicle->GetDriverPawn()) PlayerPawn = ControlledVehicle->GetDriverPawn();
    }

    if (TimeRemaining <= 0.0f)
    {
        FailWork(PlayerPawn, TEXT("The contract window expired."));
        return;
    }

    if (Stage == EGTTRuralWorkStage::DeliverTimber)
    {
        if (AGTTVehicleBase* Vehicle = FindNearbyVehicle(PlayerPawn, 750.0f, false))
        {
            const float BodyDamage = 1.0f - Vehicle->GetConditionPercent();
            const float TireDamage = 1.0f - Vehicle->GetTireIntegrity();
            const float Roughness = FMath::Max(BodyDamage, TireDamage * 0.75f);
            if (Roughness > 0.35f)
            {
                CargoIntegrity = FMath::Max(0.0f, CargoIntegrity - Roughness * 0.022f * DeltaSeconds);
            }
        }
        if (CargoIntegrity <= 0.08f)
        {
            FailWork(PlayerPawn, TEXT("The timber load broke loose and was rejected."));
        }
    }
}

bool AGTTRuralWorkDirector::CanTakeLegalWork(APawn* PlayerPawn) const
{
    if (!PlayerPawn || Stage != EGTTRuralWorkStage::Idle) return false;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0) return false;
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0) return false;
    }
    return true;
}

bool AGTTRuralWorkDirector::TryStartTimber(APawn* PlayerPawn)
{
    if (!CanTakeLegalWork(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("TIMBER BOARD: clear police/ranger attention and finish other work first."));
        return false;
    }

    if (GetWorld())
    {
        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            const FGTTFleetMissionAssessment Assessment = Fleet->AssessJobReadiness(FName(TEXT("TimberHaul")));
            PushMessage(PlayerPawn, Fleet->BuildJobDispatchHint(FName(TEXT("TimberHaul"))), 6.0f);
            if (Assessment.Readiness == EGTTFleetMissionReadiness::ServiceRequired)
            {
                PushMessage(PlayerPawn, TEXT("TIMBER LOADOUT CAUTION: service the CARGO loadout or use another healthy vehicle before loading logs."), 5.5f);
            }
        }
    }

    WorkType = EGTTRuralWorkType::TimberHaul;
    Stage = EGTTRuralWorkStage::ReachTimberPickup;
    TimeRemaining = TimberTimeLimit;
    CargoIntegrity = 1.0f;
    MowingPassesCompleted = 0;
    PushMessage(PlayerPawn, TEXT("TIMBER CONTRACT: collect legal logs at NORTH WOOD YARD, then deliver to the workshop."), 7.0f);
    return true;
}

bool AGTTRuralWorkDirector::TryPickupTimber(APawn* PlayerPawn)
{
    if (!PlayerPawn || WorkType != EGTTRuralWorkType::TimberHaul || Stage != EGTTRuralWorkStage::ReachTimberPickup) return false;
    AGTTVehicleBase* Vehicle = FindNearbyVehicle(PlayerPawn, 700.0f, false);
    if (!Vehicle)
    {
        PushMessage(PlayerPawn, TEXT("Park a working vehicle beside the timber stack before loading."));
        return false;
    }
    if (Vehicle->GetConditionPercent() < 0.25f)
    {
        PushMessage(PlayerPawn, TEXT("That vehicle is too damaged to secure a timber load."));
        return false;
    }
    Stage = EGTTRuralWorkStage::DeliverTimber;
    CargoIntegrity = 1.0f;
    PushMessage(PlayerPawn, TEXT("LOGS LOADED: take them to WORKSHOP YARD. Damage and bad tires can reduce the payout."), 7.0f);
    return true;
}

bool AGTTRuralWorkDirector::TryDeliverTimber(APawn* PlayerPawn)
{
    if (!PlayerPawn || WorkType != EGTTRuralWorkType::TimberHaul || Stage != EGTTRuralWorkStage::DeliverTimber) return false;
    if (!FindNearbyVehicle(PlayerPawn, 750.0f, false))
    {
        PushMessage(PlayerPawn, TEXT("Bring the loaded vehicle into the workshop yard."));
        return false;
    }
    const float TimeRatio = TimberTimeLimit > 0.0f ? TimeRemaining / TimberTimeLimit : 0.0f;
    const int32 IntegrityPay = FMath::RoundToInt(TimberBaseReward * FMath::Clamp(CargoIntegrity, 0.0f, 1.0f));
    const int32 Bonus = TimeRatio >= 0.45f ? TimberFastBonus : 0;
    FinishWork(PlayerPawn, FMath::Max(60, IntegrityPay + Bonus),
        FString::Printf(TEXT("TIMBER COMPLETE | load %.0f%%%s"), CargoIntegrity * 100.0f, Bonus > 0 ? TEXT(" | FAST BONUS") : TEXT("")));
    return true;
}

bool AGTTRuralWorkDirector::TryStartMowing(APawn* PlayerPawn)
{
    if (!CanTakeLegalWork(PlayerPawn))
    {
        PushMessage(PlayerPawn, TEXT("FIELD BOARD: clear police/ranger attention and finish other work first."));
        return false;
    }

    if (GetWorld())
    {
        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            const FGTTFleetMissionAssessment Assessment = Fleet->AssessJobReadiness(FName(TEXT("FieldMowing")));
            PushMessage(PlayerPawn, Fleet->BuildJobDispatchHint(FName(TEXT("FieldMowing"))), 6.0f);
            if (Assessment.Readiness == EGTTFleetMissionReadiness::Unavailable || Assessment.Readiness == EGTTFleetMissionReadiness::ServiceRequired)
            {
                PushMessage(PlayerPawn, TEXT("FIELD WORK BLOCKED: prep a mission-ready TRACTOR loadout before starting the mowing route."), 5.5f);
                return false;
            }
        }
    }

    if (!FindNearbyVehicle(PlayerPawn, 900.0f, true))
    {
        PushMessage(PlayerPawn, TEXT("FIELD WORK requires a tractor parked near the field office."));
        return false;
    }
    WorkType = EGTTRuralWorkType::FieldMowing;
    Stage = EGTTRuralWorkStage::MowingField;
    TimeRemaining = MowingTimeLimit;
    CargoIntegrity = 1.0f;
    MowingPassesCompleted = 0;
    PushMessage(PlayerPawn, TEXT("MOWING CONTRACT: drive the tractor through FIELD GATES 1-5 in order."), 7.0f);
    return true;
}

bool AGTTRuralWorkDirector::TryMowingPass(APawn* PlayerPawn, int32 PassIndex)
{
    if (!PlayerPawn || WorkType != EGTTRuralWorkType::FieldMowing || Stage != EGTTRuralWorkStage::MowingField) return false;
    if (PassIndex != MowingPassesCompleted)
    {
        return false;
    }

    ++MowingPassesCompleted;
    PushMessage(PlayerPawn, FString::Printf(TEXT("FIELD PASS %d/%d complete."), MowingPassesCompleted, RequiredMowingPasses), 2.5f);
    if (MowingPassesCompleted >= RequiredMowingPasses)
    {
        const int32 TimeBonus = TimeRemaining >= MowingTimeLimit * 0.35f ? 80 : 0;
        FinishWork(PlayerPawn, MowingReward + TimeBonus,
            FString::Printf(TEXT("MOWING COMPLETE%s"), TimeBonus > 0 ? TEXT(" | EFFICIENT ROUTE BONUS") : TEXT("")));
    }
    return true;
}

FString AGTTRuralWorkDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTRuralWorkStage::ReachTimberPickup:
            return FString::Printf(TEXT("RURAL WORK | TIMBER | reach NORTH WOOD YARD | %.0fs"), TimeRemaining);
        case EGTTRuralWorkStage::DeliverTimber:
            return FString::Printf(TEXT("RURAL WORK | TIMBER -> WORKSHOP | %.0fs | load %.0f%%"), TimeRemaining, CargoIntegrity * 100.0f);
        case EGTTRuralWorkStage::MowingField:
            return FString::Printf(TEXT("RURAL WORK | MOWING | gate %d/%d | %.0fs"), MowingPassesCompleted + 1, RequiredMowingPasses, TimeRemaining);
        default:
            return FString();
    }
}

AGTTVehicleBase* AGTTRuralWorkDirector::FindNearbyVehicle(APawn* PlayerPawn, float Radius, bool bRequireTractor) const
{
    if (!PlayerPawn || !GetWorld()) return nullptr;
    if (AGTTVehicleBase* Controlled = Cast<AGTTVehicleBase>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        if ((!bRequireTractor || Cast<AGTTTractorPawn>(Controlled)) && Controlled->GetConditionPercent() > 0.0f) return Controlled;
    }

    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || Vehicle->GetConditionPercent() <= 0.0f) continue;
        if (bRequireTractor && !Cast<AGTTTractorPawn>(Vehicle)) continue;
        const float DistSq = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Vehicle;
        }
    }
    return Best;
}

void AGTTRuralWorkDirector::FinishWork(APawn* PlayerPawn, int32 Reward, const FString& Message)
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Reward, FString::Printf(TEXT("Rural contract: +$%d"), Reward));
        Economy->PushMessage(FString::Printf(TEXT("%s | +$%d"), *Message, Reward), 7.0f);
    }
    WorkType = EGTTRuralWorkType::None;
    Stage = EGTTRuralWorkStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    MowingPassesCompleted = 0;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
}

void AGTTRuralWorkDirector::FailWork(APawn* PlayerPawn, const FString& Reason)
{
    WorkType = EGTTRuralWorkType::None;
    Stage = EGTTRuralWorkStage::Idle;
    TimeRemaining = 0.0f;
    CargoIntegrity = 1.0f;
    MowingPassesCompleted = 0;
    PushMessage(PlayerPawn, FString::Printf(TEXT("RURAL WORK FAILED: %s"), *Reason), 6.0f);
}

void AGTTRuralWorkDirector::PushMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn)) Economy->PushMessage(Message, Duration);
}
