#include "Activities/GTTRoadRunDirector.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTLogisticsReputationSubsystem.h"

namespace
{
    const FName RoadRunJob(TEXT("RoadRun"));
    const FName RattlebackId(TEXT("Rattleback82"));
}

AGTTRoadRunDirector::AGTTRoadRunDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    PickupMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupMarker"));
    PickupMarker->SetupAttachment(SceneRoot);
    PickupMarker->SetRelativeLocation(PartsPickupLocation + FVector(0.0f, 0.0f, 150.0f));
    PickupMarker->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    PickupMarker->SetHorizontalAlignment(EHTA_Center);
    PickupMarker->SetWorldSize(42.0f);
    PickupMarker->SetText(FText::FromString(TEXT("PARTS DEPOT\nCOURIER PICKUP")));
    PickupMarker->SetTextRenderColor(FColor(255, 205, 75));
    PickupMarker->SetVisibility(false, true);

    RelayMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RelayMarker"));
    RelayMarker->SetupAttachment(SceneRoot);
    RelayMarker->SetRelativeLocation(HillFarmRelayLocation + FVector(0.0f, 0.0f, 150.0f));
    RelayMarker->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    RelayMarker->SetHorizontalAlignment(EHTA_Center);
    RelayMarker->SetWorldSize(42.0f);
    RelayMarker->SetText(FText::FromString(TEXT("HILL FARM\nCOURIER RELAY")));
    RelayMarker->SetTextRenderColor(FColor(135, 235, 130));
    RelayMarker->SetVisibility(false, true);

    DeliveryMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DeliveryMarker"));
    DeliveryMarker->SetupAttachment(SceneRoot);
    DeliveryMarker->SetRelativeLocation(DeliveryLocation + FVector(0.0f, 0.0f, 150.0f));
    DeliveryMarker->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    DeliveryMarker->SetHorizontalAlignment(EHTA_Center);
    DeliveryMarker->SetWorldSize(42.0f);
    DeliveryMarker->SetText(FText::FromString(TEXT("NORTH WOOD YARD\nCOURIER DROP")));
    DeliveryMarker->SetTextRenderColor(FColor(85, 220, 255));
    DeliveryMarker->SetVisibility(false, true);
}

void AGTTRoadRunDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage == EGTTRoadRunStage::Idle) return;

    StatusMessageCooldown = FMath::Max(0.0f, StatusMessageCooldown - DeltaSeconds);
    APawn* PlayerPawn = ResolvePlayerPawn();
    APawn* ControlledVehicle = nullptr;
    const bool bDrivingRattleback = IsRattlebackControlled(ControlledVehicle);
    if (!PlayerPawn) return;

    if (Stage == EGTTRoadRunStage::CollectParts)
    {
        // Give a driver physically entering the pickup radius on this frame the handoff before
        // expiring the SLA, avoiding a one-frame edge-case loss at exactly zero seconds.
        if (bDrivingRattleback && ControlledVehicle && FVector::DistSquared2D(ControlledVehicle->GetActorLocation(), PartsPickupLocation) <= FMath::Square(CheckpointRadius))
        {
            BeginDelivery(PlayerPawn, ControlledVehicle);
            return;
        }

        if (PriorityUrgencyAtStart > 0 && PickupSlaRemaining > 0.0f)
        {
            PickupSlaRemaining = FMath::Max(0.0f, PickupSlaRemaining - DeltaSeconds);
            if (PickupSlaRemaining <= 0.0f)
            {
                FailContract(PlayerPawn, TEXT("Priority pickup SLA expired before the parts were collected."));
            }
        }
        return;
    }

    if (Stage != EGTTRoadRunStage::RelayHillFarm && Stage != EGTTRoadRunStage::DeliverParts) return;

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    if (TimeRemaining <= 0.0f)
    {
        FailContract(PlayerPawn, TEXT("The workshop closed the order after the delivery window expired."));
        return;
    }

    int32 WantedLevel = 0;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        WantedLevel = Wanted->GetWantedLevel();
        if (WantedLevel > 0) bPoliceIncidentDuringRun = true;
    }

    if (!bDrivingRattleback || !ControlledVehicle) return;

    UpdateDeliveryRisk(DeltaSeconds, PlayerPawn, ControlledVehicle);
    if (ParcelIntegrity <= 0.02f)
    {
        FailContract(PlayerPawn, TEXT("The parts shipment was destroyed by rough driving."), true);
        return;
    }

    if (Stage == EGTTRoadRunStage::RelayHillFarm &&
        FVector::DistSquared2D(ControlledVehicle->GetActorLocation(), HillFarmRelayLocation) <= FMath::Square(CheckpointRadius))
    {
        if (WantedLevel > 0)
        {
            if (StatusMessageCooldown <= 0.0f)
            {
                PushMessage(PlayerPawn, TEXT("COURIER RELAY BLOCKED: Hill Farm will not sign while police are on you."), 4.5f);
                StatusMessageCooldown = 4.0f;
            }
            return;
        }
        CompleteRelay(PlayerPawn);
        return;
    }

    if (Stage == EGTTRoadRunStage::DeliverParts &&
        FVector::DistSquared2D(ControlledVehicle->GetActorLocation(), DeliveryLocation) <= FMath::Square(CheckpointRadius))
    {
        if (WantedLevel > 0)
        {
            if (StatusMessageCooldown <= 0.0f)
            {
                PushMessage(PlayerPawn, TEXT("COURIER HANDOFF BLOCKED: lose the police before entering the North Wood Yard."), 4.5f);
                StatusMessageCooldown = 4.0f;
            }
            return;
        }
        CompleteContract(PlayerPawn);
    }
}

bool AGTTRoadRunDirector::TryStartContract(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTRoadRunStage::Idle || !GetWorld()) return false;

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            PushMessage(PlayerPawn, TEXT("Lose the police before taking the parts courier contract."));
            return false;
        }
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            PushMessage(PlayerPawn, TEXT("Clear the game-warden alert before taking legal courier work."));
            return false;
        }
    }

    UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Logistics || !Logistics->IsRoadCourierWindowOpen())
    {
        PushMessage(PlayerPawn, TEXT("PARTS DEPOT CLOSED: courier dispatch runs 06:00-21:30."), 5.5f);
        return false;
    }

    const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>();
    if (!Fleet)
    {
        PushMessage(PlayerPawn, TEXT("Fleet registry unavailable."));
        return false;
    }

    const FGTTFleetMissionAssessment Assessment = Fleet->AssessJobReadiness(RoadRunJob);
    if (Assessment.AssignedVehicleId != RattlebackId || Assessment.Readiness == EGTTFleetMissionReadiness::Unavailable)
    {
        PushMessage(PlayerPawn, TEXT("ROAD LOADOUT UNAVAILABLE: assign the Rattleback 82 in the garage first."), 6.0f);
        return false;
    }
    if (Assessment.Readiness == EGTTFleetMissionReadiness::ServiceRequired)
    {
        PushMessage(PlayerPawn, FString::Printf(TEXT("RATTLEBACK NOT ROAD-SAFE: service slot %d before courier work."), Assessment.AssignedSlot + 1), 6.0f);
        return false;
    }

    // Freeze all accepted emergency terms at dispatch. Market pressure can evolve while the
    // driver approaches the depot, but the agreed reward, pickup SLA and delivery window cannot.
    PriorityUrgencyAtStart = Logistics->GetRoadPriorityUrgency();
    float PriorityTimeScale = Logistics->GetRoadPriorityTimeScale();
    LockedDeliveryTimeScale = PriorityTimeScale;
    PriorityLabelAtStart = Logistics->GetPriorityDispatchLabel();
    PickupSlaRemaining = PriorityUrgencyAtStart > 0 ? Logistics->GetRoadPriorityPickupSlaSeconds() : 0.0f;

    Stage = EGTTRoadRunStage::CollectParts;
    TimeRemaining = 0.0f;
    ParcelIntegrity = 1.0f;
    NativeImpactBaseline = 0;
    NativeImpactCountDuringRun = 0;
    StatusMessageCooldown = 0.0f;
    bPoliceIncidentDuringRun = false;
    RewardMultiplierAtStart = Logistics->GetRoadCourierRewardMultiplier() * Logistics->GetRoadPriorityRewardMultiplier();
    SetMarkerState(true, false, false);

    const FString PickupTerms = PriorityUrgencyAtStart > 0
        ? FString::Printf(TEXT("PICKUP SLA %.0fs"), PickupSlaRemaining)
        : TEXT("standard pickup window");
    PushMessage(PlayerPawn, FString::Printf(
        TEXT("PARTS COURIER: Rattleback -> PARTS DEPOT -> HILL FARM -> NORTH WOOD YARD. %s | %s | REP %s %d | payout x%.2f | %s."),
        *Logistics->GetRoadCourierScheduleLabel(), *PickupTerms, *Logistics->GetTierLabel(), Logistics->GetReputation(), RewardMultiplierAtStart,
        *PriorityLabelAtStart), 8.0f);
    return true;
}

APawn* AGTTRoadRunDirector::ResolvePlayerPawn() const
{
    APawn* ControlledPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(ControlledPawn))
    {
        if (Vehicle->GetDriverPawn()) return Vehicle->GetDriverPawn();
    }
    if (AGTTRoadVehicleNativePawn* NativeRoad = Cast<AGTTRoadVehicleNativePawn>(ControlledPawn))
    {
        if (NativeRoad->GetDriverPawn()) return NativeRoad->GetDriverPawn();
    }
    return ControlledPawn;
}

bool AGTTRoadRunDirector::IsRattlebackControlled(APawn*& OutControlledVehicle) const
{
    OutControlledVehicle = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!OutControlledVehicle) return false;
    if (const AGTTVehicleBase* Legacy = Cast<AGTTVehicleBase>(OutControlledVehicle))
    {
        return Legacy->GetPersistentVehicleId() == RattlebackId;
    }
    if (const AGTTRoadVehicleNativePawn* NativeRoad = Cast<AGTTRoadVehicleNativePawn>(OutControlledVehicle))
    {
        return NativeRoad->GetPersistentVehicleId() == RattlebackId && NativeRoad->IsLegacyTakeoverActive();
    }
    return false;
}

void AGTTRoadRunDirector::BeginDelivery(APawn* PlayerPawn, APawn* ControlledVehicle)
{
    Stage = EGTTRoadRunStage::RelayHillFarm;
    const float PriorityTimeScale = LockedDeliveryTimeScale;
    TimeRemaining = DeliveryTimeLimit * PriorityTimeScale;
    ParcelIntegrity = 1.0f;
    PickupSlaRemaining = 0.0f;
    NativeImpactCountDuringRun = 0;
    NativeImpactBaseline = 0;
    if (const AGTTRoadVehicleNativePawn* NativeRoad = Cast<AGTTRoadVehicleNativePawn>(ControlledVehicle))
    {
        NativeImpactBaseline = NativeRoad->GetNativeImpactCount();
    }
    SetMarkerState(false, true, false);
    PushMessage(PlayerPawn, FString::Printf(TEXT("PARTS LOADED ON TIME: first signature HILL FARM, then NORTH WOOD YARD. %s locks a %.0fs delivery window; shipment integrity and police risk run through both legs."),
        *PriorityLabelAtStart, TimeRemaining), 7.5f);
}

void AGTTRoadRunDirector::CompleteRelay(APawn* PlayerPawn)
{
    if (Stage != EGTTRoadRunStage::RelayHillFarm) return;
    Stage = EGTTRoadRunStage::DeliverParts;
    SetMarkerState(false, false, true);
    PushMessage(PlayerPawn, FString::Printf(TEXT("HILL FARM SIGNED: %.0f%% shipment intact. Final handoff: NORTH WOOD YARD. %.0fs remain."), ParcelIntegrity * 100.0f, TimeRemaining), 6.5f);
}

void AGTTRoadRunDirector::UpdateDeliveryRisk(float DeltaSeconds, APawn* PlayerPawn, APawn* ControlledVehicle)
{
    if (!ControlledVehicle || !GetWorld()) return;

    const float SpeedKmh = ControlledVehicle->GetVelocity().Size() * 0.036f;
    if (SpeedKmh > SafeCruiseSpeedKmh)
    {
        const float Overspeed = FMath::Clamp((SpeedKmh - SafeCruiseSpeedKmh) / 70.0f, 0.0f, 1.5f);
        ParcelIntegrity = FMath::Max(0.0f, ParcelIntegrity - Overspeed * 0.0065f * DeltaSeconds);
    }

    if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
    {
        const FGTTFleetMissionAssessment Assessment = Fleet->AssessJobReadiness(RoadRunJob);
        const float MechanicalRisk =
            FMath::Max(0.0f, 0.70f - Assessment.ConditionPercent) +
            FMath::Max(0.0f, 0.70f - Assessment.TireIntegrity) * 0.8f +
            FMath::Max(0.0f, 0.70f - Assessment.BodyHealth) * 0.55f;
        if (MechanicalRisk > 0.0f)
        {
            ParcelIntegrity = FMath::Max(0.0f, ParcelIntegrity - MechanicalRisk * 0.0045f * DeltaSeconds);
        }
    }

    if (AGTTRoadVehicleNativePawn* NativeRoad = Cast<AGTTRoadVehicleNativePawn>(ControlledVehicle))
    {
        const int32 ImpactCount = NativeRoad->GetNativeImpactCount();
        if (ImpactCount > NativeImpactBaseline)
        {
            const int32 NewImpacts = ImpactCount - NativeImpactBaseline;
            NativeImpactCountDuringRun += NewImpacts;
            ParcelIntegrity = FMath::Max(0.0f, ParcelIntegrity - NewImpacts * 0.085f);
            NativeImpactBaseline = ImpactCount;
            PushMessage(PlayerPawn, FString::Printf(TEXT("COURIER IMPACT: shipment integrity %.0f%%"), ParcelIntegrity * 100.0f), 3.5f);
        }
    }
}

void AGTTRoadRunDirector::CompleteContract(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTRoadRunStage::DeliverParts) return;

    const float ActiveDeliveryTimeLimit = DeliveryTimeLimit * LockedDeliveryTimeScale;
    const float TimeRatio = ActiveDeliveryTimeLimit > 0.0f ? TimeRemaining / ActiveDeliveryTimeLimit : 0.0f;
    const int32 DamagePenalty = FMath::RoundToInt((1.0f - FMath::Clamp(ParcelIntegrity, 0.0f, 1.0f)) * 160.0f);
    const int32 FastBonus = TimeRatio >= 0.38f ? FastDeliveryBonus : 0;
    const int32 CleanBonus = ParcelIntegrity >= 0.97f && NativeImpactCountDuringRun == 0 && !bPoliceIncidentDuringRun ? CleanRunBonus : 0;
    const int32 RawReward = FMath::Max(70, BaseReward - DamagePenalty + FastBonus + CleanBonus);
    const int32 TotalReward = FMath::Max(70, FMath::RoundToInt(static_cast<float>(RawReward) * FMath::Max(1.0f, RewardMultiplierAtStart)));

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(TotalReward, FString::Printf(TEXT("Village logistics courier: +$%d"), TotalReward));
    }

    UGTTLogisticsReputationSubsystem* Logistics = GetWorld() ? GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>() : nullptr;
    if (Logistics)
    {
        Logistics->RecordCourierSuccess(TotalReward, ParcelIntegrity, FastBonus > 0, bPoliceIncidentDuringRun, NativeImpactCountDuringRun);
    }

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->PushMessage(FString::Printf(
            TEXT("COURIER COMPLETE: $%d | shipment %.0f%% | %.0fs left | REP %s %d | streak %d%s%s%s"),
            TotalReward, ParcelIntegrity * 100.0f, TimeRemaining,
            Logistics ? *Logistics->GetTierLabel() : TEXT("NEWCOMER"), Logistics ? Logistics->GetReputation() : 0,
            Logistics ? Logistics->GetCleanStreak() : 0,
            FastBonus > 0 ? TEXT(" | FAST") : TEXT(""),
            CleanBonus > 0 ? TEXT(" | CLEAN") : TEXT(""),
            bPoliceIncidentDuringRun ? TEXT(" | POLICE INCIDENT") : TEXT("")), 8.0f);
    }

    Stage = EGTTRoadRunStage::Idle;
    TimeRemaining = 0.0f;
    PickupSlaRemaining = 0.0f;
    ParcelIntegrity = 1.0f;
    NativeImpactBaseline = 0;
    NativeImpactCountDuringRun = 0;
    bPoliceIncidentDuringRun = false;
    RewardMultiplierAtStart = 1.0f;
    LockedDeliveryTimeScale = 1.0f;
    PriorityUrgencyAtStart = 0;
    PriorityLabelAtStart = TEXT("STANDARD DISPATCH");
    SetMarkerState(false, false, false);
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
}

void AGTTRoadRunDirector::FailContract(APawn* PlayerPawn, const FString& Reason, bool bSevereFailure)
{
    if (GetWorld())
    {
        if (UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
        {
            Logistics->RecordCourierFailure(bSevereFailure);
        }
    }

    Stage = EGTTRoadRunStage::Idle;
    TimeRemaining = 0.0f;
    PickupSlaRemaining = 0.0f;
    ParcelIntegrity = 1.0f;
    NativeImpactBaseline = 0;
    NativeImpactCountDuringRun = 0;
    bPoliceIncidentDuringRun = false;
    RewardMultiplierAtStart = 1.0f;
    LockedDeliveryTimeScale = 1.0f;
    PriorityUrgencyAtStart = 0;
    PriorityLabelAtStart = TEXT("STANDARD DISPATCH");
    SetMarkerState(false, false, false);
    PushMessage(PlayerPawn, FString::Printf(TEXT("PARTS COURIER FAILED: %s Reputation/streak consequence saved."), *Reason), 6.5f);
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
}

FString AGTTRoadRunDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTRoadRunStage::CollectParts:
            return PriorityUrgencyAtStart > 0
                ? FString::Printf(TEXT("PARTS COURIER | Rattleback -> VILLAGE PARTS DEPOT | PICKUP SLA %.0fs"), PickupSlaRemaining)
                : TEXT("PARTS COURIER | Rattleback -> VILLAGE PARTS DEPOT");
        case EGTTRoadRunStage::RelayHillFarm:
            return FString::Printf(TEXT("PARTS COURIER | HILL FARM RELAY | %.0fs | shipment %.0f%%"), TimeRemaining, ParcelIntegrity * 100.0f);
        case EGTTRoadRunStage::DeliverParts:
            return FString::Printf(TEXT("PARTS COURIER | NORTH WOOD YARD | %.0fs | shipment %.0f%%"), TimeRemaining, ParcelIntegrity * 100.0f);
        default:
            return FString();
    }
}

void AGTTRoadRunDirector::PushMessage(APawn* PlayerPawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)) Economy->PushMessage(Message, Duration);
}

void AGTTRoadRunDirector::SetMarkerState(bool bPickupVisible, bool bRelayVisible, bool bDeliveryVisible)
{
    if (PickupMarker) PickupMarker->SetVisibility(bPickupVisible, true);
    if (RelayMarker) RelayMarker->SetVisibility(bRelayVisible, true);
    if (DeliveryMarker) DeliveryMarker->SetVisibility(bDeliveryVisible, true);
}
