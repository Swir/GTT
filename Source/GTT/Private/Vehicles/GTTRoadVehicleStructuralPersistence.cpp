#include "Vehicles/GTTRoadVehicleNativePawn.h"

#include "Components/StaticMeshComponent.h"
#include "GTT.h"

namespace
{
    constexpr int32 FrontPanelBit = 1 << 0;
    constexpr int32 RearPanelBit = 1 << 1;
    constexpr int32 LeftPanelBit = 1 << 2;
    constexpr int32 RightPanelBit = 1 << 3;
    constexpr int32 ValidPanelMask = FrontPanelBit | RearPanelBit | LeftPanelBit | RightPanelBit;
}

int32 AGTTRoadVehicleNativePawn::GetDetachedPanelMask() const
{
    int32 Mask = 0;
    if (bFrontPanelDetached) Mask |= FrontPanelBit;
    if (bRearPanelDetached) Mask |= RearPanelBit;
    if (bLeftPanelDetached) Mask |= LeftPanelBit;
    if (bRightPanelDetached) Mask |= RightPanelBit;
    return Mask;
}

void AGTTRoadVehicleNativePawn::FlushNativePersistenceMirror()
{
    if (bTakeoverActive)
    {
        SyncLegacyMirror();
    }
}

void AGTTRoadVehicleNativePawn::RestorePersistentBodyDamage(const FGTTRoadBodyDamageSnapshot& InDamage, int32 DetachedPanelMask)
{
    RestoreNativeBodyDamage();

    BodyDamage.FrontHealth = FMath::Clamp(InDamage.FrontHealth, 0.0f, 1.0f);
    BodyDamage.RearHealth = FMath::Clamp(InDamage.RearHealth, 0.0f, 1.0f);
    BodyDamage.LeftHealth = FMath::Clamp(InDamage.LeftHealth, 0.0f, 1.0f);
    BodyDamage.RightHealth = FMath::Clamp(InDamage.RightHealth, 0.0f, 1.0f);
    BodyDamage.CoolingStress = FMath::Clamp(InDamage.CoolingStress, 0.0f, 1.0f);

    const int32 Mask = DetachedPanelMask & ValidPanelMask;
    bFrontPanelDetached = (Mask & FrontPanelBit) != 0;
    bRearPanelDetached = (Mask & RearPanelBit) != 0;
    bLeftPanelDetached = (Mask & LeftPanelBit) != 0;
    bRightPanelDetached = (Mask & RightPanelBit) != 0;
    BodyDamage.DetachedPanelCount =
        (bFrontPanelDetached ? 1 : 0) + (bRearPanelDetached ? 1 : 0) +
        (bLeftPanelDetached ? 1 : 0) + (bRightPanelDetached ? 1 : 0);

    auto RecreateDetachedDebris = [this](UStaticMeshComponent* Panel, bool bDetached, const FVector& LocalDropOffset)
    {
        if (!Panel || !bDetached) return;
        Panel->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Panel->SetWorldLocation(GetActorTransform().TransformPosition(LocalDropOffset));
        Panel->SetVisibility(true, true);
        Panel->SetHiddenInGame(false, true);
        Panel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Panel->SetSimulatePhysics(true);
    };

    RecreateDetachedDebris(FrontDamageDebris, bFrontPanelDetached, FVector(175.0f, 110.0f, 35.0f));
    RecreateDetachedDebris(RearDamageDebris, bRearPanelDetached, FVector(-175.0f, -110.0f, 35.0f));
    RecreateDetachedDebris(LeftDamageDebris, bLeftPanelDetached, FVector(-30.0f, -155.0f, 35.0f));
    RecreateDetachedDebris(RightDamageDebris, bRightPanelDetached, FVector(30.0f, 155.0f, 35.0f));

    UpdateDamageConsequences(0.0f);
    GTT_LOG( Log,
        TEXT("NATIVE_ROAD_STRUCTURAL_RESTORE vehicle=%s front=%.3f rear=%.3f left=%.3f right=%.3f cooling=%.3f panel_mask=%d detached=%d surcharge=%d"),
        *NativeVehicleId.ToString(), BodyDamage.FrontHealth, BodyDamage.RearHealth,
        BodyDamage.LeftHealth, BodyDamage.RightHealth, BodyDamage.CoolingStress,
        Mask, BodyDamage.DetachedPanelCount, GetBodyDamageRepairSurcharge());
}

bool AGTTRoadVehicleNativePawn::ApplyScriptedImpactDamage(float ImpactSpeedKmh, EGTTRoadDamageZone Zone)
{
    if (!bNativeReady || !bTakeoverActive || ImpactSpeedKmh < 14.0f)
    {
        return false;
    }

    const FGTTRoadBodyDamageSnapshot Before = BodyDamage;
    const int32 BeforeMask = GetDetachedPanelMask();
    FVector LocalHit(145.0f, 0.0f, 45.0f);
    switch (Zone)
    {
        case EGTTRoadDamageZone::Front: LocalHit = FVector(155.0f, 0.0f, 45.0f); break;
        case EGTTRoadDamageZone::Rear: LocalHit = FVector(-155.0f, 0.0f, 45.0f); break;
        case EGTTRoadDamageZone::Left: LocalHit = FVector(0.0f, -110.0f, 55.0f); break;
        case EGTTRoadDamageZone::Right: LocalHit = FVector(0.0f, 110.0f, 55.0f); break;
        default: break;
    }

    ApplyNativeImpactDamage(ImpactSpeedKmh, Zone, GetActorTransform().TransformPosition(LocalHit), FVector::ZeroVector);

    // Scripted crash events represent the same structural model used by physical NotifyHit, while
    // allowing missions/cinematics to apply a deterministic radiator shock without bypassing damage math.
    if (Zone == EGTTRoadDamageZone::Front && ImpactSpeedKmh > 45.0f)
    {
        BodyDamage.CoolingStress = FMath::Max(
            BodyDamage.CoolingStress,
            FMath::Clamp((ImpactSpeedKmh - 45.0f) / 120.0f, 0.0f, 0.55f));
        UpdateDamageConsequences(0.0f);
    }

    SyncLegacyMirror();
    const bool bChanged =
        !FMath::IsNearlyEqual(Before.FrontHealth, BodyDamage.FrontHealth) ||
        !FMath::IsNearlyEqual(Before.RearHealth, BodyDamage.RearHealth) ||
        !FMath::IsNearlyEqual(Before.LeftHealth, BodyDamage.LeftHealth) ||
        !FMath::IsNearlyEqual(Before.RightHealth, BodyDamage.RightHealth) ||
        !FMath::IsNearlyEqual(Before.CoolingStress, BodyDamage.CoolingStress) ||
        BeforeMask != GetDetachedPanelMask();

    GTT_LOG( Display,
        TEXT("NATIVE_ROAD_SCRIPTED_IMPACT vehicle=%s zone=%s speed_kmh=%.1f changed=%s panel_mask=%d surcharge=%d"),
        *NativeVehicleId.ToString(), DamageZoneToString(Zone), ImpactSpeedKmh,
        bChanged ? TEXT("YES") : TEXT("NO"), GetDetachedPanelMask(), GetBodyDamageRepairSurcharge());
    return bChanged;
}
