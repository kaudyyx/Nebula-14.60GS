#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>


class UFortAthenaAISpawnerData_BehaviorOnSpawned
{
public:
	static inline void (*UFortAthenaAISpawnerDataComponent_BehaviorOnSpawnedOG)(UFortAthenaAISpawnerDataComponent_Behavior* Behavior, AFortPawn* Pawn);
	static void UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned(UFortAthenaAISpawnerDataComponent_Behavior* Behavior, AFortPawn* Pawn);

	static void InitUFortAthenaAISpawnerDataComponent_BehaviorOnSpawned();
};