#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class CosmeticLibraryBase
{
public:
	static inline void (*UFortAthenaAISpawnerDataComponent_CosmeticLibraryBaseOnSpawnedOG)(UFortAthenaAISpawnerDataComponent* CosmeticLibrary, AFortPlayerPawnAthena* Pawn);
	static void UFortAthenaAISpawnerDataComponent_CosmeticLibraryBaseOnSpawned(UFortAthenaAISpawnerDataComponent* CosmeticLibrary, AFortPlayerPawnAthena* Pawn);
	static void InitCosmeticLibraryBase();
};