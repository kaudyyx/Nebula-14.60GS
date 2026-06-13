#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>


class NavigationSystem
{
public:
	static inline void (*InitializeForWorldOG)(UNavigationSystemV1* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode);
	static void InitializeForWorld(UAthenaNavSystem* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode);
	static inline void (*CreateAndConfigureNavigationSystemOG)(UAthenaNavSystemConfig* ModuleConfig, UWorld* World);
	static void CreateAndConfigureNavigationSystem(UAthenaNavSystemConfig* ModuleConfig, UWorld* World);
	static inline int64 ILoveStripped();
	static void InitNavigationSystem();
};