#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/FortTeams/Public/Teams.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>
#include <FortniteGame/Source/FortniteGame/Public/BuildingContainer/Public/BuildingContainer.h>
inline AFortPlayerControllerAthena* GetPCFromId(FUniqueNetIdRepl& ID)
{
	for (auto& PlayerState : UWorld::GetWorld()->GameState->PlayerArray)
	{
		auto PlayerStateAthena = Utils::Cast<AFortPlayerStateAthena>(PlayerState);
		if (!PlayerStateAthena)
			continue;
		if (PlayerStateAthena->AreUniqueIDsIdentical(ID, PlayerState->UniqueId))
			return Utils::Cast<AFortPlayerControllerAthena>(PlayerState->Owner);
	}

	return nullptr;
}
static void (*AddToAlivePlayers)(UObject*, UObject*) = decltype(AddToAlivePlayers)(InSDKUtils::GetImageBase() + 0x20401B0);//"FortGameModeAthena: Player [%s] added to alive players list (Team [%d]).  Player count is now [%d].  Team count is now [%d]. "
static void (*OnResurrectionCompleted)(UObject*, int) = decltype(OnResurrectionCompleted)(InSDKUtils::GetImageBase() + 0x32560D0);//48 89 5C 24 ? 57 48 83 EC ? 48 8B 15 ? ? ? ? 48 8B F9 48 8B 19 E8 ? ? ? ? 48 8B D0 45 33 C0 48 8B CF 48 8B 83 ? ? ? ? 48 8B 5C 24 ? 48 83 C4 ? 5F 48 FF E0 CC 48 8D 44 24
void Teams::RebootingDelegate(ABuildingGameplayActorSpawnMachine* RebootVan)
{
	if (!RebootVan)
		return;

	if (!RebootVan->ResurrectLocation)
		return;

	if (RebootVan->PlayerIdsForResurrection.Num() <= 0)
		return;

	auto PC = GetPCFromId(RebootVan->PlayerIdsForResurrection[0]);

	if (!PC)
		return;

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

	if (!PlayerState)
		return;

	auto ResurrectionComponent = PC->ResurrectionComponent;

	if (!ResurrectionComponent)
		return;

	TWeakObjectPtr<AFortPlayerStart> WeakPlayerStart{};
	WeakPlayerStart.ObjectIndex = RebootVan->ResurrectLocation->Index;
	WeakPlayerStart.ObjectSerialNumber = UObject::GObjects->GetSerialByIdx(WeakPlayerStart.ObjectIndex);

	ResurrectionComponent->ResurrectionLocation = WeakPlayerStart;
	PC->RespawnPlayerAfterDeath(false);
	PC->ClientClearDeathNotification();
	UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);

	AFortPlayerPawnAthena* Pawn = Utils::Cast<AFortPlayerPawnAthena>(PC->Pawn);

	if (!Pawn)
	{
		printf("Failed to spawn pawn!\n");
		OnResurrectionCompleted(RebootVan, RebootVan->SquadId);
		return;
	}

	PC->ClientClearDeathNotification();

	Pawn->SetHealth(100);
	Pawn->SetMaxHealth(100);

	Pawn->SetShield(0);
	Pawn->SetMaxShield(100);

	Pawn->SetOwner(PC);

	PlayerState->ResurrectionChipAvailable.bResurrectionChipAvailable = false;

	PlayerState->SpectatingTarget = nullptr;
	PlayerState->OnRep_SpectatingTarget();

	PlayerState->Spectators.SpectatorArray.Free();
	PlayerState->Spectators.MarkArrayDirty();

	static UFunction* OnPlayerPawnResurrected =
		RebootVan->Class->GetFunction(RebootVan->Class->GetName(), "OnPlayerPawnResurrected");

	if (OnPlayerPawnResurrected)
		RebootVan->ProcessEvent(OnPlayerPawnResurrected, &Pawn);

	auto Instigator = RebootVan->InstigatorPC.Get();

	if (Instigator && Instigator->PlayerState)
	{
		auto InstigatorPS = (AFortPlayerStateAthena*)Instigator->PlayerState;

		InstigatorPS->RebootCounter++;
		InstigatorPS->OnRep_RebootCounter();

		for (size_t i = 0; i < InstigatorPS->Spectators.SpectatorArray.Num(); i++)
		{
			auto& Spectator = InstigatorPS->Spectators.SpectatorArray[i];

			if (Spectator.PlayerState == PlayerState)
			{
				InstigatorPS->Spectators.SpectatorArray.Remove(i);
				break;
			}
		}

		InstigatorPS->Spectators.MarkArrayDirty();
	}

	static void (*FinishResurrection)(ABuildingGameplayActorSpawnMachine * SpawnMachine, int SquadId) = decltype(FinishResurrection)(InSDKUtils::GetImageBase() + 0x2f531d0);

	if (FinishResurrection)
	{
		AddToAlivePlayers(UWorld::GetWorld()->AuthorityGameMode, PC);

		RebootVan->PlayerIdsForResurrection.Remove(0);

		FinishResurrection(RebootVan, RebootVan->SquadId);

		if (RebootVan->PlayerIdsForResurrection.Num() <= 0)
		{
			OnResurrectionCompleted(RebootVan, RebootVan->SquadId);
		}
	}
}

void Teams::ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AFortPlayerControllerAthena* EventInstigator, AFortAthenaAIBotController* BotController)
{
	if (!Pawn || !EventInstigator || !BotController)
		return;

	AFortPlayerControllerAthena* DeadPC = (AFortPlayerControllerAthena*)Pawn->Controller;
	AFortPlayerStateAthena* DeadPlayerState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
	if (!DeadPC || !DeadPC->PlayerState)
		return;
	static auto Class = Utils::StaticLoadObject<UClass>("/Game/Abilities/NPC/Generic/GAB_AthenaDBNORevive.GAB_AthenaDBNORevive_C");
	static auto Class2 = Utils::StaticLoadObject<UClass>("/Game/Abilities/NPC/Generic/GAB_AthenaDBNO.GAB_AthenaDBNO_C");
	static auto Prop = Utils::StaticLoadObject<UProperty>("/Game/Abilities/NPC/Generic/GAB_AthenaDBNORevive.GAB_AthenaDBNORevive_C.PlayerPawn");
	for (size_t i = 0; i < DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items.Num(); i++)
	{
		if (DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Ability->IsA(Class2))
		{
			DeadPlayerState->AbilitySystemComponent->ClientCancelAbility(DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Handle, DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo);
			DeadPlayerState->AbilitySystemComponent->ClientEndAbility(DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Handle, DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo);
			DeadPlayerState->AbilitySystemComponent->ServerEndAbility(DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].Handle, DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo, DeadPlayerState->AbilitySystemComponent->ActivatableAbilities.Items[i].ActivationInfo.PredictionKeyWhenActivated);
		}
	}
	EventInstigator->bMarkedAlive = true;
	Pawn->bIsDBNO = false;
	Pawn->OnRep_IsDBNO();
	Pawn->bPlayedDying = false;
	Pawn->bIsDying = false;
	Pawn->SetHealth(30);// fixed the HP!
	if (DeadPC && DeadPC->IsA(AFortPlayerControllerAthena::StaticClass()) && DeadPC->IsLocalController())
	{
		DeadPC->RespawnPlayerAfterDeath(false);
	}
	DeadPlayerState->DeathInfo = FDeathInfo{};
	DeadPlayerState->OnRep_DeathInfo();
	if (DeadPC && DeadPC->IsA(AFortPlayerControllerAthena::StaticClass()) && DeadPC->IsLocalController())
	{
		DeadPC->ClientOnPawnRevived(EventInstigator);
	}
	//else
	//{
	//	DeadPC->ClientOnPawnRevived(BotController); // attempt at bots revive
	//}
}
//0x26949D0
static void AddActorsToIndicatedList_Impl(AFortPlayerControllerAthena* InstigatingController, TArray<AActor*>& IndicatedActors, FIndicatedActorData& IndicatedActorData, bool bAddAsUnique, bool bAllowOwningPlayer); // forward decl

inline void AddActorsToIndicatedList(AFortPlayerControllerAthena* InstigatingController, TArray<AActor*>& IndicatedActors, FIndicatedActorData& IndicatedActorData, bool bAddAsUnique, bool bAllowOwningPlayer)
{
	__try { AddActorsToIndicatedList_Impl(InstigatingController, IndicatedActors, IndicatedActorData, bAddAsUnique, bAllowOwningPlayer); }
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

inline void AddActorsToIndicatedList_Impl(AFortPlayerControllerAthena* InstigatingController, TArray<AActor*>& IndicatedActors, FIndicatedActorData& IndicatedActorData, bool bAddAsUnique, bool bAllowOwningPlayer)
{
	std::cout << __FUNCTION__ << std::endl;

	AFortPlayerPawn* Pawn = InstigatingController->MyFortPawn;

	if (Pawn == NULL)
		return;

	for (AActor* IndicatedActor : IndicatedActors)
	{
		if (Pawn == IndicatedActor && !bAllowOwningPlayer)
			continue;

		float TimeSeconds = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

		FIndicatedActorInfoEntry ActorInfoEntry{};
		ActorInfoEntry.Actor = IndicatedActor;
		ActorInfoEntry.StartTime = TimeSeconds;
		ActorInfoEntry.EndTime = TimeSeconds + IndicatedActorData.Duration;
		ActorInfoEntry.Data = IndicatedActorData;

		InstigatingController->IndicatedActorManagementComponent->IndicatedActorList.Entries.Add(ActorInfoEntry);
		InstigatingController->IndicatedActorManagementComponent->IndicatedActorList.MarkArrayDirty();

		AFortPlayerControllerAthena* ActorPlayerController = Utils::Cast<AFortPlayerControllerAthena>(UFortKismetLibrary::GetFortPlayerControllerFromActor(IndicatedActor));
	}

	IndicatedActors.Clear();
}

inline void AddActorsToStenciledList(AFortPlayerControllerAthena* InstigatingController, TArray<AActor*>& StenciledActors, FStenciledActorData& StenciledActorData, bool bAddAsUnique)
{
	std::cout << __FUNCTION__ << std::endl;

	for (AActor* StenciledActor : StenciledActors)
	{
		FStenciledActorInfoEntry ActorInfoEntry{};
		ActorInfoEntry.Actor = StenciledActor;
		ActorInfoEntry.Data = StenciledActorData;

		InstigatingController->IndicatedActorManagementComponent->StenciledActorList.Entries.Add(ActorInfoEntry);
		InstigatingController->IndicatedActorManagementComponent->StenciledActorList.MarkArrayDirty();
	}
}
inline void AddActorsInRadiusToIndicatedList(AFortPlayerControllerAthena* InstigatingController, TArray<FIndicatedActorDataWithFilter>& IndicatedActorFilterDatas, const bool bAddAsUnique, const bool bReplaceExistingEntry)
{
	std::cout << __FUNCTION__ << std::endl;

	AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)InstigatingController->Pawn;

	for (auto& Data : IndicatedActorFilterDatas)
	{
		TArray<AActor*> Temp_Actors;
		UGameplayStatics::GetAllActorsOfClassWithTag(UWorld::GetWorld(), Data.ActorClassFilter, Data.IndicatedActorTags.GameplayTags[0].TagName, &Temp_Actors);

		for (auto& Temp_Actor : Temp_Actors)
		{
			if (UKismetMathLibrary::Vector_DistanceSquared(Data.IndicatedData.IndicatorOffset, Temp_Actor->K2_GetActorLocation()) > Data.OverlapRadius)
				continue;

			FIndicatedActorInfoEntry ActorInfoEntry{};
			ActorInfoEntry.Actor = Temp_Actor;
			ActorInfoEntry.Data = Data.IndicatedData;

			std::cout << "Hello i am awesome" << std::endl;

			InstigatingController->IndicatedActorManagementComponent->IndicatedActorList.Entries.Add(ActorInfoEntry);
			InstigatingController->IndicatedActorManagementComponent->IndicatedActorList.MarkArrayDirty();
		}

		Temp_Actors.Clear();
	}
}


void Teams::InitTeams()
{
	uintptr_t HookAddr = __int64(APlayerController::StaticClass()->GetFunction("PlayerController", "SetVirtualJoystickVisibility")->ExecFunction);
	ModifyInstructionLEA(ImageBase + 0x232b062, HookAddr, 3);
	Hook(HookAddr, RebootingDelegate);
	Utils::SwapVFTs(APlayerPawn_Athena_C::StaticClass()->DefaultObject, 0x1E4, Teams::ServerReviveFromDBNO, nullptr);
	//	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x26949D0), AddActorsToStenciledList, nullptr); 
		//MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x26949D0), AddActorsToIndicatedList, nullptr);//Correct
}