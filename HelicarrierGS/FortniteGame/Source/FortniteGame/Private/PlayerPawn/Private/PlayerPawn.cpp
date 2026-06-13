#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/PlayerPawn/Public/PlayerPawn.h>
#include <FortniteGame/Source/FortnitePhoebe/Public/FortnitePhoebe.h>
#include <SDK/Source/Runtime/DevelopmentKit/SDK/B_Athena_Wumba_classes.hpp>

void FortPlayerPawn::ServerHandlePickupInfo(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo& Params)
{
    __try { ServerHandlePickupInfo_Impl(Pawn, Pickup, Params); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FortPlayerPawn::ServerHandlePickupInfo_Impl(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo& Params)
{
	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

	// CRASH FIX: PC pode ser null (bot, controller n�o inicializado) ou WorldInventory pode
	// n�o existir ainda. Acessar PC->WorldInventory sem check causa FaultAddr=0x18.
	if (PC && PC->WorldInventory &&
		(Params.bTrySwapWithWeapon || Params.bUseRequestedSwap) &&
		Pawn->CurrentWeapon &&
		Inventory::GetQuickbar(Pawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary &&
		Inventory::GetQuickbar(Pickup->PrimaryPickupItemEntry.ItemDefinition) == EFortQuickBars::Primary)
	{
		// PC e WorldInventory j� validados acima � n�o redeclarar PC aqui.
		auto SwapEntry = PC->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemGuid == Params.SwapWithItem; });
		PC->SwappingItemDefinition = (UFortWorldItemDefinition*)SwapEntry; // proper af
	}
	Pawn->IncomingPickups.Add(Pickup);

	Pickup->PickupLocationData.bPlayPickupSound = Params.bPlayPickupSound;
	Pickup->PickupLocationData.FlyTime = 0.4f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	//Pickup->PickupLocationData.StartDirection = Params.Direction.QuantizeNormal();
	Pickup->OnRep_PickupLocationData();

	if (PC->IsA(AFortAthenaAIBotController::StaticClass()))
	{
		for (auto& bot : SpawnedBots)
		{
			if (((void*)bot->PC) == ((void*)PC))
			{
				bot->GiveItem(Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
				if (((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP() && ((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP() != Pickup->PrimaryPickupItemEntry.ItemDefinition)
				{
					bot->GiveItem(((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP(), 30);
				}
				break;
			}
		}
	}

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();
}


void FortPlayerPawn::InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry& PickupEntry) {
	auto MaxStack = (int32)Utils::EvaluateScalableFloat(PickupEntry.ItemDefinition->MaxStackSize);
	int ItemCount = 0;
	for (auto& Item : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Inventory::GetQuickbar(Item.ItemDefinition) == EFortQuickBars::Primary)
			ItemCount += ((UFortWorldItemDefinition*)Item.ItemDefinition)->NumberOfSlotsToTake;
	}
	auto GiveOrSwap = [&]() {
		if (ItemCount == 5 && Inventory::GetQuickbar(PickupEntry.ItemDefinition) == EFortQuickBars::Primary) {
			// CRASH FIX: CurrentWeapon pode ser null durante transi��es de estado
			if (PlayerController->MyFortPawn->CurrentWeapon &&
				Inventory::GetQuickbar(PlayerController->MyFortPawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
				auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PlayerController](FFortItemEntry& entry)
					{ return entry.ItemGuid == PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid; });
				// CRASH FIX: GetViewTarget() pode retornar null quando o player está morrendo,
				// desconectando ou em transição de estado. Null deref causa crash 0xC0000005.
				AActor* _safeViewTarget = PlayerController->GetViewTarget();
				FVector _dropLoc = _safeViewTarget ? _safeViewTarget->K2_GetActorLocation()
				                  : (PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector());
				Inventory::SpawnPickup(_dropLoc, *itemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
				Inventory::Remove(PlayerController, PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid);
				Inventory::GiveItem(PlayerController, PickupEntry, PickupEntry.Count, true);
			}
			else {
				// CRASH FIX: GetViewTarget() pode retornar null quando o player está morrendo,
				// desconectando ou em transição de estado. Null deref causa crash 0xC0000005.
				AActor* _safeViewTarget = PlayerController->GetViewTarget();
				FVector _dropLoc = _safeViewTarget ? _safeViewTarget->K2_GetActorLocation()
				                  : (PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector());
				Inventory::SpawnPickup(_dropLoc, (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
			}
		}
		else
			Inventory::GiveItem(PlayerController, PickupEntry, PickupEntry.Count, true);
		};
	auto GiveOrSwapStack = [&](int32 OriginalCount) {
		if (PickupEntry.ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
			Inventory::GiveItem(PlayerController, PickupEntry, OriginalCount - MaxStack, true);
		else {
			// CRASH FIX: GetViewTarget() pode retornar null quando o player está morrendo,
			// desconectando ou em transição de estado. Null deref causa crash 0xC0000005.
			AActor* _safeViewTarget = PlayerController->GetViewTarget();
			FVector _dropLoc = _safeViewTarget ? _safeViewTarget->K2_GetActorLocation()
			                  : (PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector());
			Inventory::SpawnPickup(_dropLoc, (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn, OriginalCount - MaxStack);
		}
		};
	if (PickupEntry.ItemDefinition->IsStackable()) {
		auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
			{ return entry.ItemDefinition == PickupEntry.ItemDefinition && entry.Count < MaxStack; });
		if (itemEntry) {
			auto State = itemEntry->StateValues.Search([](FFortItemEntryStateValue& Value)
				{ return Value.StateType == EFortItemEntryState::ShouldShowItemToast; });
			if (!State) {
				FFortItemEntryStateValue Value{};
				Value.StateType = EFortItemEntryState::ShouldShowItemToast;
				Value.IntValue = true;
				itemEntry->StateValues.Add(Value);
			}
			else State->IntValue = true;

			if ((itemEntry->Count += PickupEntry.Count) > MaxStack) {
				auto OriginalCount = itemEntry->Count;
				itemEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}
			Inventory::ReplaceEntry(PlayerController, *itemEntry);
		}
		else {
			if (PickupEntry.Count > MaxStack) {
				auto OriginalCount = PickupEntry.Count;
				PickupEntry.Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}
			GiveOrSwap();
		}
	}
	else {
		GiveOrSwap();
	}
}

char FortPlayerPawn::CompletePickupAnimation(AFortPickup* Pickup)
{
    char _r{};
    __try { _r = CompletePickupAnimation_Impl(Pickup); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

char FortPlayerPawn::CompletePickupAnimation_Impl(AFortPickup* Pickup)
{
	auto Pawn = (AFortPlayerPawnAthena*)Pickup->PickupLocationData.PickupTarget;
	if (!Pawn)
		return CompletePickupAnimationOG(Pickup);
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	// CRASH FIX: null check em PlayerController e WorldInventory antes de qualquer acesso
	if (!PlayerController || !Pawn->Controller || PlayerController->IsA(AFortAthenaAIBotController::StaticClass()))
		return CompletePickupAnimationOG(Pickup);

	if (auto entry = (FFortItemEntry*)PlayerController->SwappingItemDefinition)
	{
		// CRASH FIX: GetViewTarget() pode retornar null quando o player está morrendo,
		// desconectando ou em transição de estado. Null deref causa crash 0xC0000005.
		AActor* _safeViewTarget = PlayerController->GetViewTarget();
		FVector _dropLoc = _safeViewTarget ? _safeViewTarget->K2_GetActorLocation()
		                  : (PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector());
		Inventory::SpawnPickup(_dropLoc, *entry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
		// SwapEntry(PC, *entry, Pickup->PrimaryPickupItemEntry);
		Inventory::Remove(PlayerController, entry->ItemGuid);
		Inventory::GiveItem(PlayerController, Pickup->PrimaryPickupItemEntry);
		PlayerController->SwappingItemDefinition = nullptr;
	}
	else
	{
		InternalPickup(PlayerController, Pickup->PrimaryPickupItemEntry);
	}
	return CompletePickupAnimationOG(Pickup);
}

void FortPlayerPawn::OnCapsuleBeginOverlap(AFortPlayerPawn* PlayerPawn, FFrame& Stack, void* Ret)
{
    __try { OnCapsuleBeginOverlap_Impl(PlayerPawn, Stack, Ret); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void FortPlayerPawn::OnCapsuleBeginOverlap_Impl(AFortPlayerPawn* PlayerPawn, FFrame& Stack, void* Ret)
{
	UPrimitiveComponent* OverlappedComp;
	AActor* OtherActor;
	UPrimitiveComponent* OtherComp;
	int32 OtherBodyIndex;
	bool bFromSweep;
	FHitResult SweepResult;

	Stack.StepCompiledIn(&OverlappedComp);
	Stack.StepCompiledIn(&OtherActor);
	Stack.StepCompiledIn(&OtherComp);
	Stack.StepCompiledIn(&OtherBodyIndex);
	Stack.StepCompiledIn(&bFromSweep);
	Stack.StepCompiledIn(&SweepResult);

	// CRASH FIX: verificar PlayerPawn PRIMEIRO antes de qualquer desrefer�ncia.
	// O c�digo original checava !PlayerPawn depois de usar PlayerPawn->bIsDBNO � UB garantido.
	if (!PlayerPawn || PlayerPawn->IsDead())
	{
		return;
	}

	Stack.Code += Stack.Code != nullptr;

	AFortPlayerController* PlayerController = Utils::Cast<AFortPlayerController>(PlayerPawn->Controller);
	if (!PlayerController) return;

	AFortPickup* Pickup = Utils::Cast<AFortPickup>(OtherActor);
	if (!Pickup) return;

	if (!PlayerPawn->bIsDBNO ||
		!PlayerPawn->bIsSkydiving)
	{
		if (Pickup->bPickedUp || !Pickup->bWeaponsCanBeAutoPickups)
			return;

		if (!Pickup->bServerStoppedSimulation && (Pickup->PawnWhoDroppedPickup == PlayerPawn))
			return;

		if (Pickup->PawnWhoDroppedPickup == PlayerPawn)
		{
			return;
		}

		UFortWorldItemDefinition* WorldItemDefinition = Utils::Cast<UFortWorldItemDefinition>(Pickup->PrimaryPickupItemEntry.ItemDefinition);
		if (!WorldItemDefinition) return;

		if (WorldItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || WorldItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
		{
			int32 ItemQuantity = UFortKismetLibrary::K2_GetItemQuantityOnPlayer(PlayerController, WorldItemDefinition);

			for (int32 i = 0; i < PlayerPawn->QueuedAutoPickups.Num(); i++)
			{
				AFortPickup* QueuedAutoPickup = PlayerPawn->QueuedAutoPickups[i];
				if (!QueuedAutoPickup) continue;

				UFortWorldItemDefinition* QueuedWorldItemDefinition = Utils::Cast<UFortWorldItemDefinition>(QueuedAutoPickup->PrimaryPickupItemEntry.ItemDefinition);
				if (!QueuedWorldItemDefinition) continue;

				if (QueuedWorldItemDefinition != WorldItemDefinition)
					continue;

				ItemQuantity += QueuedAutoPickup->PrimaryPickupItemEntry.Count;
			}

			int32 MaxStackSize = Utils::EvaluateScalableFloat(Pickup->PrimaryPickupItemEntry.ItemDefinition->MaxStackSize);

			if (ItemQuantity >= MaxStackSize && !WorldItemDefinition->bAllowMultipleStacks)
				return;

			PlayerPawn->QueuedAutoPickups.Add(Pickup);

			float InFlyTime = 0.4;
			PlayerPawn->ServerHandlePickup(Pickup, InFlyTime, FVector(), true);
		}
	}
}

static void (*SetIsDoorOpen)(AActor* Actor, uint8_t SetSate, AFortPlayerPawnAthena* Pawn) = decltype(SetIsDoorOpen)(__int64(GetModuleHandleW(0)) + 0x237AB60);//40 55 56 57 41 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 80 B9
void FortPlayerPawn::OpenActor(UObject* Context, FFrame& Stack)
{
	AActor* Actor = nullptr;
	AFortPlayerControllerAthena* PC = nullptr;
	bool FastOpening = false;

	Stack.StepCompiledIn(&Actor);
	Stack.StepCompiledIn(&PC);
	Stack.StepCompiledIn(&FastOpening);
	Stack.IncrementCode();

	if (!Actor && ABGA_Athena_Keycard_Lock_Parent_C::StaticClass())
		return;

	if (Actor->IsA<ABuildingWall>())
	{
		SetIsDoorOpen(Actor, FastOpening ? 1 : 0, PC ? Utils::Cast<AFortPlayerPawnAthena>(PC->MyFortPawn) : nullptr);
	}
	/*else
	{
		return OpenActorOG(Context, Stack);
	}*/
}
static inline void (*GiveItemToInventoryOwnerOG)(UObject* Object, FFrame& Stack);
static inline void GiveItemToInventoryOwner(UObject* Object, FFrame& Stack) {
	TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner;
	UFortWorldItemDefinition* ItemDefinition;
	int32 NumberToGive;
	bool bNotifyPlayer;
	int32 ItemLevel;
	int32 PickupInstigatorHandle;
	Stack.StepCompiledIn(&InventoryOwner);
	Stack.StepCompiledIn(&ItemDefinition);
	Stack.StepCompiledIn(&NumberToGive);
	Stack.StepCompiledIn(&bNotifyPlayer);
	Stack.StepCompiledIn(&ItemLevel);
	Stack.StepCompiledIn(&PickupInstigatorHandle);

	auto PC = (AFortPlayerControllerAthena*)InventoryOwner.ObjectPointer;

	Inventory::GiveItem(PC, ItemDefinition, 1, 1, false);

	return GiveItemToInventoryOwnerOG(Object, Stack);
}

static inline int32(*K2_RemoveItemFromPlayerOG)(UObject* Context, FFrame& Stack, int32* Ret);
inline int32 K2_RemoveItemFromPlayer(UObject* Context, FFrame& Stack, int32* Ret)
{

	FFrame OriginalStack = Stack; //crash fix idfk

	AFortPlayerControllerAthena* PC = nullptr;
	UFortWorldItemDefinition* ItemDefinition = nullptr;
	FGuid ItemVariantGuid;
	int32 AmountToRemove = 0;
	bool bForceRemoval = false;

	Stack.StepCompiledIn(&PC);
	Stack.StepCompiledIn(&ItemDefinition);
	Stack.StepCompiledIn(&ItemVariantGuid);
	Stack.StepCompiledIn(&AmountToRemove);
	Stack.StepCompiledIn(&bForceRemoval);
	Stack.IncrementCode();

	if (!PC)
		return K2_RemoveItemFromPlayerOG(Context, OriginalStack, Ret);

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	AFortPlayerPawn* Pawn = (AFortPlayerPawn*)PC->Pawn;

	AFortInventory* WorldInventory = PC->WorldInventory;
	if (!WorldInventory)
		return K2_RemoveItemFromPlayerOG(Context, OriginalStack, Ret);

	auto& Entries = WorldInventory->Inventory.ReplicatedEntries;

	FFortItemEntry* ItemEntry = nullptr;

	for (auto& Entry : Entries)
	{
		if (Entry.ItemDefinition == ItemDefinition)
		{
			ItemEntry = &Entry;
			break;
		}
	}

	if (!ItemEntry)
	{
		return K2_RemoveItemFromPlayerOG(Context, OriginalStack, Ret);
	}

	auto ItemInstance = FindItemInstance(WorldInventory, ItemDefinition);

	if (!ItemInstance)
	{
		*Ret = 0;
		return 0;
	}

	if (ItemEntry->ItemDefinition && ItemEntry->ItemDefinition->GetName().contains("AGID_Disguise"))
	{
		Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
		Pawn->OnRep_BaseCosmeticLoadout();
		UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
		PlayerState->OnRep_CharacterData();
	}

	Inventory::Remove(PC, ItemEntry->ItemGuid);

	*Ret = 1;
	return 1;
}

static inline int32(*K2_RemoveItemFromPlayerByGuidOG)(UObject* Context, FFrame& Stack, int32* Ret);
inline int32 K2_RemoveItemFromPlayerByGuid(UObject* Context, FFrame& Stack, int32* Ret)
{

	FFrame OriginalStack = Stack; //crash fix idfk

	class AFortPlayerControllerAthena* PC;
	struct FGuid ItemGuid;
	int32 AmountToRemove;
	bool bForceRemoval;
	Stack.StepCompiledIn(&PC);
	Stack.StepCompiledIn(&ItemGuid);
	Stack.StepCompiledIn(&AmountToRemove);
	Stack.StepCompiledIn(&bForceRemoval);
	Stack.IncrementCode();

	if (!PC)
		return K2_RemoveItemFromPlayerByGuidOG(Context, OriginalStack, Ret);

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
	AFortPlayerPawn* Pawn = (AFortPlayerPawn*)PC->Pawn;

	AFortInventory* WorldInventory = PC->WorldInventory;
	if (!WorldInventory)
		return K2_RemoveItemFromPlayerByGuidOG(Context, OriginalStack, Ret);


	Inventory::Remove(PC, ItemGuid);

	for (auto& Entry : WorldInventory->Inventory.ReplicatedEntries)
	{
		if (Entry.ItemDefinition && Entry.ItemDefinition->GetName().contains("AGID_Disguise"))
		{

			Pawn->CosmeticLoadout = PC->CosmeticLoadoutPC;
			Pawn->OnRep_BaseCosmeticLoadout();
			UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
			PlayerState->OnRep_CharacterData();
			break;
		}
	}

	return K2_RemoveItemFromPlayerByGuidOG(Context, OriginalStack, Ret);
}

static void ServerSendZiplineState_Impl(AFortPlayerPawnAthena* Pawn, FZiplinePawnState State); // forward decl

static void ServerSendZiplineState(AFortPlayerPawnAthena* Pawn, FZiplinePawnState State)
{
    __try { ServerSendZiplineState_Impl(Pawn, State); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void ServerSendZiplineState_Impl(AFortPlayerPawnAthena* Pawn, FZiplinePawnState State)
{
	Pawn->ZiplineState = State;
	((void (*)(AFortPlayerPawn*))(ImageBase + 0x2b1b650))(Pawn);

	if (State.bJumped)
	{
		auto Velocity = Pawn->CharacterMovement->Velocity;
		auto VelocityX = Velocity.X * -0.5f;
		auto VelocityY = Velocity.Y * -0.5f;
		Pawn->LaunchCharacterJump({ VelocityX >= -750 ? fminf(VelocityX, 750) : -750, VelocityY >= -750 ? fminf(VelocityY, 750) : -750, 1200 }, false, false, true, true);
	}
}
//SetPickupTarget -> 0x2814940

//void FortPlayerPawn::ServerHandlePickupInfo(AFortPlayerPawn* Pawn, AFortPickup* PickUp, const FFortPickupRequestInfo& Params_0)
//{
//	PickUp->SetPickupTarget(Pawn, Params_0.FlyTime / Pawn->PickupSpeedMultiplier, Params_0.Direction);
//}


//void FortPlayerPawn::GivePickupTo(AFortPickup* Pickup, IFortInventoryOwnerInterface* InventoryOwner, bool DestroyAfterPickup)
//{
//	printf("GivePickup is getting called");
//	Original::GivePickupTo(Pickup, InventoryOwner, DestroyAfterPickup);
//
//	AFortPlayerPawnAthena* Pawn = Utils::Cast<AFortPlayerPawnAthena>(Pickup->PickupLocationData.PickupTarget);
//
//	if (Pawn == NULL)
//		return;
//
//	if (AFortPlayerController* PlayerController = Utils::Cast<AFortPlayerController>(Pawn->Controller))
//	{
//		AFortInventory* Inventory = PlayerController->WorldInventory;
//
//		if (Inventory == NULL)
//			return;
//
//		Pickup->PrimaryPickupItemEntry.StateValues;
//
//		Inventory::GiveItem(PlayerController, Pickup->PrimaryPickupItemEntry, Pickup->PrimaryPickupItemEntry.Count, true);
//	}
//	else if (AFortAthenaAIBotController* BotController = Utils::Cast<AFortAthenaAIBotController>(Pawn->Controller))
//	{
//		//what the hell bro fucking proper
//
//		std::cout << "Hello fan im a player ai " << std::endl;
//
//		AFortInventory* tory = BotController->Inventory;
//	}
//}

//void FortPlayerPawn::SpawnItemVariantPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
//{
//	UObject* WorldContextObject;
//	FSpawnItemVariantParams Params;
//
//	Stack.StepCompiledIn(&WorldContextObject);
//	Stack.StepCompiledIn(&Params);
//	Stack.IncrementCode();
//
//
//	*Ret = Inventory::SpawnPickup(FSpawnItemVariantParams::HasPosition() ? Params.position : Params.position, Params.WorldItemDefinition, Params.NumberToSpawn, 0,
//		Params.SourceType, Params.Source, nullptr, Params.bToss, Params.bRandomRotation);
//}

//void UFortKismetLibrary::K2_SpawnPickupInWorldWithClassAndItemEntry(UObject* Context, FFrame& Stack, AFortPickupAthena** Ret)
//{
//	UObject* WorldContextObject;
//	auto Entry = (FFortItemEntry*)malloc(FFortItemEntry::Size());
//	memset(Entry, 0, FFortItemEntry::Size());
//	TSubclassOf<AFortPickupAthena> PickupClass;
//	FVector Position;
//	FVector Direction;
//	int32 OverrideMaxStackCount;
//	bool bToss;
//	bool bRandomRotation;
//	bool bBlockedFromAutoPickup;
//	uint8_t SourceType;
//	uint8_t Source;
//	class AFortPlayerControllerAthena* OptionalOwnerPC;
//	bool bPickupOnlyRelevantToOwner;
//
//	Stack.StepCompiledIn(&WorldContextObject);
//	Stack.StepCompiledIn(Entry);
//	Stack.StepCompiledIn(&PickupClass);
//	Stack.StepCompiledIn(&Position);
//	Stack.StepCompiledIn(&Direction);
//	Stack.StepCompiledIn(&OverrideMaxStackCount);
//	Stack.StepCompiledIn(&bToss);
//	Stack.StepCompiledIn(&bRandomRotation);
//	Stack.StepCompiledIn(&bBlockedFromAutoPickup);
//	Stack.StepCompiledIn(&SourceType);
//	Stack.StepCompiledIn(&Source);
//	Stack.StepCompiledIn(&OptionalOwnerPC);
//	Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
//	Stack.IncrementCode();
//
//	*Ret = Inventory::SpawnPickup(
//		Position, Entry->ItemDefinition, Entry->Count, Entry->Level, SourceType, Source, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, bToss, bRandomRotation);
//	free(Entry);
//}

void FortPlayerPawn::InitFortPlayerPawn()
{
	Utils::SwapVFTs(AFortPlayerPawnAthena::StaticClass()->DefaultObject, 0x209, ServerSendZiplineState, nullptr);
	//Utils::SwapVFTs(AFortPickupAthena::StaticClass()->DefaultObject, 0x658 / 8, GivePickupTo, (LPVOID*)&Original::GivePickupTo);

	Utils::HookVTable(AFortPlayerPawnAthena::GetDefaultObj(), 0x1FC, ServerHandlePickupInfo, (PVOID*)&ServerHandlePickupInfoOG);
	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2805e90), CompletePickupAnimation, (LPVOID*)&CompletePickupAnimationOG);
	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x32d3e20), OnCapsuleBeginOverlap, (LPVOID*)&OnCapsuleBeginOverlapOG);
	Utils::ExecHook(("/Script/FortniteGame.FortKismetLibrary.GiveItemToInventoryOwner"), GiveItemToInventoryOwner, GiveItemToInventoryOwnerOG);
	Utils::ExecHook(("/Script/FortniteGame.FortKismetLibrary.K2_RemoveItemFromPlayerByGuid"), K2_RemoveItemFromPlayerByGuid, K2_RemoveItemFromPlayerByGuidOG);
	Utils::ExecHook(("/Script/FortniteGame.FortKismetLibrary.K2_RemoveItemFromPlayer"), K2_RemoveItemFromPlayer, K2_RemoveItemFromPlayerOG);
	Utils::ExecHook(("/Script/FortniteGame.FortKismetLibrary.OpenActor"), OpenActor, OpenActorOG);

}