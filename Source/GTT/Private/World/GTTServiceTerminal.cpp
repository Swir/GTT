#include "World/GTTServiceTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "GTT.h"

namespace
{
    const FName FieldmasterVehicleId(TEXT("RustyFieldmaster60"));

    AGTTFieldmasterNativePawn* FindActiveNativeFieldmaster(UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World) return nullptr;
        AGTTFieldmasterNativePawn* Best = nullptr;
        float BestDistanceSquared = FMath::Square(Radius);
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            AGTTFieldmasterNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive()) continue;
            const float DistanceSquared = FVector::DistSquared(Origin, Native->GetActorLocation());
            if (DistanceSquared <= BestDistanceSquared) { BestDistanceSquared = DistanceSquared; Best = Native; }
        }
        return Best;
    }

    AGTTRoadVehicleNativePawn* FindActiveNativeRoadVehicle(UWorld* World, const FVector& Origin, float Radius)
    {
        if (!World) return nullptr;
        AGTTRoadVehicleNativePawn* Best = nullptr;
        float BestDistanceSquared = FMath::Square(Radius);
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            AGTTRoadVehicleNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsLegacyTakeoverActive()) continue;
            const FGTTRoadVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
            if (!State.bOwnedByPlayer || Native->GetPersistentVehicleId().IsNone()) continue;
            const float DistanceSquared = FVector::DistSquared(Origin, Native->GetActorLocation());
            if (DistanceSquared <= BestDistanceSquared) { BestDistanceSquared = DistanceSquared; Best = Native; }
        }
        return Best;
    }

    bool HasActiveNativeRoadTakeover(UWorld* World, FName VehicleId)
    {
        if (!World || VehicleId.IsNone()) return false;
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
        {
            const AGTTRoadVehicleNativePawn* Native = *It;
            if (IsValid(Native) && Native->IsLegacyTakeoverActive() && Native->GetPersistentVehicleId() == VehicleId) return true;
        }
        return false;
    }

    bool NativeRoadNeedsMechanicalService(const AGTTRoadVehicleNativePawn* Vehicle)
    {
        if (!Vehicle) return false;
        const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
        const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
        const bool bBodyDamaged = Body.FrontHealth < 0.999f || Body.RearHealth < 0.999f ||
            Body.LeftHealth < 0.999f || Body.RightHealth < 0.999f || Body.CoolingStress > 0.01f || Body.DetachedPanelCount > 0;
        return State.ConditionPercent < 0.999f || State.TireIntegrity < 0.999f || bBodyDamaged;
    }

    bool NativeRoadFullyServiced(const AGTTRoadVehicleNativePawn* Vehicle)
    {
        if (!Vehicle) return false;
        const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
        const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
        return State.ConditionPercent >= 0.999f
            && State.TireIntegrity >= 0.999f
            && State.FuelLiters + 0.01f >= Vehicle->GetFuelCapacityLiters()
            && Body.FrontHealth >= 0.999f
            && Body.RearHealth >= 0.999f
            && Body.LeftHealth >= 0.999f
            && Body.RightHealth >= 0.999f
            && Body.DetachedPanelCount == 0
            && Vehicle->GetDetachedPanelMask() == 0
            && !Vehicle->NeedsNativeWorkshopService();
    }

    AGTTVehicleBase* FindFieldmasterMirror(UWorld* World, const AGTTFieldmasterNativePawn* Native)
    {
        if (!World || !Native) return nullptr;
        AGTTVehicleBase* Best = nullptr;
        float BestDistanceSquared = TNumericLimits<float>::Max();
        for (TActorIterator<AGTTVehicleBase> It(World); It; ++It)
        {
            AGTTVehicleBase* Vehicle = *It;
            if (!IsValid(Vehicle) || Vehicle->GetPersistentVehicleId() != FieldmasterVehicleId) continue;
            const float DistanceSquared = FVector::DistSquared(Native->GetActorLocation(), Vehicle->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared) { BestDistanceSquared = DistanceSquared; Best = Vehicle; }
        }
        return Best;
    }
}

AGTTServiceTerminal::AGTTServiceTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
    SetRootComponent(TerminalMesh);
    TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TerminalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TerminalMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.9f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) TerminalMesh->SetStaticMesh(CubeFinder.Object);
}

int32 AGTTServiceTerminal::GetNativeRoadRepairQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle || !GetWorld()) return WorkshopServiceCost;
    const UGTTBreakdownDecisionSubsystem* Decision = GetWorld()->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    return Decision ? Decision->CalculateRepairEstimate(Vehicle, WorkshopServiceCost) : WorkshopServiceCost + Vehicle->GetBodyDamageRepairSurcharge();
}

int32 AGTTServiceTerminal::GetNativeRoadFuelQuote(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    if (!Vehicle) return 0;
    const FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    const float MissingLiters = FMath::Max(0.0f, Vehicle->GetFuelCapacityLiters() - State.FuelLiters);
    return MissingLiters <= KINDA_SMALL_NUMBER ? 0 : FMath::Max(1, FMath::CeilToInt(MissingLiters * NativeFuelPricePerLiter));
}

bool AGTTServiceTerminal::PurchaseNativeRoadWorkshopService(AGTTRoadVehicleNativePawn* Vehicle, APawn* CustomerPawn)
{
    UGTTPlayerEconomyComponent* Economy = CustomerPawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(CustomerPawn) : nullptr;
    if (!Vehicle || !CustomerPawn || !Economy || !Vehicle->IsLegacyTakeoverActive()) return false;

    const FGTTRoadVehicleMigrationSnapshot Before = Vehicle->GetMigrationSnapshot();
    const FName ExpectedVehicleId = Vehicle->GetPersistentVehicleId();
    if (!Before.bOwnedByPlayer || ExpectedVehicleId.IsNone())
    {
        Economy->PushMessage(TEXT("Workshop: this vehicle is not registered to your garage."), 5.0f);
        UE_LOG(LogGTT, Warning, TEXT("NATIVE_WORKSHOP_SERVICE_REJECTED vehicle=%s reason=NOT_OWNED charged=NO"), *ExpectedVehicleId.ToString());
        return false;
    }

    if (FVector::DistSquared(GetActorLocation(), Vehicle->GetActorLocation()) > FMath::Square(VehicleSearchRadius))
    {
        Economy->PushMessage(TEXT("Workshop: park the registered vehicle beside the service bay."), 5.0f);
        return false;
    }

    if (const UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(CustomerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            Economy->PushMessage(TEXT("Workshop locked while police are searching. Clear Wanted before voluntary service."), 6.0f);
            UE_LOG(LogGTT, Warning, TEXT("NATIVE_WORKSHOP_SERVICE_REJECTED vehicle=%s reason=WANTED charged=NO"), *ExpectedVehicleId.ToString());
            return false;
        }
    }

    const bool bNeedsMechanical = NativeRoadNeedsMechanicalService(Vehicle);
    const bool bNeedsFuel = Before.FuelLiters + KINDA_SMALL_NUMBER < Vehicle->GetFuelCapacityLiters();
    if (!bNeedsMechanical && !bNeedsFuel)
    {
        Economy->PushMessage(TEXT("Workshop: that Native road vehicle is already ready to go."));
        return false;
    }

    const float CargoLoadBefore = Vehicle->GetCargoLoadFactor();
    const FGTTRoadBodyDamageSnapshot BodyBefore = Vehicle->GetBodyDamageSnapshot();
    const int32 DetachedMaskBefore = Vehicle->GetDetachedPanelMask();
    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));

    if (!bNeedsMechanical && bNeedsFuel)
    {
        const int32 FuelCost = GetNativeRoadFuelQuote(Vehicle);
        const float MissingLiters = FMath::Max(0.0f, Vehicle->GetFuelCapacityLiters() - Before.FuelLiters);
        if (FuelCost <= 0 || !Economy->SpendCash(FuelCost, FString::Printf(TEXT("%s exact fuel quote - $%d"), *Vehicle->GetVehicleDisplayName().ToString(), FuelCost))) return false;

        const float Added = Vehicle->RefuelNativeVehicle(MissingLiters);
        const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
        const bool bIdentityPreserved = Vehicle->GetPersistentVehicleId() == ExpectedVehicleId;
        const bool bCargoPreserved = FMath::IsNearlyEqual(CargoLoadBefore, Vehicle->GetCargoLoadFactor(), 0.001f);
        const bool bFuelVerified = Added > KINDA_SMALL_NUMBER
            && After.FuelLiters > Before.FuelLiters
            && After.FuelLiters + 0.01f >= Vehicle->GetFuelCapacityLiters();
        if (!bIdentityPreserved || !bCargoPreserved || !bFuelVerified)
        {
            Vehicle->RestorePersistentMigrationSnapshot(Before);
            Vehicle->SetCargoLoadFactor(CargoLoadBefore);
            Vehicle->FlushNativePersistenceMirror();
            Economy->AddCash(FuelCost, TEXT("Native road refuel verification rollback"));
            Economy->PushMessage(TEXT("Workshop: refuel verification failed; vehicle state rolled back and payment returned."), 7.0f);
            UE_LOG(LogGTT, Error,
                TEXT("NATIVE_WORKSHOP_REFUEL_COMPLETE vehicle=%s cost=%d result=FAIL identity_preserved=%s cargo_preserved=%s fuel_verified=%s refund=YES"),
                *ExpectedVehicleId.ToString(), FuelCost,
                bIdentityPreserved ? TEXT("YES") : TEXT("NO"),
                bCargoPreserved ? TEXT("YES") : TEXT("NO"),
                bFuelVerified ? TEXT("YES") : TEXT("NO"));
            return false;
        }

        Vehicle->FlushNativePersistenceMirror();
        Economy->PushMessage(FString::Printf(TEXT("%s refuelled %.1f L for $%d. Mechanical state and cargo authority were preserved."), *Vehicle->GetVehicleDisplayName().ToString(), Added, FuelCost), 6.0f);
        UE_LOG(LogGTT, Display, TEXT("NATIVE_WORKSHOP_REFUEL_COMPLETE vehicle=%s cost=%d result=PASS identity_preserved=YES cargo_preserved=YES persistence=SAVE"), *ExpectedVehicleId.ToString(), FuelCost);
        if (GameMode) GameMode->SaveProgress();
        return true;
    }

    const int32 BodyParts = Vehicle->GetBodyDamageRepairSurcharge();
    const int32 TotalCost = GetNativeRoadRepairQuote(Vehicle);
    if (TotalCost <= 0 || !Economy->SpendCash(TotalCost, FString::Printf(TEXT("Native road workshop exact quote - $%d"), TotalCost))) return false;

    const bool bApplied = Vehicle->ApplyNativeWorkshopService();
    const bool bIdentityPreserved = Vehicle->GetPersistentVehicleId() == ExpectedVehicleId;
    const bool bCargoPreserved = FMath::IsNearlyEqual(CargoLoadBefore, Vehicle->GetCargoLoadFactor(), 0.001f);
    const bool bFullyServiced = NativeRoadFullyServiced(Vehicle);
    if (!bApplied || !bIdentityPreserved || !bCargoPreserved || !bFullyServiced)
    {
        Vehicle->RestorePersistentMigrationSnapshot(Before);
        Vehicle->RestorePersistentBodyDamage(BodyBefore, DetachedMaskBefore);
        Vehicle->SetCargoLoadFactor(CargoLoadBefore);
        Vehicle->FlushNativePersistenceMirror();
        Economy->AddCash(TotalCost, TEXT("Native road workshop verification rollback"));
        Economy->PushMessage(TEXT("Workshop: service verification failed; vehicle state rolled back and payment returned."), 8.0f);
        UE_LOG(LogGTT, Error,
            TEXT("NATIVE_WORKSHOP_SERVICE_COMPLETE vehicle=%s cost=%d result=FAIL identity_preserved=%s cargo_preserved=%s fully_serviced=%s rollback=YES refund=YES"),
            *ExpectedVehicleId.ToString(), TotalCost,
            bIdentityPreserved ? TEXT("YES") : TEXT("NO"),
            bCargoPreserved ? TEXT("YES") : TEXT("NO"),
            bFullyServiced ? TEXT("YES") : TEXT("NO"));
        return false;
    }

    Vehicle->FlushNativePersistenceMirror();
    Economy->PushMessage(FString::Printf(
        TEXT("%s repaired + refuelled for $%d (structural parts $%d). Exact vehicle and cargo authority preserved."),
        *Vehicle->GetVehicleDisplayName().ToString(), TotalCost, BodyParts), 7.0f);
    UE_LOG(LogGTT, Display,
        TEXT("NATIVE_WORKSHOP_SERVICE_COMPLETE vehicle=%s cost=%d result=PASS identity_preserved=YES cargo_preserved=YES body_restored=YES fuel_full=YES tires_full=YES persistence=SAVE"),
        *ExpectedVehicleId.ToString(), TotalCost);
    if (GameMode) GameMode->SaveProgress();
    return true;
}

void AGTTServiceTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Economy) return;

    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        Economy->SellAllFish(FishPricePerKg);
        return;
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        PurchaseNativeRoadWorkshopService(NativeRoad, Pawn);
        return;
    }

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        AGTTVehicleBase* Mirror = FindFieldmasterMirror(GetWorld(), Native);
        if (!Mirror) { Economy->PushMessage(TEXT("Workshop: Native Fieldmaster compatibility mirror is unavailable.")); return; }
        const FGTTVehicleMigrationSnapshot State = Native->GetMigrationSnapshot();
        const bool bNeedsRepair = State.ConditionPercent < 0.999f;
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < Mirror->GetFuelCapacity();
        if (!bNeedsRepair && !bNeedsFuel) { Economy->PushMessage(TEXT("Workshop: that machine is already ready to go.")); return; }
        if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost))) return;
        Mirror->RepairVehicle(100000.0f);
        Mirror->RefuelVehicle(100000.0f);
        FString ImportSummary;
        if (!Native->ImportLegacyGameplayState(Mirror, ImportSummary))
        {
            Economy->AddCash(WorkshopServiceCost, TEXT("Workshop service rollback"));
            Economy->PushMessage(TEXT("Workshop: Native Fieldmaster state refresh failed; payment returned."));
            return;
        }
        Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled; Native Chaos state synchronized."), *Native->GetVehicleDisplayName().ToString()), 5.0f);
        if (GameMode) GameMode->SaveProgress();
        return;
    }

    AGTTVehicleBase* Vehicle = FindNearestVehicle();
    if (!Vehicle) { Economy->PushMessage(TEXT("Workshop: park a vehicle nearby first.")); return; }
    const bool bNeedsRepair = Vehicle->GetConditionPercent() < 0.999f;
    const bool bNeedsFuel = Vehicle->GetFuelPercent() < 0.999f;
    if (!bNeedsRepair && !bNeedsFuel) { Economy->PushMessage(TEXT("Workshop: that machine is already ready to go.")); return; }
    if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost))) return;
    Vehicle->RepairVehicle(100000.0f);
    Vehicle->RefuelVehicle(100000.0f);
    Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled."), *Vehicle->GetVehicleDisplayName().ToString()), 5.0f);
    if (GameMode) GameMode->SaveProgress();
}

FText AGTTServiceTerminal::GetInteractionText_Implementation() const
{
    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        return NSLOCTEXT("GTT", "SellFish", "Sell all fish");
    }

    if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        const FGTTRoadVehicleMigrationSnapshot State = NativeRoad->GetMigrationSnapshot();
        const bool bNeedsMechanical = NativeRoadNeedsMechanicalService(NativeRoad);
        const bool bNeedsFuel = State.FuelLiters + KINDA_SMALL_NUMBER < NativeRoad->GetFuelCapacityLiters();
        const FString VehicleTag = FString::Printf(TEXT("%s [%s]"), *NativeRoad->GetVehicleDisplayName().ToString(), *NativeRoad->GetPersistentVehicleId().ToString());
        if (!bNeedsMechanical && !bNeedsFuel)
            return FText::FromString(FString::Printf(TEXT("Workshop: %s is ready"), *VehicleTag));
        if (!bNeedsMechanical && bNeedsFuel)
            return FText::FromString(FString::Printf(TEXT("Refuel %s ($%d exact fuel quote)"), *VehicleTag, GetNativeRoadFuelQuote(NativeRoad)));
        return FText::FromString(FString::Printf(TEXT("Workshop: repair + refuel %s ($%d exact quote)"), *VehicleTag, GetNativeRoadRepairQuote(NativeRoad)));
    }

    if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster(GetWorld(), GetActorLocation(), VehicleSearchRadius))
    {
        return FText::FromString(FString::Printf(TEXT("Workshop: inspect %s ($%d base service)"), *Native->GetVehicleDisplayName().ToString(), WorkshopServiceCost));
    }

    return NSLOCTEXT("GTT", "WorkshopService", "Inspect + repair nearby vehicle (damage-based quote)");
}

AGTTVehicleBase* AGTTServiceTerminal::FindNearestVehicle() const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* BestVehicle = nullptr;
    float BestDistanceSquared = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!IsValid(Vehicle)) continue;
        if (Vehicle->GetPersistentVehicleId() == FieldmasterVehicleId)
        {
            bool bNativeTakeoverActive = false;
            for (TActorIterator<AGTTFieldmasterNativePawn> NativeIt(GetWorld()); NativeIt; ++NativeIt)
            {
                if (IsValid(*NativeIt) && NativeIt->IsLegacyTakeoverActive()) { bNativeTakeoverActive = true; break; }
            }
            if (bNativeTakeoverActive) continue;
        }
        if (HasActiveNativeRoadTakeover(GetWorld(), Vehicle->GetPersistentVehicleId())) continue;
        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared) { BestDistanceSquared = DistanceSquared; BestVehicle = Vehicle; }
    }
    return BestVehicle;
}
