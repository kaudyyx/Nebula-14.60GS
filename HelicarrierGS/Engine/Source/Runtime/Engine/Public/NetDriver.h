#pragma once
#include "pch.h"

class NetDriver
{
public:
	static bool Listen(UWorld* World, FURL& URL);
	DefHookOg(void, TickFlush, UNetDriver*);
	static void InitNetDriver();
};