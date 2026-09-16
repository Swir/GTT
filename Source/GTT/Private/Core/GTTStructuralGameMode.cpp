#include "Core/GTTStructuralGameMode.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTT.h"

namespace
{
    constexpr int32 StructuralSaveVersion = 5;
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

    Save->SaveVersion = FMath::Max(Save->SaveVersion, StructuralSaveVersion);
    Save->RoadStructuralDamage.Reset();

    if (GetWorld())
    {
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
    UE_LOG(LogGTT, bSaved ? Log : Error,
        TEXT("STRUCTURAL_SAVE result=%s version=%d vehicles=%d"),
        bSaved ? TEXT("PASS") : TEXT("FAIL"), Save->SaveVersion, Save->RoadStructuralDamage.Num());
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

    int32 RestoredCount = 0;
    for (const TWeakObjectPtr<AGTTRoadVehicleNativePawn>& WeakNative : NativeVehicles)
    {
        AGTTRoadVehicleNativePawn* Native = WeakNative.Get();
        if (!Native || !Native->IsNativeReady()) continue;

        FGTTRoadBodyDamageSnapshot Body;
        int32 PanelMask = 0;
        if (Save->SaveVersion >= StructuralSaveVersion)
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

    UE_LOG(LogGTT, Log,
        TEXT("STRUCTURAL_LOAD result=PASS version=%d records=%d restored=%d"),
        Save->SaveVersion, Save->RoadStructuralDamage.Num(), RestoredCount);
    return true;
}
