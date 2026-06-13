#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/NavigationSystem/Public/NavigationSystem.h>

void NavigationSystem::CreateAndConfigureNavigationSystem(UAthenaNavSystemConfig* ModuleConfig, UWorld* World)
{
    ModuleConfig->bPrioritizeNavigationAroundSpawners = true;
    ModuleConfig->bAutoSpawnMissingNavData = true;
    ModuleConfig->bAllowAutoRebuild = true;
    ModuleConfig->bSupportRuntimeNavmeshDisabling = false;
    ModuleConfig->bUsesStreamedInNavLevel = true;

    return CreateAndConfigureNavigationSystemOG(ModuleConfig, World);
}

void NavigationSystem::InitializeForWorld(UAthenaNavSystem* NavSystem, UWorld* World, EFNavigationSystemRunMode Mode)
{
    NavSystem->bAutoCreateNavigationData = true;
    NavSystem->SupportedAgentsMask.bSupportsAgent3 = 1;
    NavSystem->bSpawnNavDataInNavBoundsLevel = true;
    Utils::AthenaNavSystem = NavSystem;

    return InitializeForWorldOG(NavSystem, World, Mode);
}

int64 NavigationSystem::ILoveStripped()
{
    static auto Base = InSDKUtils::GetImageBase();
    auto Ret = int64(_ReturnAddress()) - Base;
    if (Ret == 0x1FB1007)
    {
        return (int64)((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->ServerBotManager;
    }
    return 0;
}

UFortServerBotManagerAthena* GetBotManager(UObject* Object) {
    if (Object) {
        return GetGameMode()->ServerBotManager;
    }
    return nullptr;
}

void NavigationSystem::InitNavigationSystem()
{
    MH_CreateHook((LPVOID)(ImageBase + Addresses::CreateNavegationSystem), CreateAndConfigureNavigationSystem, (LPVOID*)&CreateAndConfigureNavigationSystemOG);
    Utils::HookVTable(UAthenaNavSystem::GetDefaultObj(), 0x54, InitializeForWorld, (LPVOID*)&InitializeForWorldOG);
    /*MH_CreateHook((LPVOID)(ImageBase + 0x2347370), GetBotManager, nullptr);
    ModifyInstruction(ImageBase + 0x1FB1002, ImageBase + 0x2347370);*/
    MH_CreateHook((LPVOID)(ImageBase + 0x9C12A0), ILoveStripped, nullptr);
}