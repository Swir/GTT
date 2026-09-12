#include "Activities/GTTHeavyHaulDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTFarmTrailer.h"
#include "Vehicles/GTTTractorPawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

AGTTHeavyHaulDirector::AGTTHeavyHaulDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTHeavyHaulDirector::BeginPlay()
{
    Super::BeginPlay();
    if (GetWorld())
    {
        Trailer = GetWorld()->SpawnActor<AGTTFarmTrailer>(TrailerYardLocation, FRotator(0.0f, 90.0f, 0.0f));
    }
}

void AGTTHeavyHaulDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsActive()) return;

    TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds);
    if (TimeRemaining <= 0.0f)
    {
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
        PushMessage(PlayerPawn, TEXT("HEAVY HAUL FAILED: the delivery window expired."), 6.0f);
        ResetContract(true);
        return;
    }

    if (!Trailer) return;

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

AGTTVehicleBase* AGTTHeavyHaulDirector::FindEligibleTowVehicle(const FVector& Origin, float Radius) const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Candidate = *It;
        if (!Candidate || !Candidate->IsOwnedByPlayer() || Candidate->GetConditionPercent() < 0.40f) continue;
        if (!Cast<AGTTTractorPawn>(Candidate)) continue;
        const float DistSq = FVector::DistSquared2D(Candidate->GetActorLocation(), Origin);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }
    return Best;
}

bool AGTTHeavyHaulDirector::TryStartContract(APawn* PlayerPawn)
{
    if (!CanTakeContract(PlayerPawn) || !Trailer) return false;
    if (!FindEligibleTowVehicle(TrailerYardLocation, 1800.0f))
    {
        PushMessage(PlayerPawn, TEXT("HEAVY HAUL requires your owned Fieldmaster tractor in usable condition near the farm."), 5.0f);
        return false;
    }

    Trailer->ResetTrailer(FTransform(FRotator(0.0f, 90.0f, 0.0f), TrailerYardLocation));
    ContractTowVehicle = nullptr;
    TimeRemaining = ContractTimeLimit;
    Stage = EGTTHeavyHaulStage::HitchTrailer;
    PushMessage(PlayerPawn, TEXT("HEAVY TIMBER HAUL: hitch the farm trailer, drive to NORTH WOOD YARD, load timber and deliver to HILL FARM."), 7.0f);
    return true;
}

bool AGTTHeavyHaulDirector::TryHitchTrailer(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || Stage != EGTTHeavyHaulStage::HitchTrailer) return false;
    AGTTVehicleBase* Tractor = FindEligibleTowVehicle(Trailer->GetActorLocation(), 750.0f);
    if (!Tractor)
    {
        PushMessage(PlayerPawn, TEXT("Park your owned Fieldmaster beside the trailer hitch. Tractor must have at least 40% condition."));
        return false;
    }
    if (!Trailer->AttachToVehicle(Tractor)) return false;

    ContractTowVehicle = Tractor;
    Stage = Trailer->HasCargo() ? EGTTHeavyHaulStage::DeliverHillFarm : EGTTHeavyHaulStage::ReachWoodYard;
    PushMessage(PlayerPawn, Trailer->HasCargo() ? TEXT("TRAILER RE-HITCHED: continue to HILL FARM.") : TEXT("TRAILER HITCHED: haul the empty trailer to NORTH WOOD YARD."), 5.0f);
    return true;
}

bool AGTTHeavyHaulDirector::TryLoadTimber(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || Stage != EGTTHeavyHaulStage::LoadTimber || !Trailer->IsAttached()) return false;
    if (FVector::Dist2D(Trailer->GetActorLocation(), WoodYardLoadLocation) > 900.0f)
    {
        PushMessage(PlayerPawn, TEXT("Bring the trailer into the NORTH WOOD YARD loading area."));
        return false;
    }

    Trailer->SetCargoLoaded(true);
    Stage = EGTTHeavyHaulStage::DeliverHillFarm;
    PushMessage(PlayerPawn, TEXT("HEAVY LOGS LOADED: deliver to HILL FARM. Speed, rollover angle and rough driving can damage the load."), 7.0f);
    return true;
}

bool AGTTHeavyHaulDirector::TryDeliverTimber(APawn* PlayerPawn)
{
    if (!PlayerPawn || !Trailer || Stage != EGTTHeavyHaulStage::DeliverHillFarm || !Trailer->HasCargo()) return false;
    if (!Trailer->IsAttached() || FVector::Dist2D(Trailer->GetActorLocation(), HillFarmDropLocation) > 950.0f)
    {
        PushMessage(PlayerPawn, TEXT("Tow the loaded trailer fully into the HILL FARM heavy-haul bay."));
        return false;
    }

    const float CargoFactor = FMath::Clamp(Trailer->GetCargoIntegrity(), 0.20f, 1.0f);
    const float TrailerFactor = FMath::Clamp(Trailer->GetTrailerIntegrity(), 0.45f, 1.0f);
    const float VehicleFactor = ContractTowVehicle ? FMath::Clamp(ContractTowVehicle->GetConditionPercent(), 0.40f, 1.0f) : 0.40f;
    const int32 ConditionPay = FMath::RoundToInt(BaseReward * CargoFactor * (0.55f + 0.25f * TrailerFactor + 0.20f * VehicleFactor));
    const bool bFast = TimeRemaining >= ContractTimeLimit * 0.38f;
    const int32 Reward = FMath::Max(180, ConditionPay + (bFast ? FastBonus : 0));

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->AddCash(Reward, FString::Printf(TEXT("Heavy timber haul: +$%d"), Reward));
        Economy->PushMessage(FString::Printf(TEXT("HEAVY HAUL COMPLETE | $%d | cargo %.0f%% | trailer %.0f%%%s"), Reward, Trailer->GetCargoIntegrity()*100.0f, Trailer->GetTrailerIntegrity()*100.0f, bFast ? TEXT(" | FAST BONUS") : TEXT("")), 7.0f);
    }

    Trailer->SetCargoLoaded(false);
    Stage = EGTTHeavyHaulStage::Completed;
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    return true;
}

void AGTTHeavyHaulDirector::ResetContract(bool bResetTrailer)
{
    Stage = EGTTHeavyHaulStage::Idle;
    TimeRemaining = 0.0f;
    ContractTowVehicle = nullptr;
    if (bResetTrailer && Trailer) Trailer->ResetTrailer(FTransform(FRotator(0.0f, 90.0f, 0.0f), TrailerYardLocation));
}

void AGTTHeavyHaulDirector::PushMessage(APawn* Pawn, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn)) Economy->PushMessage(Message, Duration);
}

FString AGTTHeavyHaulDirector::GetObjectiveText() const
{
    if (!Trailer)
        return TEXT("HEAVY HAUL | trailer unavailable");
    switch (Stage)
    {
        case EGTTHeavyHaulStage::Idle: return TEXT("HEAVY HAUL | available at Player Farm");
        case EGTTHeavyHaulStage::HitchTrailer: return FString::Printf(TEXT("HEAVY HAUL | hitch Fieldmaster to trailer | %.0fs"), TimeRemaining);
        case EGTTHeavyHaulStage::ReachWoodYard: return FString::Printf(TEXT("HEAVY HAUL | tow EMPTY trailer to NORTH WOOD | %.0fs | hitch %.0f%%"), TimeRemaining, Trailer->GetHitchLoad()*100.0f);
        case EGTTHeavyHaulStage::LoadTimber: return FString::Printf(TEXT("HEAVY HAUL | load logs at NORTH WOOD | %.0fs"), TimeRemaining);
        case EGTTHeavyHaulStage::DeliverHillFarm: return FString::Printf(TEXT("HEAVY HAUL | HILL FARM | %.0fs | cargo %.0f%% | trailer %.0f%% | hitch %.0f%%"), TimeRemaining, Trailer->GetCargoIntegrity()*100.0f, Trailer->GetTrailerIntegrity()*100.0f, Trailer->GetHitchLoad()*100.0f);
        case EGTTHeavyHaulStage::Completed: return TEXT("HEAVY HAUL | completed - new contract available");
        default: return TEXT("HEAVY HAUL");
    }
}
