

#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class Inventory {
public:
	static FFortItemEntry* GetItemEntry(AFortInventory* WorldInventory, const FGuid& ItemGuid);
	static FFortItemEntry* GetItemEntry(AFortInventory* WorldInventory, const UFortItemDefinition* ItemDefinition, bool bCheckStackSize = false);
	static FFortItemEntry* GetItemEntry(AFortInventory* WorldInventory, const UClass* Class);
	static void TriggerInventoryUpdate(AFortPlayerController* PC, FFortItemEntry* Entry);
	static void Remove67(AFortPlayerController* PlayerController, FGuid Guid, int Amount);

	static void GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo);

	static void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count);

	static EFortQuickBars GetQuickbar(UFortItemDefinition* ItemDef);

	static AFortPickupAthena* SpawnPickup(FVector, FFortItemEntry&, EFortPickupSourceTypeFlag = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* = nullptr, int = -1, bool = true, bool = true, bool = true);
	static AFortPickupAthena* SpawnPickup(FVector, UFortItemDefinition*, int, int, EFortPickupSourceTypeFlag = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* = nullptr, bool = true, bool = true);
	static AFortPickupAthena* SpawnPickup(ABuildingContainer*, FFortItemEntry, AFortPlayerPawn* = nullptr, int = -1);
	static FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition*);

	static FFortItemEntry* MakeItemEntry(UFortItemDefinition*, int32, int32);

	static int GetLevel(const FDataTableCategoryHandle&);
	static void Remove(AFortPlayerController*, FGuid);
	static UFortWorldItem* GiveItem(AFortPlayerController*, UFortItemDefinition*, int = 1, int = 0, int = 0, bool = true, bool = true, int = 0);
	static UFortWorldItem* GiveItem(AFortPlayerController*, FFortItemEntry, int = -1, bool = true, bool = true);

	static void ReplaceEntry(AFortPlayerController*, FFortItemEntry&);
	static AFortPickupAthena* SpawnPickup67(const FSpawnPickupData& SpawnPickupData);
};