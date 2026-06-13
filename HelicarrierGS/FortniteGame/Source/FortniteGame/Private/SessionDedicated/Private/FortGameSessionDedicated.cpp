
#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/SessionDedicated/Public/FortGameSessionDedicated.h>

int FortGameSessionDedicated::ReturnTrue()
{
    return true;
}

int FortGameSessionDedicated::ReturnHook()
{
    return 0;
}


void FortGameSessionDedicated::CollectGravity()
{
    return;
}

void FortGameSessionDedicated::McpDispatchRequestHook(__int64 a1, __int64* a2, int a3)
{
    return McpDispatchRequestOG(a1, a2, 3);
}

bool FortGameSessionDedicated::CanCreateInCurrentContext() {
    //  printf("CanCreateInCurrentContext Called!");
    return true;
}

float FortGameSessionDedicated::GetMaxTickRate() {
    return 60.f;
}

void NullSHit()
{

}

void FortGameSessionDedicated::InitializeHooks()
{
    MH_CreateHook((LPVOID)(ImageBase + 0x4CDE3C0), ReturnTrue, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + 0x9F83B0), ReturnTrue, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + 0x370F060), ReturnTrue, nullptr);

    MH_CreateHook((LPVOID)(ImageBase + Addresses::GetNetMode), ReturnTrue, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + Addresses::GetMaxTickRate), GetMaxTickRate, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + 0x2653520), ReturnTrue, nullptr);//AActorNetModeCrash fix
    MH_CreateHook((LPVOID)(ImageBase + 0x4a3faa0), ReturnTrue, nullptr);//actor get net mode
    MH_CreateHook((LPVOID)(ImageBase + 0x4cd9866), ReturnTrue, nullptr);//reboot getnetmode maybe fix crash
    MH_CreateHook((LPVOID)(ImageBase + 0x36e1aed), ReturnTrue, nullptr);//CollageGrabe

    Utils::HookVTable(AActor::GetDefaultObj(), 0x20, CanCreateInCurrentContext, nullptr);// CanCreateInCurrentContext
    Utils::HookVTable(AAthenaAIDirector::GetDefaultObj(), 0x20, CanCreateInCurrentContext, nullptr); // CanCreateInCurrentContext
    Utils::HookVTable(AAthenaNavMesh::GetDefaultObj(), 0x20, CanCreateInCurrentContext, nullptr); // CanCreateInCurrentContext

    Patch<uint8_t>(ImageBase + Addresses::EncryptionPatch, 0x74);
    Patch<uint8_t>(ImageBase + Addresses::GameSessionPatch, 0x85);
}