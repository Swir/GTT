#include "World/GTTRuralEconomySubsystem.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTRuralEconomySave.h"
#include "TimerManager.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTRoadGraph.h"
#include "World/GTTRuralEconomyTerminal.h"

namespace
{
const FVector ImpoundStorage(3450.0f, -3250.0f, 120.0f);
const FVector ImpoundRelease(3200.0f, -2550.0f, 120.0f);
}

void UGTTRuralEconomySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    LoadState();
    SpawnServicePoints(InWorld);
    InWorld.GetTimerManager().SetTimer(RoadLawTimer, this, &UGTTRuralEconomySubsystem::EvaluateRoadLaw, 1.0f, true, 2.0f);

    if (!ImpoundedVehicleId.IsNone())
    {
        if (AGTTVehicleBase* Vehicle = FindOwnedVehicleById(ImpoundedVehicleId))
        {
            Vehicle->RecallToTransform(FTransform(FRotator(0, 90, 0), ImpoundStorage));
        }
    }
}

void UGTTRuralEconomySubsystem::AddContraband(APawn* PlayerPawn, int32 Units, int32 EstimatedValue, const FString& SourceLabel)
{
    if (Units <= 0 || EstimatedValue <= 0) return;
    ContrabandUnits += Units;
    ContrabandValue += EstimatedValue;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->PushMessage(FString::Printf(TEXT("STASHED: %s | contraband %d units | fence estimate $%d"), *SourceLabel, ContrabandUnits, ContrabandValue), 6.0f);
    }
    SaveState();
}

bool UGTTRuralEconomySubsystem::SellContraband(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    if (!Economy) return false;

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            Economy->PushMessage(TEXT("BACKLOT FENCE: lose the police tail before bringing hot goods."));
            return false;
        }
    }
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(PlayerPawn)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            Economy->PushMessage(TEXT("BACKLOT FENCE: game warden is watching the roads. Come back clean."));
            return false;
        }
    }
    if (ContrabandUnits <= 0 || ContrabandValue <= 0)
    {
        Economy->PushMessage(TEXT("BACKLOT FENCE: nothing illegal in your stash."));
        return false;
    }

    const int32 Payout = FMath::Max(1, ContrabandValue + ContrabandUnits * 9);
    LifetimeFenceRevenue += Payout;
    const int32 SoldUnits = ContrabandUnits;
    ContrabandUnits = 0;
    ContrabandValue = 0;
    Economy->AddCash(Payout, FString::Printf(TEXT("Backlot fence: %d units moved for $%d"), SoldUnits, Payout));
    SaveState();
    return true;
}

bool UGTTRuralEconomySubsystem::BuyOrUseInsurance(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    if (!Economy) return false;
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            Economy->PushMessage(TEXT("FARM MUTUAL: policy desk closes while police are chasing you."));
            return false;
        }
    }

    if (!bInsuranceActive)
    {
        constexpr int32 Premium = 260;
        if (!Economy->SpendCash(Premium, TEXT("Farm Mutual vehicle policy: -$260"))) return false;
        bInsuranceActive = true;
        Economy->PushMessage(TEXT("INSURED: impound release fees reduced by 60%; repair claims now available."), 6.0f);
        SaveState();
        return true;
    }

    AGTTVehicleBase* Vehicle = FindNearestOwnedVehicle(PlayerPawn->GetActorLocation(), 950.0f);
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("FARM MUTUAL: park an owned vehicle beside the office for a repair claim."));
        return false;
    }
    if (Vehicle->GetConditionPercent() >= 0.95f && Vehicle->GetTireIntegrity() >= 0.95f)
    {
        Economy->PushMessage(TEXT("FARM MUTUAL: vehicle condition does not justify a claim."));
        return false;
    }
    constexpr int32 Deductible = 55;
    if (!Economy->SpendCash(Deductible, TEXT("Insurance repair deductible: -$55"))) return false;
    Vehicle->RepairVehicle(45.0f);
    Vehicle->RepairTires();
    Economy->PushMessage(FString::Printf(TEXT("INSURANCE CLAIM: %s repaired; fuel is not covered."), *Vehicle->GetVehicleDisplayName().ToString()), 5.0f);
    SaveState();
    return true;
}

void UGTTRuralEconomySubsystem::HandleArrestImpound(APawn* PlayerPawn)
{
    if (!PlayerPawn || !ImpoundedVehicleId.IsNone()) return;
    AGTTVehicleBase* Vehicle = FindNearestOwnedVehicle(PlayerPawn->GetActorLocation(), 1800.0f);
    if (!Vehicle) return;

    const int32 WantedLevel = FMath::Max(1, UGTTGameplayStatics::GetPlayerWantedLevel(PlayerPawn, 0));
    const int32 RawFee = 140 + WantedLevel * 55;
    PendingImpoundFee = bInsuranceActive ? FMath::Max(60, FMath::RoundToInt(RawFee * 0.40f)) : RawFee;
    ImpoundedVehicleId = Vehicle->GetPersistentVehicleId();
    Vehicle->RecallToTransform(FTransform(FRotator(0, 90, 0), ImpoundStorage));

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
    {
        Economy->PushMessage(FString::Printf(TEXT("COUNTY IMPOUND: %s seized | release fee $%d%s"),
            *Vehicle->GetVehicleDisplayName().ToString(), PendingImpoundFee, bInsuranceActive ? TEXT(" | insured rate") : TEXT("")), 7.0f);
    }
    SaveState();
}

bool UGTTRuralEconomySubsystem::ReleaseImpoundedVehicle(APawn* PlayerPawn)
{
    if (!PlayerPawn) return false;
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    if (!Economy) return false;
    if (ImpoundedVehicleId.IsNone())
    {
        Economy->PushMessage(TEXT("COUNTY IMPOUND: no vehicle held under your farm registration."));
        return false;
    }
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            Economy->PushMessage(TEXT("COUNTY IMPOUND: clear your current warrant first."));
            return false;
        }
    }
    AGTTVehicleBase* Vehicle = FindOwnedVehicleById(ImpoundedVehicleId);
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("COUNTY IMPOUND: registered vehicle could not be located."));
        return false;
    }
    if (!Economy->SpendCash(PendingImpoundFee, FString::Printf(TEXT("Impound release: -$%d"), PendingImpoundFee))) return false;

    Vehicle->RecallToTransform(FTransform(FRotator(0, 90, 0), ImpoundRelease));
    Economy->PushMessage(FString::Printf(TEXT("RELEASED: %s is waiting outside the lot."), *Vehicle->GetVehicleDisplayName().ToString()), 5.0f);
    ImpoundedVehicleId = NAME_None;
    PendingImpoundFee = 0;
    SaveState();
    return true;
}

void UGTTRuralEconomySubsystem::EvaluateRoadLaw()
{
    CitationCooldownSeconds = FMath::Max(0.0f, CitationCooldownSeconds - 1.0f);
    APawn* Controlled = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Controlled);
    if (!Vehicle || !Vehicle->IsOccupied())
    {
        SpeedingExposureSeconds = 0.0f;
        return;
    }

    const float Limit = FGTTRoadGraph::GetSpeedLimitAtLocation(Vehicle->GetActorLocation());
    const float Speed = Vehicle->GetSpeedKmh();
    const float Over = Speed - Limit;
    if (Over <= 18.0f)
    {
        SpeedingExposureSeconds = FMath::Max(0.0f, SpeedingExposureSeconds - 1.5f);
        return;
    }

    SpeedingExposureSeconds += 1.0f;
    if (SpeedingExposureSeconds < 3.0f || CitationCooldownSeconds > 0.0f) return;

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Vehicle);
    if (!Economy) return;
    const int32 Fine = 25 + FMath::RoundToInt(Over * 1.8f);
    Economy->ChargeFine(Fine, FString::Printf(TEXT("SPEEDING %.0f/%.0f km/h - citation $%d."), Speed, Limit, Fine));
    ++SpeedingCitations;
    CitationCooldownSeconds = 20.0f;
    SpeedingExposureSeconds = 0.0f;

    if (Over >= 45.0f)
    {
        if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Vehicle)) Wanted->AddHeat(8.0f);
        Economy->PushMessage(TEXT("RECKLESS SPEED: citation issued and police heat added."), 5.0f);
    }
    SaveState();
}

AGTTVehicleBase* UGTTRuralEconomySubsystem::FindNearestOwnedVehicle(const FVector& Origin, float Radius) const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || !Vehicle->IsOwnedByPlayer() || Vehicle->GetPersistentVehicleId().IsNone() || Vehicle->IsOccupied()) continue;
        const float DistSq = FVector::DistSquared2D(Origin, Vehicle->GetActorLocation());
        if (DistSq < BestDistSq) { BestDistSq = DistSq; Best = Vehicle; }
    }
    return Best;
}

AGTTVehicleBase* UGTTRuralEconomySubsystem::FindOwnedVehicleById(FName VehicleId) const
{
    if (!GetWorld() || VehicleId.IsNone()) return nullptr;
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (Vehicle && Vehicle->IsOwnedByPlayer() && Vehicle->GetPersistentVehicleId() == VehicleId) return Vehicle;
    }
    return nullptr;
}

void UGTTRuralEconomySubsystem::SpawnServicePoints(UWorld& World)
{
    if (AGTTRuralEconomyTerminal* Fence = World.SpawnActor<AGTTRuralEconomyTerminal>(FVector(-5050,-750,55), FRotator::ZeroRotator))
        Fence->SetTerminalType(EGTTRuralEconomyTerminalType::Fence);
    if (AGTTRuralEconomyTerminal* Insurance = World.SpawnActor<AGTTRuralEconomyTerminal>(FVector(1250,-2300,55), FRotator::ZeroRotator))
        Insurance->SetTerminalType(EGTTRuralEconomyTerminalType::Insurance);
    if (AGTTRuralEconomyTerminal* Impound = World.SpawnActor<AGTTRuralEconomyTerminal>(FVector(3150,-2850,55), FRotator::ZeroRotator))
        Impound->SetTerminalType(EGTTRuralEconomyTerminalType::Impound);
}

void UGTTRuralEconomySubsystem::LoadState()
{
    UGTTRuralEconomySave* Save = Cast<UGTTRuralEconomySave>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Save) return;
    ContrabandUnits = FMath::Max(0, Save->ContrabandUnits);
    ContrabandValue = FMath::Max(0, Save->ContrabandValue);
    bInsuranceActive = Save->bInsuranceActive;
    ImpoundedVehicleId = Save->ImpoundedVehicleId;
    PendingImpoundFee = FMath::Max(0, Save->PendingImpoundFee);
    LifetimeFenceRevenue = FMath::Max(0, Save->LifetimeFenceRevenue);
    SpeedingCitations = FMath::Max(0, Save->SpeedingCitations);
}

void UGTTRuralEconomySubsystem::SaveState() const
{
    UGTTRuralEconomySave* Save = Cast<UGTTRuralEconomySave>(UGameplayStatics::CreateSaveGameObject(UGTTRuralEconomySave::StaticClass()));
    if (!Save) return;
    Save->SaveVersion = 1;
    Save->ContrabandUnits = ContrabandUnits;
    Save->ContrabandValue = ContrabandValue;
    Save->bInsuranceActive = bInsuranceActive;
    Save->ImpoundedVehicleId = ImpoundedVehicleId;
    Save->PendingImpoundFee = PendingImpoundFee;
    Save->LifetimeFenceRevenue = LifetimeFenceRevenue;
    Save->SpeedingCitations = SpeedingCitations;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}
