#include "pch.h"
#include "../Public/McpProfileGroup.h"

void McpProfileGroup::McpDispatchRequestHook(__int64 a1, __int64* a2, int a3)
{
    return McpDispatchRequestOG(a1, a2, 3);
}

void McpProfileGroup::InitMcpProfileGroup()
{
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x144cb00), McpProfileGroup::McpDispatchRequestHook, (LPVOID*)&McpProfileGroup::McpDispatchRequestOG);
}