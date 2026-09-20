#include "Activities/GTTHeavyHaulDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTTractorPawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "GTT.h"

namespace
{
    constexpr float SmoothMaxRollDegrees = 10.0f;
    constexpr float SmoothMaxPitchDegrees = 8.0f;
    constexpr float RoughRollDegrees = 22.0f;
    constexpr float RoughPitchDegrees = 17.0f;
}

AGTTHeavyHaulDirector::AGTTHeavyHaulDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTHeavyHaulDirector::BeginPlay()
{
    Super::BeginPlay();
    if (GetWorld()) Trailer = GetWorld()->SpawnActor<AGTTFarmTrailer>(TrailerYardLocation, FRotator(0.0f, 90.0f, 0.0f));
}

void AGTTHeavyHaulDirector::ResetDrivingQuality()
{
    SmoothHaulSeconds = 0.0f;
    RoughHaulSeconds = 0.0f;
    bSmoothHaulBonusAnnounced = false;
    bRoughDrivingWarningIssued = false;
}

bool AGTTHeavyHaulDirector::IsSmoothHaulBonusArmed() const
{
    return Trailer
        && Stage == EGTTHeavyHaulStage::DeliverHillFarm
        && Trailer->HasCargo()
        && SmoothHaulSeconds >= SmoothHaulTargetSeconds
        && RoughHaulSeconds <= RoughHaulAllowanceSeconds
        && Trailer->GetCargoIntegrity() >= 0.90f
        && Trailer->GetTrailerIntegrity() >= 0.70f;
}

void AGTTHeavyHaulDirector::UpdateDrivingQuality(float DeltaSeconds)
{
    if (!Trailer || Stage != EGTTHeavyHaulStage::DeliverHillFarm || !Trailer->HasCargo() || !Trailer->IsAttached() || DeltaSeconds <= 0.0f) return;

    const float SpeedKmh = Trailer->GetVelocity().Size() * 0.036f;
    const FRotator Rotation = Trailer->GetActorRotation();
    const float RollDegrees = FMath::Abs(Rotation.Roll);
    const float PitchDegrees = FMath::Abs(Rotation.Pitch);
    const float HitchLoad = Trailer->GetHitchLoad();
    const bool bAxleIntact = Trailer->HasIntactAxle();

    const bool bSmooth =
        SpeedKmh >= SmoothMinSpeedKmh
        && SpeedKmh <= SmoothMaxSpeedKmh
        && HitchLoad <= SmoothMaxHitchLoad
        && RollDegrees <= SmoothMaxRollDegrees
        && PitchDegrees <= SmoothMaxPitchDegrees
        && bAxleIntact
        && Trailer->GetTrailerIntegrity() >= 0.75f
        && Trailer->GetCargoIntegrity() >= 0.88f;

    const bool bRough =
        SpeedKmh > RoughSpeedKmh
        || HitchLoad > RoughHitchLoad
        || RollDegrees > RoughRollDegrees
        || PitchDegrees > RoughPitchDegrees
        || !bAxleIntact;

    if (bSmooth)
    {
        SmoothHaulSeconds = FMath::Min(SmoothHaulTargetSeconds, SmoothHaulSeconds + DeltaSeconds);
    }
    if (bRough)
    {
        RoughHaulSeconds += DeltaSeconds;
    }

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!bSmoothHaulBonusAnnounced && SmoothHaulSeconds >= SmoothHaulTargetSeconds && RoughHaulSeconds <= RoughHaulAllowanceSeconds)
    {
        bSmoothHaulBonusAnnounced = true;
        PushMessage(PlayerPawn, FString::Printf(
            TEXT("SMOOTH HAUL READY: +$%d handling bonus is armed. Keep rough-driving exposure under %.0fs and protect the cargo."),
            SmoothHaulBonus, RoughHaulAllowanceSeconds), 6.5f);
        UE_LOG(LogGTT, Display,
            TEXT("HEAVY_HAUL_DRIVING_QUALITY event=BONUS_ARMED smooth=%.2f rough=%.2f cargo=%.3f trailer=%.3f"),
            SmoothHaulSeconds, RoughHaulSeconds, Trailer->GetCargoIntegrity(), Trailer->GetTrailerIntegrity());
    }

    if (!bRoughDrivingWarningIssued && RoughHaulSeconds > RoughHaulAllowanceSeconds)
    {
        bRoughDrivingWarningIssued = true;
        PushMessage(PlayerPawn, TEXT("HEAVY HAUL WARNING: rough-driving allowance exceeded. Smooth-haul bonus lost; protect the remaining cargo."), 6.0f);
        UE_LOG(LogGTT, Warning,
            TEXT("HEAVY_HAUL_DRIVING_QUALITY event=ROUGH_LIMIT_EXCEEDED smooth=%.2f rough=%.2f speed_kmh=%.2f hitch=%.3f roll=%.2f pitch=%.2f"),
            SmoothHaulSeconds, RoughHaulSeconds, SpeedKmh, HitchLoad, RollDegrees, PitchDegrees);
    }
}

void AGTTHeavyHaulDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsActive()) return;

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    if (Trailer && Trailer->GetRoadsideRepairCount() > RoadsideRepairCount)
    {
        const int32 CompletedRepairs = Trailer->GetRoadsideRepairCount() - RoadsideRepairCount;
        RoadsideRepairCount = Trailer->GetRoadsideRepairCount();
        const float PenaltySeconds = RoadsideRepairTimePenalty * CompletedRepairs;
        TimeRemaining = FMath::Max(0.0f, TimeRemaining - PenaltySeconds);
        PushMessage(UGameplayStatics::GetPlayerPawn(this, 0), FString::Printf(TEXT("HEAVY HAUL FIELD REPAIR: contract clock -%.0fs | repairs %d."), PenaltySeconds, RoadsideRepairCount), 5.0f);
    }

    if (TimeRemaining <= 0.0f)
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        PushMessage(PlayerPawn, TEXT("HEAVY HAUL FAILED: the delivery window expired."), 6.0f);
        ResetContract(true);
        return;
    }

    if (!Trailer) return;
    UpdateDrivingQuality(DeltaSeconds);

    if ((Stage == EGTTHeavyHaulStage::ReachWoodYard || Stage == EGTTHeavyHaulStage::LoadTimber || Stage == EGTTHeavyHaulStage::DeliverHillFarm) && !Trailer->IsAttached())
    {
        Stage = EGTTHeavyHaulStage::HitchTrailer;
        PushMessage(UGameplayStatics::GetPlayerPawn(this, 0), TEXT("TRAILER UNCOUPLED: re-hitch the heavy trailer before continuing."), 4.5f);
    }

    if (Stage == EGTTHeavyHaulStage::ReachWoodYard && FVector::Dist2D(Trailer->GetActorLocation(), WoodYardLoadLocation) < 900.0f)
    {
        Stage = EGTTHeavyHaulStage::LoadTimber;
        PushMessage(UGameplayStatics::GetPlayerPawn(this, 0), TEXT("WOOD YARD REACHED: stop beside the heavy-load crane and load timber."), 5.0f);
    }
}

bool AGTTHeavyHaulDirector::CanTakeContract(APawn* PlayerPawn) const
{
    if (!PlayerPawn || IsActive()) return false;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn)) if (Wanted->GetWantedLevel() > 0) return false;
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) if (GameMode->GetWildlifeAlertLevel() > 0) return false;
    return true;
}

AGTTVehicleBase* AGTTHeavyHaulDirector::FindEligibleTowVehicle(const FVector& Origin, float Radius) const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Candidate = *It;
        if (!Candidate || !Candidate->IsOwnedByPlayer() || Candidate->GetConditionPercent() < 0.40f || !Cast<AGTTTractorPawn>(Candidate) || Candidate->IsHidden()) continue;
        const float DistSq = FVector::DistSquared2D(Candidate->GetActorLocation(), Origin);
        if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = Candidate; }
    }
    return Best;
}

AGTTFieldmasterNativePawn* AGTTHeavyHaulDirector::FindEligibleNativeTowVehicle(const FVector& Origin, float Radius) const
{
    if (!GetWorld()) return nullptr;
    AGTTFieldmasterNativePawn* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTFieldmasterNativePawn> It(GetWorld()); It; ++It)
    {
        AGTTFieldmasterNativePawn* Candidate = *It;
        if (!Candidate || !Candidate->IsNativeFieldmasterReady() || !Candidate->IsLegacyTakeoverActive() || !Candidate->IsOwnedByPlayer()) continue;
        if (Candidate->GetMigrationSnapshot().ConditionPercent < 0.40f) continue;
        const float DistSq = FVector::DistSquared2D(Candidate->GetActorLocation(), Origin);
        if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = Candidate; }
    }
    return Best;
}

float AGTTHeavyHaulDirector::GetContractTowConditionFactor() const
{
    if (ContractNativeTowVehicle) return FMath::Clamp(ContractNativeTowVehicle->GetMigrationSnapshot().ConditionPercent, 0.40f, 1.0f);
    if (ContractTowVehicle) return FMath::Clamp(ContractTowVehicle->GetConditionPercent(), 0.40f, 1.0f);
    return 0.40f;
}

bool AGTTHeavyHaulDirector::TryStartContract(APawn* PlayerPawn)
{
    if (!CanTakeContract(PlayerPawn) || !Trailer) return false;

    if (GetWorld())
    {
        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            const FGTTFleetMissionAssessment Assessment = Fleet->AssessJobReadiness(FName(TEXT("HeavyHaul")));
            PushMessage(PlayerPawn, Fleet->BuildJobDispatchHint(FName(TEXT("HeavyHaul"))), 6.0f);
            if (Assessment.Readiness == EGTTFleetMissionReadiness::Unavailable || Assessment.Readiness == EGTTFleetMissionReadiness::ServiceRequired)
            {
                PushMessage(PlayerPawn, TEXT("HEAVY HAUL BLOCKED: prep a mission-ready TRACTOR loadout before accepting this contract."), 5.5f);
                return false;
            }
        }
    }

    if (!FindEligibleNativeTowVehicle(TrailerYardLocation, 1800.0f) && !FindEligibleTowVehicle(TrailerYardLocation, 1800.0f))
    {
        PushMessage(PlayerPawn, TEXT("HEAVY HAUL requires your owned Fieldmaster tractor in usable condition near the farm."), 5.0f);
        return false;
    }

    Trailer->ResetTrailer(FTransform(FRotator(0.0f, 90.0f, 0.0f), TrailerYardLocation));
    ContractTowVehicle = nullptr;
    ContractNativeTowVehicle = nullptr;
    RoadsideRepairCount = 0;
    ResetDrivingQuality();
    TimeRemaining = ContractTimeLimit;
    Stage = EGTTHeavyHaulStage::HitchTrailer;
    PushMessage(PlayerPawn, TEXT("HEAVY TIMBER HAUL: hitch the farm trailer, drive to NORTH WOOD YARD, load timber and deliver to HILL FARM."), 7.0f);
    return true;
}

bool AGTTHeavyHaulDirector::TryHitchTrailer(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || Stage != EGTTHeavyHaulStage::HitchTrailer) return false;
    if (AGTTFieldmasterNativePawn* NativeTractor = FindEligibleNativeTowVehicle(Trailer->GetActorLocation(), 750.0f))
    {
        if (!Trailer->AttachToNativeFieldmaster(NativeTractor)) return false;
        ContractNativeTowVehicle = NativeTractor;
        ContractTowVehicle = nullptr;
        Stage = Trailer->HasCargo() ? EGTTHeavyHaulStage::DeliverHillFarm : EGTTHeavyHaulStage::ReachWoodYard;
        PushMessage(PlayerPawn, Trailer->HasCargo() ? TEXT("TRAILER RE-HITCHED TO NATIVE FIELDMASTER: continue to HILL FARM.") : TEXT("TRAILER HITCHED TO NATIVE FIELDMASTER: haul the empty trailer to NORTH WOOD YARD."), 5.0f);
        return true;
    }

    AGTTVehicleBase* Tractor = FindEligibleTowVehicle(Trailer->GetActorLocation(), 750.0f);
    if (!Tractor) { PushMessage(PlayerPawn, TEXT("Park your owned Fieldmaster beside the trailer hitch. Tractor must have at least 40% condition.")); return false; }
    if (!Trailer->AttachToVehicle(Tractor)) return false;
    ContractTowVehicle = Tractor;
    ContractNativeTowVehicle = nullptr;
    Stage = Trailer->HasCargo() ? EGTTHeavyHaulStage::DeliverHillFarm : EGTTHeavyHaulStage::ReachWoodYard;
    PushMessage(PlayerPawn, Trailer->HasCargo() ? TEXT("TRAILER RE-HITCHED: continue to HILL FARM.") : TEXT("TRAILER HITCHED: haul the empty trailer to NORTH WOOD YARD."), 5.0f);
    return true;
}

bool AGTTHeavyHaulDirector::TryLoadTimber(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || Stage != EGTTHeavyHaulStage::LoadTimber || !Trailer->IsAttached()) return false;
    if (FVector::Dist2D(Trailer->GetActorLocation(), WoodYardLoadLocation) > 900.0f) { PushMessage(PlayerPawn, TEXT("Bring the trailer into the NORTH WOOD YARD loading area.")); return false; }
    Trailer->SetCargoLoaded(true);
    ResetDrivingQuality();
    Stage = EGTTHeavyHaulStage::DeliverHillFarm;
    PushMessage(PlayerPawn, FString::Printf(
        TEXT("HEAVY LOGS LOADED: deliver to HILL FARM. Hold %.0f-%.0f km/h, keep the hitch calm and the trailer level for %.0fs to arm a +$%d smooth-haul bonus."),
        SmoothMinSpeedKmh, SmoothMaxSpeedKmh, SmoothHaulTargetSeconds, SmoothHaulBonus), 8.0f);
    return true;
}

bool AGTTHeavyHaulDirector::TryRoadsideRepair(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || !IsActive()) return false;
    if (Trailer->IsRoadsideRepairPending())
    {
        PushMessage(PlayerPawn, FString::Printf(TEXT("FIELD REPAIR IN PROGRESS: %.0fs remaining | locked $%d."), Trailer->GetRoadsideRepairTimeRemaining(), Trailer->GetLockedRoadsideRepairQuote()), 4.0f);
        return true;
    }
    return Trailer->TryBeginRoadsideRepair(PlayerPawn);
}

bool AGTTHeavyHaulDirector::TryDeliverTimber(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || Stage != EGTTHeavyHaulStage::DeliverHillFarm || !Trailer->HasCargo()) return false;
    if (!Trailer->IsAttached() || FVector::Dist2D(Trailer->GetActorLocation(), HillFarmDropLocation) > 950.0f) { PushMessage(PlayerPawn, TEXT("Tow the loaded trailer fully into the HILL FARM heavy-haul bay.")); return false; }

    const float CargoFactor = FMath::Clamp(Trailer->GetCargoIntegrity(), 0.20f, 1.0f);
    const float TrailerFactor = FMath::Clamp(Trailer->GetTrailerIntegrity(), 0.45f, 1.0f);
    const float VehicleFactor = GetContractTowConditionFactor();
    const int32 ConditionPay = FMath::RoundToInt(BaseReward * CargoFactor * (0.55f + 0.25f * TrailerFactor + 0.20f * VehicleFactor));
    const bool bFast = TimeRemaining >= ContractTimeLimit * 0.38f;
    const bool bSmoothHaul = IsSmoothHaulBonusArmed();
    const int32 Reward = FMath::Max(180, ConditionPay + (bFast ? FastBonus : 0) + (bSmoothHaul ? SmoothHaulBonus : 0));

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Reward, FString::Printf(TEXT("Heavy timber haul: +$%d"), Reward));
        Economy->PushMessage(FString::Printf(
            TEXT("HEAVY HAUL COMPLETE | $%d | cargo %.0f%% | trailer %.0f%% | smooth %.0fs | rough %.0fs%s%s"),
            Reward,
            Trailer->GetCargoIntegrity() * 100.0f,
            Trailer->GetTrailerIntegrity() * 100.0f,
            SmoothHaulSeconds,
            RoughHaulSeconds,
            bFast ? TEXT(" | FAST BONUS") : TEXT(""),
            bSmoothHaul ? TEXT(" | SMOOTH HAUL BONUS") : TEXT("")), 8.0f);
    }

    UE_LOG(LogGTT, Display,
        TEXT("HEAVY_HAUL_DRIVING_QUALITY event=DELIVER reward=%d fast=%s smooth_bonus=%s smooth=%.2f rough=%.2f cargo=%.3f trailer=%.3f"),
        Reward, bFast ? TEXT("YES") : TEXT("NO"), bSmoothHaul ? TEXT("YES") : TEXT("NO"),
        SmoothHaulSeconds, RoughHaulSeconds, Trailer->GetCargoIntegrity(), Trailer->GetTrailerIntegrity());

    Trailer->SetCargoLoaded(false);
    Stage = EGTTHeavyHaulStage::Completed;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    return true;
}

void AGTTHeavyHaulDirector::ResetContract(bool bResetTrailer)
{
    Stage = EGTTHeavyHaulStage::Idle;
    TimeRemaining = 0.0f;
    RoadsideRepairCount = 0;
    ContractTowVehicle = nullptr;
    ContractNativeTowVehicle = nullptr;
    ResetDrivingQuality();
    if (bResetTrailer && Trailer) Trailer->ResetTrailer(FTransform(FRotator(0.0f, 90.0f, 0.0f), TrailerYardLocation));
}

void AGTTHeavyHaulDirector::PushMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn)) Economy->PushMessage(Message, Duration);
}

FString AGTTHeavyHaulDirector::GetObjectiveText() const
{
    if (!Trailer) return TEXT("HEAVY HAUL | trailer unavailable");
    FString RepairState;
    if (Trailer->IsRoadsideRepairPending())
        RepairState = FString::Printf(TEXT(" | FIELD REPAIR %.0fs / $%d"), Trailer->GetRoadsideRepairTimeRemaining(), Trailer->GetLockedRoadsideRepairQuote());
    else if (Trailer->NeedsRoadsideRepair())
        RepairState = FString::Printf(TEXT(" | FIELD REPAIR $%d"), Trailer->GetRoadsideRepairQuote());

    const FString DrivingQualityState = Trailer->HasCargo()
        ? FString::Printf(TEXT(" | smooth %.0f/%.0fs | rough %.0f/%.0fs%s"),
            SmoothHaulSeconds, SmoothHaulTargetSeconds,
            RoughHaulSeconds, RoughHaulAllowanceSeconds,
            IsSmoothHaulBonusArmed() ? TEXT(" | BONUS ARMED") : TEXT(""))
        : FString();

    switch (Stage)
    {
        case EGTTHeavyHaulStage::Idle: return TEXT("HEAVY HAUL | available at Player Farm");
        case EGTTHeavyHaulStage::HitchTrailer: return FString::Printf(TEXT("HEAVY HAUL | hitch Fieldmaster to trailer | %.0fs%s%s"), TimeRemaining, *DrivingQualityState, *RepairState);
        case EGTTHeavyHaulStage::ReachWoodYard: return FString::Printf(TEXT("HEAVY HAUL | tow EMPTY trailer to NORTH WOOD | %.0fs | hitch %.0f%%%s"), TimeRemaining, Trailer->GetHitchLoad()*100.0f, *RepairState);
        case EGTTHeavyHaulStage::LoadTimber: return FString::Printf(TEXT("HEAVY HAUL | load logs at NORTH WOOD | %.0fs%s"), TimeRemaining, *RepairState);
        case EGTTHeavyHaulStage::DeliverHillFarm: return FString::Printf(TEXT("HEAVY HAUL | HILL FARM | %.0fs | cargo %.0f%% | trailer %.0f%% | hitch %.0f%%%s%s"), TimeRemaining, Trailer->GetCargoIntegrity()*100.0f, Trailer->GetTrailerIntegrity()*100.0f, Trailer->GetHitchLoad()*100.0f, *DrivingQualityState, *RepairState);
        case EGTTHeavyHaulStage::Completed: return TEXT("HEAVY HAUL | completed - new contract available");
        default: return TEXT("HEAVY HAUL");
    }
}
