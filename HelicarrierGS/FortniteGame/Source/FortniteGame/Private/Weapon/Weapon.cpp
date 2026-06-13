#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/Weapon/Weapon.h>


void FortWeapon::ReloadWeapon(AFortWeapon* Weapon, int AmmoToRemove)
{
    __try { ReloadWeapon_Impl(Weapon, AmmoToRemove); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FortWeapon::ReloadWeapon_Impl(AFortWeapon* Weapon, int AmmoToRemove)
{
	if (!Weapon || !Weapon->WeaponData)
		return;
	auto WeaponData = Weapon->WeaponData->GetAmmoWorldItemDefinition_BP();
	AFortPlayerPawnAthena* Pawn = reinterpret_cast<AFortPlayerPawnAthena*>(Weapon->Owner);
	if (!Pawn)
		return;
	AFortPlayerControllerAthena* PC = reinterpret_cast<AFortPlayerControllerAthena*>(Pawn->Controller);
	if (!PC)
		return;
	if (PC->IsA(AFortAthenaAIBotController::StaticClass()))
		return;

	AFortInventory* Inventory;
	if (auto Bot = PC->Cast<AFortAthenaAIBotController>())
	{
		Inventory = Bot->Inventory;
	}
	else
	{
		Inventory = PC->WorldInventory;
	}
	if (!PC || !Inventory || !Weapon)
		return;
	if (Weapon->WeaponData->bUsesPhantomReserveAmmo)
	{
		Weapon->PhantomReserveAmmo -= AmmoToRemove;
		Weapon->OnRep_PhantomReserveAmmo();
		return;
	}
	auto Ammo = Weapon->WeaponData->GetAmmoWorldItemDefinition_BP();
	auto ent = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return Weapon->WeaponData == Ammo ? entry.ItemGuid == Weapon->ItemEntryGuid : entry.ItemDefinition == Ammo; });
	auto WeaponEnt = Inventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == Weapon->ItemEntryGuid; });
	if (!WeaponEnt)
		return;

	if (ent)
	{
		ent->Count -= AmmoToRemove;
		if (ent->Count <= 0)
			Inventory::Remove(PC, ent->ItemGuid);
		else
			Inventory::ReplaceEntry(PC, *ent);
	}
	WeaponEnt->LoadedAmmo += AmmoToRemove;
	Inventory::ReplaceEntry(PC, *WeaponEnt);
}

bool bHasWorldContextObject = false;
void FortWeapon::TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret)
{
    __try { TeleportPlayerPawn_Impl(Context, Stack, Ret); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FortWeapon::TeleportPlayerPawn_Impl(UObject* Context, FFrame& Stack, bool* Ret)
{
	UObject* WorldContextObject;
	AFortPlayerPawnAthena* PlayerPawn;
	FVector DestLocation;
	FRotator DestRotation;
	bool bIgnoreCollision;
	bool bIgnoreSupplementalKillVolumeSweep;

	if (bHasWorldContextObject)
		Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&PlayerPawn);
	Stack.StepCompiledIn(&DestLocation);
	Stack.StepCompiledIn(&DestRotation);
	Stack.StepCompiledIn(&bIgnoreCollision);
	Stack.StepCompiledIn(&bIgnoreSupplementalKillVolumeSweep);
	Stack.IncrementCode();

	PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
	*Ret = true;
}

void FortWeapon::SetLoadedAmmo(UFortWorldItem* WorldItem, int32 InCount)
{
    __try { SetLoadedAmmo_Impl(WorldItem, InCount); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FortWeapon::SetLoadedAmmo_Impl(UFortWorldItem* WorldItem, int32 InCount)
{
	AFortInventory* WorldInventory = WorldItem->OwnerInventory;

	if (WorldInventory == NULL)
		return;

	if (InCount != WorldItem->ItemEntry.LoadedAmmo)
	{
		WorldItem->ItemEntry.LoadedAmmo = InCount;

		WorldItem->OwnerInventory->Inventory.MarkItemDirty(WorldItem->ItemEntry);

		FFortItemEntry* ItemEntry = Inventory::GetItemEntry(WorldInventory, WorldItem->ItemEntry.ItemGuid);

		if (ItemEntry == NULL)
			return;

		ItemEntry->LoadedAmmo = InCount;

		WorldItem->OwnerInventory->Inventory.MarkItemDirty(*ItemEntry);
		WorldItem->OwnerInventory->HandleInventoryLocalUpdate();
	}
}

void FortWeapon::SetPhantomReserveAmmo(UFortWorldItem* WorldItem, int32 InCount)
{
    __try { SetPhantomReserveAmmo_Impl(WorldItem, InCount); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FortWeapon::SetPhantomReserveAmmo_Impl(UFortWorldItem* WorldItem, int32 InCount)
{
	printf("SetPhantomReserveAmmo is getting called");
	AFortInventory* WorldInventory = WorldItem->OwnerInventory;

	if (WorldInventory == NULL)
		return;

	if (InCount != WorldItem->ItemEntry.PhantomReserveAmmo)
	{
		WorldItem->ItemEntry.PhantomReserveAmmo = InCount;

		WorldItem->OwnerInventory->Inventory.MarkItemDirty(WorldItem->ItemEntry);

		FFortItemEntry* ItemEntry = Inventory::GetItemEntry(WorldInventory, WorldItem->ItemEntry.ItemGuid);

		if (ItemEntry == NULL)
			return;

		ItemEntry->PhantomReserveAmmo = InCount;

		WorldItem->OwnerInventory->Inventory.MarkItemDirty(*ItemEntry);
		WorldItem->OwnerInventory->HandleInventoryLocalUpdate();
	}
}



void FortWeapon::InitFortWeapon()
{
	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2EC0C40), ReloadWeapon, nullptr);
	Utils::ExecHook("/Script/FortniteGame.FortMissionLibrary:TeleportPlayerPawn", TeleportPlayerPawn);
	Utils::HookVTable(UFortWorldItem::GetDefaultObj(), 0xA7, SetPhantomReserveAmmo, nullptr);
	Utils::HookVTable(UFortWorldItem::GetDefaultObj(), 0xA6, SetLoadedAmmo, nullptr);
}