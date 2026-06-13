
#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>
#include <FortniteGame/Source/FortniteGame/Public/BuildingContainer/Public/BuildingContainer.h>

FFortRangedWeaponStats* Inventory::GetStats(UFortWeaponItemDefinition* Def)
{
	if (!Def || !Def->WeaponStatHandle.DataTable)
		return nullptr;

	auto Val = Def->WeaponStatHandle.DataTable->Search([Def](FName& Key, uint8_t* Value)
		{ return Def->WeaponStatHandle.RowName == Key && Value; });

	return Val ? *(FFortRangedWeaponStats**)Val : nullptr;
}

UFortWorldItem* Inventory::GiveItem(AFortPlayerController* PlayerController, UFortItemDefinition* Def, int Count, int LoadedAmmo, int Level, bool ShowPickupNoti, bool updateInventory, int PhantomReserveAmmo)
{
	if (!PlayerController || !Def || !Count)
		return nullptr;
	UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, Level);
	if (!Item) return nullptr;
	Item->SetOwningControllerForTemporaryItem(PlayerController);
	Item->ItemEntry.LoadedAmmo = LoadedAmmo;
	Item->ItemEntry.PhantomReserveAmmo = PhantomReserveAmmo;

	if (Def->IsA<UFortAmmoItemDefinition>() || Def->IsA<UFortResourceItemDefinition>())
	{
		FFortItemEntryStateValue Value{};
		Value.IntValue = ShowPickupNoti;
		Value.StateType = EFortItemEntryState::ShouldShowItemToast;
		Item->ItemEntry.StateValues.Add(Value);
	}

	PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
	PlayerController->WorldInventory->Inventory.ItemInstances.Add(Item);

	if (updateInventory)
		TriggerInventoryUpdate(PlayerController, &Item->ItemEntry);
	return Item;
}

UFortWorldItem* Inventory::GiveItem(AFortPlayerController* PC, FFortItemEntry entry, int Count, bool ShowPickupNoti, bool updateInventory)
{
	if (Count == -1)
		Count = entry.Count;
	return GiveItem(PC, entry.ItemDefinition, Count, entry.LoadedAmmo, entry.Level, ShowPickupNoti, updateInventory, entry.PhantomReserveAmmo);
}

void Inventory::TriggerInventoryUpdate(AFortPlayerController* PC, FFortItemEntry* Entry)
{
	AFortInventory* Inventory;
	if (auto Bot = Utils::Cast<AFortAthenaAIBotController>(PC)) {
		Inventory = Bot->Inventory;
	}
	else {
		Inventory = PC->WorldInventory;
	}
	if (!PC || !Inventory) return;
	Inventory->bRequiresLocalUpdate = true;
	Inventory->HandleInventoryLocalUpdate();

	return Entry ? Inventory->Inventory.MarkItemDirty(*Entry) : Inventory->Inventory.MarkArrayDirty();
}

void Inventory::GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
{
	if (!PC || !PC->WorldInventory || !Def) return;
	EEvaluateCurveTableResult Result;
	float OutXY = 0;
	UDataTableFunctionLibrary::EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &OutXY, FString());
	if (!Def->MaxStackSize.Curve.CurveTable || OutXY <= 0)
		OutXY = Def->MaxStackSize.Value;;
	FFortItemEntry* Found = nullptr;
	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
		{
			Found = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			PC->WorldInventory->Inventory.ReplicatedEntries[i].Count += Count;
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].Count > OutXY)
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].Count = OutXY;
			}
			PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
			TriggerInventoryUpdate(PC, nullptr);
			PC->WorldInventory->HandleInventoryLocalUpdate();
			return;
		}
	}

	if (!Found)
	{
		GiveItem((AFortPlayerControllerAthena*)PC, Def, Count, LoadedAmmo);
	}
}

void Inventory::RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count)
{
	if (!PC || !PC->WorldInventory || !Def) return;
	bool Remove = false;
	FGuid guid;
	for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
		if (Entry.ItemDefinition == Def)
		{
			Entry.Count -= Count;
			if (Entry.Count <= 0)
			{
				//if (Def->IsA(UFortGadgetItemDefinition::StaticClass())) reinterpret_cast<bool(*)(UFortGadgetItemDefinition*, IFortInventoryOwnerInterface*, UFortItem*)>(__int64(GetModuleHandleW(0)) + 0x2AF2EA0)((UFortGadgetItemDefinition*)Def, reinterpret_cast<IFortInventoryOwnerInterface * (*)(AFortPlayerController*, UClass*)>(__int64(GetModuleHandleW(0)) + 0x3AD9490)(PC, IFortInventoryOwnerInterface::StaticClass()), PC->WorldInventory->Inventory.ItemInstances[i]);
				PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues.Free();
				PC->WorldInventory->Inventory.ReplicatedEntries.Remove(i);
				Remove = true;
				guid = Entry.ItemGuid;
			}
			else
			{
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				TriggerInventoryUpdate(PC, nullptr);
				PC->WorldInventory->HandleInventoryLocalUpdate();
				return;
			}
			break;
		}
	}

	if (Remove)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ItemInstances[i]->GetItemGuid() == guid)
			{
				PC->WorldInventory->Inventory.ItemInstances.Remove(i);
				break;
			}
		}
	}

	PC->WorldInventory->Inventory.MarkArrayDirty();
	PC->WorldInventory->HandleInventoryLocalUpdate();
}

EFortQuickBars Inventory::GetQuickbar(UFortItemDefinition* ItemDef)
{
	if (!ItemDef) return EFortQuickBars::Primary;

	return (ItemDef->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) ||
		ItemDef->IsA(UFortResourceItemDefinition::StaticClass()) ||
		ItemDef->IsA(UFortAmmoItemDefinition::StaticClass()) ||
		ItemDef->IsA(UFortTrapItemDefinition::StaticClass()) ||
		ItemDef->IsA(UFortBuildingItemDefinition::StaticClass()) ||
		ItemDef->IsA(UFortEditToolItemDefinition::StaticClass()))
		? EFortQuickBars::Secondary
		: EFortQuickBars::Primary;
}

int Inventory::GetLevel(const FDataTableCategoryHandle& CategoryHandle)
{
	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	if (!CategoryHandle.DataTable)
		return 0;

	if (!UKismetStringLibrary::Conv_NameToString(CategoryHandle.ColumnName))
		return 0;

	if (!CategoryHandle.RowContents.ComparisonIndex)
		return 0;

	TArray<FFortLootLevelData*> LootLevelData;

	for (auto& LootLevelDataPair : (TMap<FName, FFortLootLevelData*>)CategoryHandle.DataTable->RowMap)
	{
		if (LootLevelDataPair.Value()->Category != CategoryHandle.RowContents)
			continue;

		LootLevelData.Add(LootLevelDataPair.Value());
	}

	if (LootLevelData.Num() > 0)
	{
		int ind = -1;
		int ll = 0;

		for (int i = 0; i < LootLevelData.Num(); i++)
		{
			if (LootLevelData[i]->LootLevel <= GameState->WorldLevel && LootLevelData[i]->LootLevel > ll)
			{
				ll = LootLevelData[i]->LootLevel;
				ind = i;
			}
		}

		if (ind != -1)
		{
			auto subbed = LootLevelData[ind]->MaxItemLevel - LootLevelData[ind]->MinItemLevel;

			if (subbed <= -1)
				subbed = 0;
			else
			{
				auto calc = (int)(((float)rand() / 32767) * (float)(subbed + 1));
				if (calc <= subbed)
					subbed = calc;
			}

			return subbed + LootLevelData[ind]->MinItemLevel;
		}
	}

	return 0;
}

FFortItemEntry* Inventory::MakeItemEntry(UFortItemDefinition* ItemDefinition, int32 Count, int32 Level) {
	FFortItemEntry* ItemEntry = new FFortItemEntry();

	ItemEntry->MostRecentArrayReplicationKey = -1;
	ItemEntry->ReplicationID = -1;
	ItemEntry->ReplicationKey = -1;

	ItemEntry->ItemDefinition = ItemDefinition;
	ItemEntry->Count = Count;
	ItemEntry->Durability = 1.f;
	ItemEntry->GameplayAbilitySpecHandle = FGameplayAbilitySpecHandle(-1);
	ItemEntry->ParentInventory.ObjectIndex = -1;
	ItemEntry->Level = Level;
	if (auto Weapon = ItemDefinition->Cast<UFortWeaponItemDefinition>())
	{
		auto Stats = GetStats(Weapon);
		ItemEntry->LoadedAmmo = Stats ? Stats->ClipSize : 1;
		if (Weapon->bUsesPhantomReserveAmmo)
			ItemEntry->PhantomReserveAmmo = Stats->InitialClips * Stats->ClipSize;
	}

	return ItemEntry;
}

AFortPickupAthena* Inventory::SpawnPickup(FVector Loc, FFortItemEntry& Entry, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource, AFortPlayerPawn* Pawn, int OverrideCount, bool Toss, bool RandomRotation, bool bCombine)
{
	AFortPickupAthena* NewPickup = Utils::SpawnActor<AFortPickupAthena>(Loc, {});
	if (!NewPickup)
		return nullptr;
	NewPickup->bRandomRotation = RandomRotation;
	NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
	NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
	NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
	NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;
	NewPickup->OnRep_PrimaryPickupItemEntry();
	NewPickup->PawnWhoDroppedPickup = Pawn;


	NewPickup->TossPickup(Loc, Pawn, -1, Toss, true, SourceTypeFlag, SpawnSource);
	NewPickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox;
	if (NewPickup->bTossedFromContainer) NewPickup->OnRep_TossedFromContainer();

	return NewPickup;
}

AFortPickupAthena* Inventory::SpawnPickup(FVector Loc, UFortItemDefinition* ItemDefinition, int Count, int LoadedAmmo, EFortPickupSourceTypeFlag SourceTypeFlag, EFortPickupSpawnSource SpawnSource, AFortPlayerPawn* Pawn, bool Toss, bool bRandomRotation)
{
	return SpawnPickup(Loc, *MakeItemEntry(ItemDefinition, Count, 0), SourceTypeFlag, SpawnSource, Pawn, -1, Toss, true, bRandomRotation);
}

AFortPickupAthena* Inventory::SpawnPickup(ABuildingContainer* Container, FFortItemEntry Entry, AFortPlayerPawn* Pawn, int OverrideCount)
{
	if (!&Entry)
		return nullptr;
	auto ContainerLoc = Container->K2_GetActorLocation();
	auto Loc = ContainerLoc + (Container->GetActorForwardVector() * Container->LootSpawnLocation_Athena.X) + (Container->GetActorRightVector() * Container->LootSpawnLocation_Athena.Y) + (Container->GetActorUpVector() * Container->LootSpawnLocation_Athena.Z);
	AFortPickupAthena* NewPickup = Utils::SpawnActor<AFortPickupAthena>(Loc, {});
	if (!NewPickup)
		return nullptr;
	NewPickup->bRandomRotation = true;
	NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
	NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
	NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
	NewPickup->OnRep_PrimaryPickupItemEntry();
	NewPickup->PawnWhoDroppedPickup = Pawn;
	//NewPickup->TossPickup(Loc, Pawn, -1, true, true, EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::Chest);
	//auto bFloorLoot = Container->IsA<ATiered_Athena_FloorLoot_01_C>() || Container->IsA<ATiered_Athena_FloorLoot_Warmup_C>();
	UFortKismetLibrary::TossPickupFromContainer(UWorld::GetWorld(), Container, NewPickup, 1, 0, Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, false);
	NewPickup->bTossedFromContainer = true;
	NewPickup->OnRep_TossedFromContainer();

	return NewPickup;
}

void Inventory::Remove(AFortPlayerController* PlayerController, FGuid Guid)
{
	if (!PlayerController)
		return;
	auto ItemEntryIdx = PlayerController->WorldInventory->Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& entry) { return entry.ItemGuid == Guid; });
	if (ItemEntryIdx != -1)
		PlayerController->WorldInventory->Inventory.ReplicatedEntries.Remove(ItemEntryIdx);
	auto ItemInstanceIdx = PlayerController->WorldInventory->Inventory.ItemInstances.SearchIndex([&](UFortWorldItem* entry) { return entry->ItemEntry.ItemGuid == Guid; });
	auto ItemInstance = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
		{ return entry->ItemEntry.ItemGuid == Guid; });
	auto Instance = ItemInstance ? *ItemInstance : nullptr;
	if (ItemInstanceIdx != -1) PlayerController->WorldInventory->Inventory.ItemInstances.Remove(ItemInstanceIdx);

	TriggerInventoryUpdate(PlayerController, nullptr);
	PlayerController->WorldInventory->Inventory.MarkArrayDirty();
	PlayerController->WorldInventory->HandleInventoryLocalUpdate();
}

void Inventory::Remove67(AFortPlayerController* PlayerController, FGuid Guid, int Amount)
{
	if (!PlayerController)
		return;

	auto& ReplicatedEntries = PlayerController->WorldInventory->Inventory.ReplicatedEntries;
	auto& ItemInstances = PlayerController->WorldInventory->Inventory.ItemInstances;

	int removed = 0;

	// Borrar de ReplicatedEntries
	for (int i = ReplicatedEntries.Num() - 1; i >= 0 && removed < Amount; i--)
	{
		if (ReplicatedEntries[i].ItemGuid == Guid)
		{
			ReplicatedEntries.Remove(i);
			removed++;
		}
	}

	removed = 0;
	// Borrar de ItemInstances
	for (int i = ItemInstances.Num() - 1; i >= 0 && removed < Amount; i--)
	{
		if (ItemInstances[i]->ItemEntry.ItemGuid == Guid)
		{
			ItemInstances.Remove(i);
			removed++;
		}
	}

	TriggerInventoryUpdate(PlayerController, nullptr);
	PlayerController->WorldInventory->Inventory.MarkArrayDirty();
	PlayerController->WorldInventory->HandleInventoryLocalUpdate();
}

void Inventory::ReplaceEntry(AFortPlayerController* PlayerController, FFortItemEntry& Entry)
{
	if (!PlayerController)
		return;
	auto ent = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* item)
		{ return item->ItemEntry.ItemGuid == Entry.ItemGuid; });
	if (ent)
		(*ent)->ItemEntry = Entry;

	TriggerInventoryUpdate(PlayerController, &Entry);
}

AFortPickupAthena* Inventory::SpawnPickup67(const FSpawnPickupData& SpawnPickupData)
{
	UFortItemDefinition* ItemDefinition = SpawnPickupData.ItemDefinition;
	FVector Location = SpawnPickupData.Location;
	FRotator Rotation = SpawnPickupData.Rotation;
	bool bRandomRotation = SpawnPickupData.bRandomRotation;
	int Count = SpawnPickupData.Count;
	int LoadedAmmo = SpawnPickupData.LoadedAmmo;
	AFortPawn* PickupOwner = SpawnPickupData.PickupOwner;
	EFortPickupSourceTypeFlag FortPickupSourceTypeFlag = SpawnPickupData.FortPickupSourceTypeFlag;
	EFortPickupSpawnSource FortPickupSpawnSource = SpawnPickupData.FortPickupSpawnSource;

	if (!ItemDefinition || Count == 0)
		return nullptr;

	if (AFortPickupAthena* NewPickup = Utils::SpawnActor<AFortPickupAthena>(Location, Rotation))
	{
		NewPickup->PawnWhoDroppedPickup = PickupOwner;
		NewPickup->PrimaryPickupItemEntry.ItemDefinition = ItemDefinition;
		NewPickup->PrimaryPickupItemEntry.Count = Count;

		if (LoadedAmmo == -1)
			LoadedAmmo = Looting::GetClipSize(ItemDefinition);

		NewPickup->PrimaryPickupItemEntry.LoadedAmmo = LoadedAmmo;
		NewPickup->OnRep_PrimaryPickupItemEntry();

		NewPickup->bRandomRotation = bRandomRotation;

		if (!NewPickup->PickupLocationData.CombineTarget)
		{
			NewPickup->TossPickup(Location, PickupOwner, 0, true, false, FortPickupSourceTypeFlag, FortPickupSpawnSource);
		}

		return NewPickup;
	}

	return nullptr;
}

FFortItemEntry* Inventory::GetItemEntry(AFortInventory* WorldInventory, const FGuid& ItemGuid)
{
	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemGuid == ItemGuid)
			return &ReplicatedEntry;
	}

	return NULL;
}

FFortItemEntry* Inventory::GetItemEntry(AFortInventory* WorldInventory, const UFortItemDefinition* ItemDefinition, bool bCheckStackSize)
{
	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemDefinition == ItemDefinition)
		{
			if (bCheckStackSize && ReplicatedEntry.Count >= ItemDefinition->MaxStackSize.GetValueAtLevel67(0))
				continue;

			return &ReplicatedEntry;
		}
	}

	return NULL;
}

FFortItemEntry* Inventory::GetItemEntry(AFortInventory* WorldInventory, const UClass* Class)
{
	FFortItemEntry* ItemEntry = NULL;

	for (FFortItemEntry& ReplicatedEntry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (ReplicatedEntry.ItemDefinition->Class == Class)
			ItemEntry = &ReplicatedEntry;
	}

	return ItemEntry;
}