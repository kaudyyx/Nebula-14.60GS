#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class FortSpawner
{
public:
	static void SpawnAI(UClass* Class, const std::string& Name);
	static void RequestAISpawn();
};