#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>


class BuildingSMActor
{
public:
	DefHookOg(void, ServerCreateBuildingActor, AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData);
	DefHookOg(void, ServerBeginEditingBuildingActor, AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit);
	DefHookOg(void, ServerEditBuildingActor, AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored);
	static void ServerEndEditingBuildingActor(UObject*, FFrame&);
	static void ServerCreateBuildingAndSpawnDeco(UObject*, FFrame&);
	DefUHookOg(ServerSpawnDeco);
	static void AttemptSpawnResources(ABuildingSMActor* BuildingActor, AFortPlayerPawn* InstigatorPawn, float ActualDamageDealt, bool bJustHitWeakspot);
	static void InitBuildingSMActor();

	// _Impl helpers
	static void ServerCreateBuildingActor_Impl(AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData);
	static void ServerBeginEditingBuildingActor_Impl(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* Building);
	static void ServerEditBuildingActor_Impl(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored);
};