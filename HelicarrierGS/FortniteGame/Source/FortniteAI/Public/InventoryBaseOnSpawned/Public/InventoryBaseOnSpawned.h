#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class InventoryBaseOnSpawned
{
public:
	static inline void (*UFortAthenaAISpawnerDataComponent_InventoryBaseOnSpawnedOG)(UFortAthenaAISpawnerDataComponent_InventoryBase* Comp, AFortPawn* Pawn);
	static void UFortAthenaAISpawnerDataComponent_InventoryBaseOnSpawned(UFortAthenaAISpawnerDataComponent_InventoryBase* Comp, AFortPawn* Pawn);
	static void InitInventoryBaseOnSpawned();
};