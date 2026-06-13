#include "pch.h"
#include <FortniteGame/Source/FortniteAI/Public/InventoryBaseOnSpawned/Public/InventoryBaseOnSpawned.h>

void InventoryBaseOnSpawned::UFortAthenaAISpawnerDataComponent_InventoryBaseOnSpawned(UFortAthenaAISpawnerDataComponent_InventoryBase* Comp, AFortPawn* Pawn)
{
    UFortAthenaAISpawnerDataComponent_InventoryBaseOnSpawnedOG(Comp, Pawn);

    if (!Pawn || !Comp)
        return;

    auto PC = Utils::Cast<AFortAthenaAIBotController>(Pawn->GetController());

    if (!PC)
        return;

    auto Inv = Utils::SpawnActor<AFortInventory>(FVector{ 0, 0, -99999 }, {});
    if (Inv)
    {
        Inv->Owner = PC;
        Inv->OnRep_Owner();

        PC->Inventory = Inv;
    }

    for (auto& ItemAndCount : ((UFortAthenaAISpawnerDataComponent_AIBotInventory*)Comp)->Items)
    {
        UFortWorldItem* Item = Utils::Cast<UFortWorldItem>(ItemAndCount.Item->CreateTemporaryItemInstanceBP(ItemAndCount.Count, 0));
        if (!Item) continue;

        Item->OwnerInventory = PC->Inventory;
        FFortItemEntry& Entry = Item->ItemEntry;
        Entry.LoadedAmmo = 9999;

        PC->Inventory->Inventory.ReplicatedEntries.Add(Entry);
        PC->Inventory->Inventory.ItemInstances.Add(Item);
        PC->Inventory->Inventory.MarkItemDirty(Entry);
        PC->Inventory->HandleInventoryLocalUpdate();

        if (auto WeaponDef = Utils::Cast<UFortWeaponRangedItemDefinition>(Item->ItemEntry.ItemDefinition))
        {
            PC->PendingEquipWeapon = Item;
            Pawn->EquipWeaponDefinition(WeaponDef, Item->ItemEntry.ItemGuid);
        }
    }
}

void InventoryBaseOnSpawned::InitInventoryBaseOnSpawned()
{
    MH_CreateHook((LPVOID)(ImageBase + 0x1FB2B70), UFortAthenaAISpawnerDataComponent_InventoryBaseOnSpawned, (LPVOID*)&UFortAthenaAISpawnerDataComponent_InventoryBaseOnSpawnedOG);
}