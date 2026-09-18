#include "Vehicles/GTTWorkshopRecoverySubsystem.h"

#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    constexpr float WorkshopScanIntervalSeconds = 0.25f;
    constexpr float WorkshopVehicleRadiusCm = 725.0f;
    constexpr float WorkshopCustomerRadiusCm = 650.0f;
    constexpr float WorkshopMaxVehicleSpeedKmh = 2.0f;
    constexpr float PromptCooldownSeconds = 8.0f;
    const FVector WorkshopBaseLocation(-400.0f, 2650.0f, 105.0f);
}

TStatId UGTTWorkshopRecoverySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopRecoverySubsystem, STATGROUP_Tickables);
}

void UGTTWorkshopRecoverySubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    PromptCooldown = FMath::Max(0.0f, PromptCooldown - DeltaSeconds);
    ScanAccumulator += DeltaSeconds;
    if (ScanAccumulator < WorkshopScanIntervalSeconds) return;
    ScanAccumulator = 0.0f;

    APlayerController* PlayerController = World->GetFirstPlayerController();
    APawn* CustomerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (!PlayerController || !CustomerPawn) return;

    AGTTRoadVehicleNativePawn* Vehicle = FindNearbyWorkshopVehicle(CustomerPawn);
    if (!Vehicle) return;

    MaybeShowWorkshopPrompt(CustomerPawn, Vehicle);
    if (PlayerController->WasInputKeyJustPressed(EKeys::H))
    {
        PurchaseFullWorkshopService(Vehicle, CustomerPawn);
    }
}

bool UGTTWorkshopRecoverySubsystem::IsVehicleAtWorkshop(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    return Vehicle
        && FVector::Dist2D(Vehicle->GetActorLocation(), WorkshopBaseLocation) <= WorkshopVehicleRadiusCm
        && Vehicle->GetVelocity().Size() * 0.036f <= WorkshopMaxVehicleSpeedKmh;
}

FGTTWorkshopServiceQuote UGTTWorkshopRecoverySubsystem::GetWorkshopServiceQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    FGTTWorkshopServiceQuote Quote;
    if (!Vehicle) return Quote;

    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    Quote.PersistentVehicleId = Vehicle->GetPersistentVehicleId();
    Quote.ConditionPercent = State.ConditionPercent;
    Quote.TireIntegrity = State.TireIntegrity;
    Quote.FuelLiters = State.FuelLiters;
    Quote.FuelCapacityLiters = Vehicle->GetFuelCapacityLiters();
    Quote.BodyDamageSurcharge = Vehicle->GetBodyDamageRepairSurcharge();
    Quote.bAtWorkshop = IsVehicleAtWorkshop(Vehicle);
    Quote.bServiceNeeded = Vehicle->NeedsNativeWorkshopService();
    Quote.bOwnedByPlayer = State.bOwnedByPlayer;

    if (const UWorld* World = GetWorld())
    {
        if (const UGTTBreakdownDecisionSubsystem* Decision = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>())
        {
            Quote.TotalCost = Quote.bServiceNeeded ? Decision->CalculateRepairEstimate(Vehicle) : 0;
        }
    }
    return Quote;
}

AGTTRoadVehicleNativePawn* UGTTWorkshopRecoverySubsystem::FindNearbyWorkshopVehicle(const APawn* CustomerPawn) const
{
    const UWorld* World = GetWorld();
    if (!World || !CustomerPawn) return nullptr;

    AGTTRoadVehicleNativePawn* Best = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!Candidate || !Candidate->IsLegacyTakeoverActive()) continue;
        const FGTTRoadVehicleMigrationSnapshot State = Candidate->GetMigrationSnapshot();
        if (!State.bOwnedByPlayer || Candidate->GetPersistentVehicleId().IsNone()) continue;
        if (!IsVehicleAtWorkshop(Candidate)) continue;

        const float DistanceSquared = FVector::DistSquared2D(CustomerPawn->GetActorLocation(), Candidate->GetActorLocation());
        if (DistanceSquared > FMath::Square(WorkshopCustomerRadiusCm) || DistanceSquared >= BestDistanceSquared) continue;
        Best = Candidate;
        BestDistanceSquared = DistanceSquared;
    }
    return Best;
}

bool UGTTWorkshopRecoverySubsystem::ValidateWorkshopCustomer(
    const AGTTRoadVehicleNativePawn* Vehicle,
    const APawn* CustomerPawn,
    FString& OutReason) const
{
    if (!Vehicle || !CustomerPawn)
    {
        OutReason = TEXT("INVALID_TARGET");
        return false;
    }
    if (!Vehicle->IsLegacyTakeoverActive())
    {
        OutReason = TEXT("NATIVE_TAKEOVER_INACTIVE");
        return false;
    }
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    if (!State.bOwnedByPlayer || Vehicle->GetPersistentVehicleId().IsNone())
    {
        OutReason = TEXT("NOT_OWNED");
        return false;
    }
    if (!IsVehicleAtWorkshop(Vehicle))
    {
        OutReason = TEXT("NOT_AT_WORKSHOP");
        return false;
    }
    if (FVector::DistSquared2D(CustomerPawn->GetActorLocation(), Vehicle->GetActorLocation()) > FMath::Square(WorkshopCustomerRadiusCm))
    {
        OutReason = TEXT("CUSTOMER_TOO_FAR");
        return false;
    }
    if (const UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(const_cast<APawn*>(CustomerPawn)))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            OutReason = TEXT("WANTED_ACTIVE");
            return false;
        }
    }
    if (!Vehicle->NeedsNativeWorkshopService())
    {
        OutReason = TEXT("SERVICE_NOT_NEEDED");
        return false;
    }
    OutReason = TEXT("OK");
    return true;
}

bool UGTTWorkshopRecoverySubsystem::PurchaseFullWorkshopService(
    AGTTRoadVehicleNativePawn* Vehicle,
    APawn* CustomerPawn)
{
    FString RejectReason;
    UGTTPlayerEconomyComponent* Economy = CustomerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(CustomerPawn) : nullptr;
    if (!ValidateWorkshopCustomer(Vehicle, CustomerPawn, RejectReason) || !Economy)
    {
        if (Economy)
        {
            Economy->PushMessage(FString::Printf(TEXT("Workshop service unavailable: %s."), *RejectReason), 5.0f);
        }
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_WORKSHOP_SERVICE_REJECTED vehicle=%s reason=%s charged=NO"),
            Vehicle ? *Vehicle->GetPersistentVehicleId().ToString() : TEXT("NONE"), *RejectReason);
        return false;
    }

    const FGTTWorkshopServiceQuote Quote = GetWorkshopServiceQuote(Vehicle);
    const FName ExpectedVehicleId = Quote.PersistentVehicleId;
    if (Quote.TotalCost <= 0 || Economy->GetCash() < Quote.TotalCost)
    {
        Economy->PushMessage(FString::Printf(TEXT("Workshop quote is $%d. You do not have enough cash."), Quote.TotalCost), 6.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_WORKSHOP_SERVICE_REJECTED vehicle=%s cost=%d reason=INSUFFICIENT_CASH charged=NO"),
            *ExpectedVehicleId.ToString(), Quote.TotalCost);
        return false;
    }

    const FGTTRoadVehicleMigrationSnapshot Before = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyBefore = Vehicle->GetBodyDamageSnapshot();
    const int32 DetachedMaskBefore = Vehicle->GetDetachedPanelMask();
    const float CargoLoadBefore = Vehicle->GetCargoLoadFactor();

    if (!Economy->SpendCash(Quote.TotalCost, TEXT("Workshop full vehicle service")))
    {
        Economy->PushMessage(TEXT("Workshop transaction failed before service. No vehicle state was changed."), 5.0f);
        return false;
    }

    const bool bApplied = Vehicle->ApplyNativeWorkshopService();
    const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot BodyAfter = Vehicle->GetBodyDamageSnapshot();
    const bool bIdentityPreserved = Vehicle->GetPersistentVehicleId() == ExpectedVehicleId;
    const bool bCargoPreserved = FMath::IsNearlyEqual(CargoLoadBefore, Vehicle->GetCargoLoadFactor(), 0.001f);
    const bool bFullyServiced =
        After.ConditionPercent >= 0.999f
        && After.TireIntegrity >= 0.999f
        && After.FuelLiters + 0.01f >= Vehicle->GetFuelCapacityLiters()
        && BodyAfter.FrontHealth >= 0.999f
        && BodyAfter.RearHealth >= 0.999f
        && BodyAfter.LeftHealth >= 0.999f
        && BodyAfter.RightHealth >= 0.999f
        && BodyAfter.DetachedPanelCount == 0
        && Vehicle->GetDetachedPanelMask() == 0
        && !Vehicle->NeedsNativeWorkshopService();
    const bool bSuccess = bApplied && bIdentityPreserved && bCargoPreserved && bFullyServiced;

    if (!bSuccess)
    {
        Vehicle->RestorePersistentMigrationSnapshot(Before);
        Vehicle->RestorePersistentBodyDamage(BodyBefore, DetachedMaskBefore);
        Vehicle->SetCargoLoadFactor(CargoLoadBefore);
        Vehicle->FlushNativePersistenceMirror();
        Economy->AddCash(Quote.TotalCost, TEXT("Workshop verification failed — service refunded"));
        Economy->PushMessage(TEXT("Workshop verification failed. Vehicle state was rolled back and the charge was refunded."), 8.0f);
        UE_LOG(LogGTT, Error,
            TEXT("NATIVE_WORKSHOP_SERVICE_COMPLETE vehicle=%s cost=%d result=FAIL identity_preserved=%s cargo_preserved=%s fully_serviced=%s rollback=YES refund=YES"),
            *ExpectedVehicleId.ToString(), Quote.TotalCost,
            bIdentityPreserved ? TEXT("YES") : TEXT("NO"),
            bCargoPreserved ? TEXT("YES") : TEXT("NO"),
            bFullyServiced ? TEXT("YES") : TEXT("NO"));
        return false;
    }

    Vehicle->FlushNativePersistenceMirror();
    Economy->PushMessage(FString::Printf(
        TEXT("Workshop service complete: $%d. Mechanical condition, tires, fuel and body restored on %s."),
        Quote.TotalCost, *ExpectedVehicleId.ToString()), 8.0f);
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_WORKSHOP_SERVICE_COMPLETE vehicle=%s cost=%d result=PASS identity_preserved=YES cargo_preserved=YES body_restored=YES fuel_full=YES tires_full=YES persistence_mirror=FLUSHED"),
        *ExpectedVehicleId.ToString(), Quote.TotalCost);
    return true;
}

void UGTTWorkshopRecoverySubsystem::MaybeShowWorkshopPrompt(APawn* CustomerPawn, AGTTRoadVehicleNativePawn* Vehicle)
{
    if (!CustomerPawn || !Vehicle || PromptCooldown > 0.0f) return;
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(CustomerPawn);
    if (!Economy) return;

    const FGTTWorkshopServiceQuote Quote = GetWorkshopServiceQuote(Vehicle);
    if (!Quote.bServiceNeeded) return;

    FString Reason;
    if (!ValidateWorkshopCustomer(Vehicle, CustomerPawn, Reason))
    {
        if (Reason == TEXT("WANTED_ACTIVE"))
        {
            Economy->PushMessage(TEXT("Workshop locked while police are searching. Clear Wanted before servicing the vehicle."), 5.0f);
            PromptCooldown = PromptCooldownSeconds;
        }
        return;
    }

    Economy->PushMessage(FString::Printf(
        TEXT("WORKSHOP | %s | full service $%d | H to repair/refuel. Tow charge is separate."),
        *Quote.PersistentVehicleId.ToString(), Quote.TotalCost), 6.0f);
    PromptCooldown = PromptCooldownSeconds;
}
