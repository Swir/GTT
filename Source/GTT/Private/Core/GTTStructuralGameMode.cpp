#include "Core/GTTStructuralGameMode.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTLogisticsReputationSubsystem.h"
#include "GTT.h"

namespace
{
    constexpr int32 StructuralDamageSaveVersion = 5;
    constexpr int32 FleetDispatchSaveVersion = 6;
    constexpr int32 MissionLoadoutSaveVersion = 7;
    constexpr int32 LogisticsReputationSaveVersion = 8;
    constexpr int32 ExtendedSaveVersion = 8;
}

bool AGTTStructuralGameMode::SaveProgress()
{
    if (GetWorld())
    {
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(GetWorld()); It; ++It)
        {
            if (AGTTRoadVehicleNativePawn* Native = *It)
            {
                Native->FlushNativePersistenceMirror();
            }
        }
    }

    if (!Super::SaveProgress())
    {
        return false;
    }

    UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Save)
    {
        UE_LOG(LogGTT, Error, TEXT("STRUCTURAL_SAVE failed: primary save could not be reopened."));
        return false;
    }

    Save->SaveVersion = FMath::Max(Save->SaveVersion, ExtendedSaveVersion);
    Save->RoadStructuralDamage.Reset();

    if (GetWorld())
    {
        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            Save->PreferredGarageVehicleId = Fleet->GetPreferredVehicleId();
            Save->PreferredTractorVehicleId = Fleet->GetRoleLoadoutVehicleId(EGTTGarageFleetRole::Tractor);
            Save->PreferredRoadVehicleId = Fleet->GetRoleLoadoutVehicleId(EGTTGarageFleetRole::Road);
            Save->PreferredCargoVehicleId = Fleet->GetRoleLoadoutVehicleId(EGTTGarageFleetRole::Cargo);
        }
        if (const UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
        {
            Logistics->CaptureToSave(Save);
        }

        for (TActorIterator<AGTTRoadVehicleNativePawn> It(GetWorld()); It; ++It)
        {
            AGTTRoadVehicleNativePawn* Native = *It;
            if (!IsValid(Native) || !Native->IsNativeReady() || Native->GetPersistentVehicleId().IsNone())
            {
                continue;
            }

            const FGTTRoadBodyDamageSnapshot Body = Native->GetBodyDamageSnapshot();
            FGTTStoredRoadStructuralDamageData Stored;
            Stored.VehicleId = Native->GetPersistentVehicleId();
            Stored.FrontHealth = Body.FrontHealth;
            Stored.RearHealth = Body.RearHealth;
            Stored.LeftHealth = Body.LeftHealth;
            Stored.RightHealth = Body.RightHealth;
            Stored.CoolingStress = Body.CoolingStress;
            Stored.DetachedPanelMask = Native->GetDetachedPanelMask();
            Save->RoadStructuralDamage.Add(Stored);
        }
    }

    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
    if (bSaved)
    {
        UE_LOG(LogGTT, Log, TEXT("STRUCTURAL_SAVE result=PASS version=%d vehicles=%d preferred=%s loadouts=T:%s R:%s C:%s logistics_rep=%d streak=%d"),
            Save->SaveVersion, Save->RoadStructuralDamage.Num(), *Save->PreferredGarageVehicleId.ToString(),
            *Save->PreferredTractorVehicleId.ToString(), *Save->PreferredRoadVehicleId.ToString(), *Save->PreferredCargoVehicleId.ToString(),
            Save->LogisticsReputation, Save->LogisticsCleanStreak);
    }
    else
    {
        UE_LOG(LogGTT, Error, TEXT("STRUCTURAL_SAVE result=FAIL version=%d vehicles=%d preferred=%s"),
            Save->SaveVersion, Save->RoadStructuralDamage.Num(), *Save->PreferredGarageVehicleId.ToString());
    }
    return bSaved;
}

bool AGTTStructuralGameMode::LoadProgress()
{
    TArray<TWeakObjectPtr<AGTTRoadVehicleNativePawn>> NativeVehicles;
    if (GetWorld())
    {
        for (TActorIterator<AGTTRoadVehicleNativePawn> It(GetWorld()); It; ++It)
        {
            AGTTRoadVehicleNativePawn* Native = *It;
            if (!IsValid(Native)) continue;
            NativeVehicles.Add(Native);
            if (Native->IsLegacyTakeoverActive())
            {
                Native->DeactivateLegacyTakeover();
            }
        }
    }

    if (!Super::LoadProgress())
    {
        return false;
    }

    const UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Save)
    {
        UE_LOG(LogGTT, Error, TEXT("STRUCTURAL_LOAD failed: primary save unavailable after base load."));
        return false;
    }

    if (GetWorld())
    {
        if (UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            Fleet->RestorePreferredVehicleId(Save->SaveVersion >= FleetDispatchSaveVersion ? Save->PreferredGarageVehicleId : NAME_None);
            if (Save->SaveVersion >= MissionLoadoutSaveVersion)
            {
                Fleet->RestoreRoleLoadouts(Save->PreferredTractorVehicleId, Save->PreferredRoadVehicleId, Save->PreferredCargoVehicleId);
            }
            else
            {
                Fleet->RestoreRoleLoadouts(NAME_None, NAME_None, NAME_None);
            }
        }
        if (UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>())
        {
            Logistics->RestoreFromSave(Save->SaveVersion >= LogisticsReputationSaveVersion ? Save : nullptr);
        }
    }

    int32 RestoredCount = 0;
    for (const TWeakObjectPtr<AGTTRoadVehicleNativePawn>& WeakNative : NativeVehicles)
    {
        AGTTRoadVehicleNativePawn* Native = WeakNative.Get();
        if (!Native || !Native->IsNativeReady()) continue;

        FGTTRoadBodyDamageSnapshot Body;
        int32 PanelMask = 0;
        if (Save->SaveVersion >= StructuralDamageSaveVersion)
        {
            const FGTTStoredRoadStructuralDamageData* Stored = Save->RoadStructuralDamage.FindByPredicate(
                [Native](const FGTTStoredRoadStructuralDamageData& Data)
                {
                    return Data.VehicleId == Native->GetPersistentVehicleId();
                });
            if (Stored)
            {
                Body.FrontHealth = Stored->FrontHealth;
                Body.RearHealth = Stored->RearHealth;
                Body.LeftHealth = Stored->LeftHealth;
                Body.RightHealth = Stored->RightHealth;
                Body.CoolingStress = Stored->CoolingStress;
                PanelMask = Stored->DetachedPanelMask;
                ++RestoredCount;
            }
        }

        Native->RestorePersistentBodyDamage(Body, PanelMask);
        Native->TryActivateLegacyTakeover();
    }

    const UGTTGarageFleetSubsystem* Fleet = GetWorld() ? GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>() : nullptr;
    const UGTTLogisticsReputationSubsystem* Logistics = GetWorld() ? GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>() : nullptr;
    UE_LOG(LogGTT, Log,
        TEXT("STRUCTURAL_LOAD result=PASS version=%d records=%d restored=%d preferred=%s loadouts=T:%s R:%s C:%s logistics_rep=%d streak=%d"),
        Save->SaveVersion, Save->RoadStructuralDamage.Num(), RestoredCount,
        Fleet ? *Fleet->GetPreferredVehicleId().ToString() : TEXT("None"),
        Fleet ? *Fleet->GetRoleLoadoutVehicleId(EGTTGarageFleetRole::Tractor).ToString() : TEXT("None"),
        Fleet ? *Fleet->GetRoleLoadoutVehicleId(EGTTGarageFleetRole::Road).ToString() : TEXT("None"),
        Fleet ? *Fleet->GetRoleLoadoutVehicleId(EGTTGarageFleetRole::Cargo).ToString() : TEXT("None"),
        Logistics ? Logistics->GetReputation() : 0,
        Logistics ? Logistics->GetCleanStreak() : 0);
    return true;
}
