#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>



class FortWeapon
{
public:
	static void ReloadWeapon(AFortWeapon*, int);
	static void TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret);
	static void SetLoadedAmmo(UFortWorldItem* WorldItem, int32 InCount);
	static void SetPhantomReserveAmmo(UFortWorldItem* WorldItem, int32 InCount);
	static void InitFortWeapon();

	// _Impl helpers
	static void ReloadWeapon_Impl(AFortWeapon* Weapon, int AmmoToRemove);
	static void TeleportPlayerPawn_Impl(UObject* Context, FFrame& Stack, bool* Ret);
	static void SetLoadedAmmo_Impl(UFortWorldItem* WorldItem, int32 InCount);
	static void SetPhantomReserveAmmo_Impl(UFortWorldItem* WorldItem, int32 InCount);
};