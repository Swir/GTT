#include "Activities/GTTFarmJobDirector.h"
#include "GTT.h"

#include "Save/GTTSaveGame.h"
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"

namespace
{
void ResetPersistedCargoSnapshot(UGTTSaveGame* Save)
{
    if (!Save) return;
    Save->bFarmCargoContractActive = false;
    Save->FarmCargoStage = static_cast<uint8>(EGTTFarmJobStage::Idle);
    Save->FarmCargoTimeRemaining = 0.0f;
    Save->FarmCargoIntegrity = 1.0f;
    Save->FarmCargoFleetPayoutMultiplier = 1.0f;
    Save->FarmCargoMarketMultiplier = 1.0f;
    Save->FarmCargoRouteTier = 1;
    Save->FarmCargoUnitsReserved = 0;
    Save->FarmCargoCommodity = TEXT("ANIMAL FEED");
    Save->FarmCargoPriority = TEXT("HILL FARM DIRECT");
    Save->bFarmCargoPoliceIncident = false;
    Save->FarmCargoBoundVehicleId = NAME_None;
}
}

void AGTTFarmJobDirector::CaptureActiveCargoToSave(UGTTSaveGame* Save) const
{
    if (!Save) return;
    if (Stage == EGTTFarmJobStage::Idle)
    {
        ResetPersistedCargoSnapshot(Save);
        return;
    }

    Save->bFarmCargoContractActive = true;
    Save->FarmCargoStage = static_cast<uint8>(Stage);
    Save->FarmCargoTimeRemaining = FMath::Max(0.0f, TimeRemaining);
    Save->FarmCargoIntegrity = FMath::Clamp(CargoIntegrity, 0.0f, 1.0f);
    Save->FarmCargoFleetPayoutMultiplier = FMath::Clamp(FleetPayoutMultiplier, 0.0f, 2.0f);
    Save->FarmCargoMarketMultiplier = FMath::Clamp(MarketMultiplierAtStart, 0.0f, 2.0f);
    Save->FarmCargoRouteTier = FMath::Clamp(RouteTierAtStart, 1, 3);
    Save->FarmCargoUnitsReserved = FMath::Max(0, CargoUnitsReserved);
    Save->FarmCargoCommodity = CargoCommodityAtStart;
    Save->FarmCargoPriority = CargoPriorityAtStart;
    Save->bFarmCargoPoliceIncident = bPoliceIncidentDuringRun;
}

void AGTTFarmJobDirector::RestoreActiveCargoFromSave(const UGTTSaveGame* Save)
{
    ClearLoadedVehicleCargoState();

    if (!Save || !Save->bFarmCargoContractActive)
    {
        Stage = EGTTFarmJobStage::Idle;
        TimeRemaining = 0.0f;
        CargoIntegrity = 1.0f;
        FleetPayoutMultiplier = 1.0f;
        MarketMultiplierAtStart = 1.0f;
        RouteTierAtStart = 1;
        CargoUnitsReserved = 0;
        CargoCommodityAtStart = TEXT("ANIMAL FEED");
        CargoPriorityAtStart = TEXT("HILL FARM DIRECT");
        bPoliceIncidentDuringRun = false;
        return;
    }

    const uint8 FirstActiveStage = static_cast<uint8>(EGTTFarmJobStage::ReachPickup);
    const uint8 LastActiveStage = static_cast<uint8>(EGTTFarmJobStage::DeliverFinalStop);
    if (Save->FarmCargoStage < FirstActiveStage || Save->FarmCargoStage > LastActiveStage)
    {
        GTT_LOG( Warning,
            TEXT("FARM_CARGO_RECOVERY event=RESTORE result=REJECT reason=invalid_stage stage=%u"),
            Save->FarmCargoStage);
        Stage = EGTTFarmJobStage::Idle;
        TimeRemaining = 0.0f;
        CargoIntegrity = 1.0f;
        FleetPayoutMultiplier = 1.0f;
        MarketMultiplierAtStart = 1.0f;
        RouteTierAtStart = 1;
        CargoUnitsReserved = 0;
        CargoCommodityAtStart = TEXT("ANIMAL FEED");
        CargoPriorityAtStart = TEXT("HILL FARM DIRECT");
        bPoliceIncidentDuringRun = false;
        return;
    }

    Stage = static_cast<EGTTFarmJobStage>(Save->FarmCargoStage);
    TimeRemaining = FMath::Max(0.0f, Save->FarmCargoTimeRemaining);
    CargoIntegrity = FMath::Clamp(Save->FarmCargoIntegrity, 0.0f, 1.0f);
    FleetPayoutMultiplier = FMath::Clamp(Save->FarmCargoFleetPayoutMultiplier, 0.0f, 2.0f);
    MarketMultiplierAtStart = FMath::Clamp(Save->FarmCargoMarketMultiplier, 0.0f, 2.0f);
    RouteTierAtStart = FMath::Clamp(Save->FarmCargoRouteTier, 1, 3);
    CargoUnitsReserved = FMath::Max(0, Save->FarmCargoUnitsReserved);
    CargoCommodityAtStart = Save->FarmCargoCommodity.IsEmpty() ? TEXT("ANIMAL FEED") : Save->FarmCargoCommodity;
    CargoPriorityAtStart = Save->FarmCargoPriority.IsEmpty() ? TEXT("HILL FARM DIRECT") : Save->FarmCargoPriority;
    bPoliceIncidentDuringRun = Save->bFarmCargoPoliceIncident;

    GTT_LOG( Display,
        TEXT("FARM_CARGO_RECOVERY event=RESTORE result=PASS stage=%u time=%.1f integrity=%.3f tier=%d units=%d vehicle=%s"),
        Save->FarmCargoStage,
        TimeRemaining,
        CargoIntegrity,
        RouteTierAtStart,
        CargoUnitsReserved,
        *Save->FarmCargoBoundVehicleId.ToString());
}

void AGTTFarmJobDirector::AdoptRestoredCargoVehicle(APawn* Vehicle)
{
    LoadedMulebox.Reset();
    LoadedNativeMulebox.Reset();
    if (!Vehicle || (Stage != EGTTFarmJobStage::DeliverCargo && Stage != EGTTFarmJobStage::DeliverFinalStop)) return;

    LoadedNativeMulebox = Cast<AGTTMuleboxNativePawn>(Vehicle);
    LoadedMulebox = Cast<AGTTFarmVanPawn>(Vehicle);

    const float CargoLoadFactor = RouteTierAtStart >= 3 ? 1.20f : 1.0f;
    if (LoadedNativeMulebox.IsValid()) LoadedNativeMulebox->SetCargoLoadFactor(CargoLoadFactor);
    if (LoadedMulebox.IsValid()) LoadedMulebox->SetCargoLoadFactor(CargoLoadFactor);

    GTT_LOG( Display,
        TEXT("FARM_CARGO_RECOVERY event=ADOPT_VEHICLE actor=%s native_mulebox=%d legacy_mulebox=%d load_factor=%.2f"),
        *Vehicle->GetName(),
        LoadedNativeMulebox.IsValid() ? 1 : 0,
        LoadedMulebox.IsValid() ? 1 : 0,
        CargoLoadFactor);
}
