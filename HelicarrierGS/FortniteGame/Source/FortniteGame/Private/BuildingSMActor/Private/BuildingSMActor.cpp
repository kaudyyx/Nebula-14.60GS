#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/BuildingSMActor/Public/BuildingSMActor.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>
#include <FortniteGame/Source/FortniteGame/Public/QuestManager/Public/QuestManage.h>

static bool (*CantBuild)(UObject*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(ImageBase + 0x26983d0);
void (*ServerCreateBuildingActorOG)(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData);
void BuildingSMActor::ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData)
{
    __try { ServerCreateBuildingActor_Impl(PC, CreateBuildingData); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void BuildingSMActor::ServerCreateBuildingActor_Impl(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData)
{
	if (!PC || PC->IsInAircraft())
		return;

	UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();
	char a7;
	TArray<AActor*> BuildingsToRemove;
	if (!CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingsToRemove, &a7))
	{
		auto ResDef = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);

		FTransform Transform{};
		Transform.Translation = CreateBuildingData.BuildLoc;
		Transform.Rotation = CreateBuildingData.BuildRot;
		Transform.Scale3D = { 1, 1, 1 };

		ABuildingSMActor* NewBuilding = Utils::SpawnActorLlama<ABuildingSMActor>(BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC);

		if (!NewBuilding)
		{
			BuildingsToRemove.Free();
			return;
		}


		//PC->PayBuildableClassPlacementCost(CreateBuildingData.BuildingClassData);// first time it works but then it crash nice 1 fortnite//"Failed to remove item %s during pay building costs, item duplicated!"
		
		Inventory::RemoveItem(PC, ResDef, 10);

		for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
		{
			BuildingsToRemove[i]->K2_DestroyActor();
		}
		UKismetArrayLibrary::Array_Clear((TArray<int32>&)BuildingsToRemove);
		Utils::FinishSpawnActor<ABuildingSMActor>(NewBuilding, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot);

		NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true);
		NewBuilding->bPlayerPlaced = true;
		NewBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
		NewBuilding->Team = EFortTeam(NewBuilding->TeamIndex);

		NewBuilding->SetNetDormancy(ENetDormancy::DORM_DormantAll);

		//BuildingsToRemove.FreeArray();
	}
}

void BuildingSMActor::ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* Building)
{
    __try { ServerBeginEditingBuildingActor_Impl(PlayerController, Building); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void BuildingSMActor::ServerBeginEditingBuildingActor_Impl(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* Building)
{
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex)
		return;

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
	if (!PlayerState)
		return;

	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([](FFortItemEntry& entry)
		{ return entry.ItemDefinition->IsA<UFortEditToolItemDefinition>(); });
	if (!itemEntry)
		return;

	PlayerController->ServerExecuteInventoryItem(itemEntry->ItemGuid);
}

bool CanBePlacedByPlayer(UClass* BuildClass) {
	return ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->AllPlayerBuildableClasses.Search([BuildClass](UClass* Class) { return Class == BuildClass; });
}

void BuildingSMActor::ServerEditBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
{
    __try { ServerEditBuildingActor_Impl(PlayerController, BuildingActorToEdit, NewBuildingClass, RotationIterations, bMirrored); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void BuildingSMActor::ServerEditBuildingActor_Impl(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
{
	if (!PlayerController || !BuildingActorToEdit || !NewBuildingClass || !BuildingActorToEdit->IsA<ABuildingSMActor>() || !CanBePlacedByPlayer(NewBuildingClass) || BuildingActorToEdit->TeamIndex != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex)
		return;

	BuildingActorToEdit->SetNetDormancy(ENetDormancy::DORM_DormantAll);
	BuildingActorToEdit->EditingPlayer = nullptr;

	auto NewBuild = BuildingActorToEdit->ReplaceBuildingActor(NewBuildingClass, BuildingActorToEdit->CurrentBuildingLevel, RotationIterations, bMirrored, PlayerController);


	if (NewBuild)
		NewBuild->bPlayerPlaced = true;
}

void BuildingSMActor::ServerEndEditingBuildingActor(UObject* Context, FFrame& Stack)
{
	ABuildingSMActor* Building;
	Stack.StepCompiledIn(&Building);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerController*)Context;
	if (!PlayerController || !PlayerController->MyFortPawn || !Building || Building->EditingPlayer != (AFortPlayerStateZone*)PlayerController->PlayerState || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PlayerController->PlayerState)->TeamIndex)
		return;

	Building->SetNetDormancy(ENetDormancy::DORM_DormantAll);
	Building->EditingPlayer = nullptr;

	AFortWeap_EditingTool* EditTool = Utils::Cast<AFortWeap_EditingTool>(PlayerController->MyFortPawn->CurrentWeapon);

	if (!EditTool)
		return;

	EditTool->EditActor = nullptr;
	EditTool->OnRep_EditActor();

	if (auto ControllerAthena = Utils::Cast<AFortPlayerControllerAthena>(PlayerController)) ControllerAthena->BuildingsEdited++;
}

static void ServerRepairBuildingActor_Impl(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair); // forward decl

void ServerRepairBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair)
{
    __try { ServerRepairBuildingActor_Impl(PlayerController, BuildingActorToRepair); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void ServerRepairBuildingActor_Impl(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair)
{
	if (!BuildingActorToRepair)
		return;

	int32 RepairCosts = PlayerController->PayRepairCosts(BuildingActorToRepair);// works but dont remove it from inventory?
	BuildingActorToRepair->RepairBuilding(PlayerController, RepairCosts);
}

void BuildingSMActor::ServerCreateBuildingAndSpawnDeco(UObject* Context, FFrame& Stack) {
	auto Tool = (AFortDecoTool*)Context;
	auto Pawn = (APawn*)Tool->Owner;
	if (!Pawn) return;
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	auto ItemDefinition = (UFortDecoItemDefinition*)Tool->ItemDefinition;
	if (!PlayerController) return;
	FVector_NetQuantize10 BuildingLocation;
	FRotator BuildingRotation;
	FVector_NetQuantize10 Location;
	FRotator Rotation;
	EBuildingAttachmentType InBuildingAttachmentType;
	Stack.StepCompiledIn(&BuildingLocation);
	Stack.StepCompiledIn(&BuildingRotation);
	Stack.StepCompiledIn(&Location);
	Stack.StepCompiledIn(&Rotation);
	Stack.StepCompiledIn(&InBuildingAttachmentType);
	Stack.IncrementCode();

	auto Mat = PlayerController->BroadcastRemoteClientInfo->RemoteBuildingMaterial;
	std::string MatName;
	std::string MatNameShort;
	switch ((int)Mat) {
	case 0:
		MatName = "Wood";
		MatNameShort = "W";
		break;
	case 1:
		MatName = "Stone";
		MatNameShort = "S";
		break;
	case 2:
		MatName = "Metal";
		MatNameShort = "M";
		break;
	}
	std::string BuildingType;
	switch ((int)InBuildingAttachmentType) {
	case 0:
	case 6:
	case 7:
	case 2:
		BuildingType = "Floor";
		break;
	case 1:
		BuildingType = "Solid";
		break;
	}

	auto Build = Utils::StaticFindObject<UClass>("/Game/Building/ActorBlueprints/Player/" + MatName + "/L1/PBWA_" + MatNameShort + "1_" + BuildingType + ".PBWA_" + MatNameShort + "1_" + BuildingType + "_C");
	ABuildingSMActor* Building = nullptr;
	TArray<ABuildingSMActor*> RemoveBuildings;
	char _Unknown;
	static auto CantBuild = (__int64 (*)(UWorld*, UObject*, FVector, FRotator, bool, TArray<ABuildingSMActor*> *, char*))(InSDKUtils::GetImageBase() + 0x26983d0);
	if (CantBuild(UWorld::GetWorld(), Build, BuildingLocation, BuildingRotation, false, &RemoveBuildings, &_Unknown)) return;
	auto Resource = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)Build->DefaultObject)->ResourceType);
	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition == Resource;
		});
	if (!itemEntry) return;

	itemEntry->Count -= 10;
	if (itemEntry->Count <= 0) Inventory::Remove(PlayerController, itemEntry->ItemGuid);
	Inventory::ReplaceEntry((AFortPlayerControllerAthena*)PlayerController, *itemEntry);

	for (auto& RemoveBuilding : RemoveBuildings) RemoveBuilding->K2_DestroyActor();
	RemoveBuildings.Free();

	Building = Utils::SpawnActorLlama<ABuildingSMActor>(Build, BuildingLocation, BuildingRotation, PlayerController);
	Building->bPlayerPlaced = true;
	Building->InitializeKismetSpawnedBuildingActor(Building, PlayerController, true);
	Building->TeamIndex = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->TeamIndex;
	Building->Team = EFortTeam(Building->TeamIndex);
	Tool->ServerSpawnDeco(Location, Rotation, Building, InBuildingAttachmentType);
}
// i mean it works it remove it but also remove it from the wall lmfao wtf
void BuildingSMActor::ServerSpawnDeco(UObject* Context, FFrame& Stack) {
	FVector Location;
	FRotator Rotation;
	ABuildingSMActor* AttachedActor;
	EBuildingAttachmentType InBuildingAttachmentType;
	Stack.StepCompiledIn(&Location);
	Stack.StepCompiledIn(&Rotation);
	Stack.StepCompiledIn(&AttachedActor);
	Stack.StepCompiledIn(&InBuildingAttachmentType);
	Stack.IncrementCode();

	auto DecoTool = (AFortDecoTool*)Context;
	auto ContextTrap = DecoTool->Cast<AFortDecoTool_ContextTrap>();
	auto ItemDefinition = (UFortDecoItemDefinition*)DecoTool->ItemDefinition;

	auto Pawn = (APawn*)DecoTool->Owner;
	if (!Pawn)
		return;
	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return;

	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemDefinition == DecoTool->ItemDefinition;
		});
	if (!itemEntry)
		return;

	if (ItemDefinition->bConsumeWhenPlaced)//Proper way bc calling bConsumeWhenPlaced good job andreu
	{
		Inventory::Remove67(PlayerController, itemEntry->ItemGuid, 1);
	}
}

void BuildingSMActor::AttemptSpawnResources(ABuildingSMActor* BuildingActor, AFortPlayerPawn* InstigatorPawn, float ActualDamageDealt, bool bJustHitWeakspot)
{
	if (!InstigatorPawn || !InstigatorPawn->Controller) return;

	AFortPlayerController* PlayerController = Utils::Cast<AFortPlayerController>(InstigatorPawn->Controller);

	if (PlayerController == NULL)
		return;

	AFortInventory* WorldInventory = PlayerController->WorldInventory;

	if (WorldInventory == NULL)
		return;

	UFortResourceItemDefinition* ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActor->ResourceType);

	if (ResourceItemDefinition == NULL)
		return;

	float Average;
	UFortKismetLibrary::EvaluateCurveTableRow(BuildingActor->BuildingResourceAmountOverride, 0.f, &Average, FString());

	int32 ResourceAmount = UKismetMathLibrary::Round(Average / (BuildingActor->GetMaxHealth() / ActualDamageDealt)); // this is definetly scuffed

	if (ResourceAmount <= 0)
		return;

	bool bDestroyed = (BuildingActor->GetHealth() - ActualDamageDealt) <= 0;

	PlayerController->ClientReportDamagedResourceBuilding(BuildingActor, BuildingActor->ResourceType, ResourceAmount, bDestroyed, bJustHitWeakspot);

	Inventory::GiveItemStack(PlayerController, ResourceItemDefinition, ResourceAmount, 0);
}



void BuildingSMActor::InitBuildingSMActor()
{
	//MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2f3f580), OnDamageServerHook, (PVOID*)&OnDamageServerOG);
	Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x233, ServerCreateBuildingActor, nullptr);
	Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x23A, ServerBeginEditingBuildingActor, nullptr);
	Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x235, ServerEditBuildingActor, nullptr);
	Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x22F, ServerRepairBuildingActor, nullptr);//AFortPlayerController

	Utils::ExecHook(("/Script/FortniteGame.FortPlayerController.ServerEndEditingBuildingActor"), ServerEndEditingBuildingActor);
	Utils::ExecHook("/Script/FortniteGame.FortDecoTool.ServerCreateBuildingAndSpawnDeco", ServerCreateBuildingAndSpawnDeco);
	Utils::ExecHook("/Script/FortniteGame.FortDecoTool_ContextTrap.ServerCreateBuildingAndSpawnDeco", ServerCreateBuildingAndSpawnDeco);

	for (int32 i = 0; i < UObject::GObjects->Num(); ++i)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (Object == NULL)
			continue;

		if (Object->IsA(ABuildingSMActor::StaticClass()))
		{
			VirtualSwap(Object->VTable, 0x165, AttemptSpawnResources);//0x1CB
		}
	}

}