
#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/LateGame/Public/LateGame.h>

FItemAndCount Lategame::GetShotguns()
{
    static UEAllocatedVector<FItemAndCount> Shotguns
    {
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03")),
    };

    return Shotguns[rand() % Shotguns.size()];
}

FItemAndCount Lategame::GetAssaultRifles()
{
    static UEAllocatedVector<FItemAndCount> AssaultRifles
    {
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_UC_Ore_T03.WID_Assault_Auto_Athena_UC_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_R_Ore_T03.WID_Assault_Auto_Athena_R_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Assault_SemiAuto_Athena_VR_Ore_T03.WID_Assault_SemiAuto_Athena_VR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/HighTower/Items/Tomato/Tomato_Rifle/WID_Assault_Stark_Athena_R_Ore_T03.WID_Assault_Stark_Athena_R_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/HighTower/Items/Tomato/Tomato_Rifle/WID_Assault_Stark_Athena_SR_Ore_T03.WID_Assault_Stark_Athena_SR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/HighTower/Items/Tomato/Tomato_Rifle/WID_Assault_Stark_Athena_VR_Ore_T03.WID_Assault_Stark_Athena_VR_Ore_T03"))
    };

    return AssaultRifles[rand() % AssaultRifles.size()];
}

FItemAndCount Lategame::GetSnipers()
{
    static UEAllocatedVector<FItemAndCount> Snipers
    {
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/HighTower/Items/Grape/BrambleShield/CoreBR/WID_HighTower_Grape_BrambleShield_CoreBR.WID_HighTower_Grape_BrambleShield_CoreBR")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Pistol_Flintlock_Athena_UC.WID_Pistol_Flintlock_Athena_UC")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_VR_Ore_T03.WID_Sniper_Heavy_Athena_VR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_SR_Ore_T03.WID_Sniper_Heavy_Athena_SR_Ore_T03")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/HighTower/Items/Soy/CoreBR/WID_HighTower_Soy_Boarding_CoreBR.WID_HighTower_Soy_Boarding_CoreBR")),
        FItemAndCount(1, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Consumables/WitchBroom/WID_Athena_WitchBroom.WID_Athena_WitchBroom")),

    };

    return Snipers[rand() % Snipers.size()];
}

FItemAndCount Lategame::GetHeals()
{
    static UEAllocatedVector<FItemAndCount> Heals
    {
        FItemAndCount(3, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields")),
        FItemAndCount(6, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall")),
        FItemAndCount(4, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Consumables/Flopper/WID_Athena_Flopper.WID_Athena_Flopper")),
        FItemAndCount(6, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Consumables/ChillBronco/Athena_ChillBronco.Athena_ChillBronco")),
        FItemAndCount(3, {}, Utils::StaticFindObject<UFortWeaponRangedItemDefinition>("/Game/Athena/Items/Consumables/Flopper/Effective/WID_Athena_Flopper_Effective.WID_Athena_Flopper_Effective")),
    };

    return Heals[rand() % Heals.size()];
}

UFortAmmoItemDefinition* Lategame::GetAmmo(EAmmoType AmmoType)
{
    static UEAllocatedVector<UFortAmmoItemDefinition*> Ammos
    {
        Utils::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight"),
        Utils::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells"),
        Utils::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium"),
        Utils::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AmmoDataRockets.AmmoDataRockets"),
        Utils::StaticFindObject<UFortAmmoItemDefinition>("/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy")
    };

    return Ammos[(int)AmmoType];
}

UFortResourceItemDefinition* Lategame::GetResource(EFortResourceType ResourceType)
{
    static UEAllocatedVector<UFortResourceItemDefinition*> Resources
    {
        Utils::StaticFindObject<UFortResourceItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData"),
        Utils::StaticFindObject<UFortResourceItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData"),
        Utils::StaticFindObject<UFortResourceItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData")
    };

    return Resources[(int)ResourceType];
}
