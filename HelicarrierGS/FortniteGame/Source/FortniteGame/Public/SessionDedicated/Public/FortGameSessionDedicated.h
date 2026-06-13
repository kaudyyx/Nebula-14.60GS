
#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class FortGameSessionDedicated
{
public:
	static int ReturnTrue();
	static int ReturnHook();
	static void CollectGravity();
	static int GetNetMode();
	static inline void (*McpDispatchRequestOG)(__int64, __int64*, int);
	static void McpDispatchRequestHook(__int64 a1, __int64* a2, int a3);
	static bool CanCreateInCurrentContext();
	static float GetMaxTickRate();
	static void InitializeHooks();
};
