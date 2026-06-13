#pragma once
#include "pch.h"


class McpProfileGroup
{
public:
	static inline void (*McpDispatchRequestOG)(__int64, __int64*, int);
	static void McpDispatchRequestHook(__int64 a1, __int64* a2, int a3);

	static void InitMcpProfileGroup();
};