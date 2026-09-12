#include "Combat/GTTCombatTypes.h"

FGTTWeaponProfile FGTTWeaponProfile::Make(EGTTWeaponType InType)
{
    FGTTWeaponProfile P;
    P.Type = InType;
    switch (InType)
    {
        case EGTTWeaponType::Pitchfork: P.DisplayName=NSLOCTEXT("GTT","Pitchfork","Pitchfork"); P.Damage=34; P.Range=205; P.Cooldown=.72f; P.Knockback=300; P.PoliceHeat=13; break;
        case EGTTWeaponType::Axe: P.DisplayName=NSLOCTEXT("GTT","Axe","Wood Axe"); P.Damage=42; P.Range=165; P.Cooldown=.82f; P.Knockback=260; P.PoliceHeat=17; break;
        case EGTTWeaponType::Branch: P.DisplayName=NSLOCTEXT("GTT","Branch","Heavy Branch"); P.Damage=19; P.Range=175; P.Cooldown=.48f; P.Knockback=240; P.PoliceHeat=8; break;
        case EGTTWeaponType::Rake: P.DisplayName=NSLOCTEXT("GTT","Rake","Rake"); P.Damage=26; P.Range=210; P.Cooldown=.66f; P.Knockback=280; P.PoliceHeat=11; break;
        case EGTTWeaponType::CowChain: P.DisplayName=NSLOCTEXT("GTT","CowChain","Cattle Chain"); P.Damage=29; P.Range=230; P.Cooldown=.62f; P.Knockback=320; P.PoliceHeat=12; break;
        case EGTTWeaponType::Shovel: P.DisplayName=NSLOCTEXT("GTT","Shovel","Shovel"); P.Damage=31; P.Range=190; P.Cooldown=.68f; P.Knockback=300; P.PoliceHeat=12; break;
        case EGTTWeaponType::WorkshopWrench: P.DisplayName=NSLOCTEXT("GTT","WorkshopWrench","Workshop Wrench"); P.Damage=23; P.Range=145; P.Cooldown=.42f; P.Knockback=190; P.PoliceHeat=9; break;
        case EGTTWeaponType::FarmShotgun: P.DisplayName=NSLOCTEXT("GTT","FarmShotgun","Old Farm Shotgun"); P.Damage=58; P.Range=2600; P.Cooldown=1.0f; P.Knockback=520; P.PoliceHeat=32; P.bRanged=true; break;
        default: P.DisplayName=NSLOCTEXT("GTT","BareHands","Bare Hands"); P.Damage=11; P.Range=135; P.Cooldown=.4f; P.Knockback=150; P.PoliceHeat=4; break;
    }
    return P;
}
