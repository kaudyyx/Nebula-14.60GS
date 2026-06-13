// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/Athena/Public/FortGameModeAthena.h>
#include <Engine/Source/Runtime/Engine/Public/NetDriver.h>
#include <Engine/Restricted/NotForLicensees/Plugins/OnlineGameplayFramework/Source/McpProfileSys/Public/McpProfileGroup.h>
#include <Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/Abilities.h>
#include <FortniteGame/Source/FortniteGame/Public/SessionDedicated/Public/FortGameSessionDedicated.h>
#include <FortniteGame/Source/FortniteGame/Public/PlayerController/Public/PlayerController.h>
#include <FortniteGame/Source/FortniteGame/Public/QuestManager/Public/QuestManage.h>
#include <FortniteGame/Source/FortniteGame/Public/BuildingSMActor/Public/BuildingSMActor.h>
#include <FortniteGame/Source/FortniteGame/Public/BuildingContainer/Public/BuildingContainer.h>
#include <FortniteGame/Source/FortniteGame/Public/PlayerPawn/Public/PlayerPawn.h>
#include <FortniteGame/Source/FortniteGame/Public/Weapon/Weapon.h>
#include <FortniteGame/Source/FortniteAI/Public/CosmeticLibraryBase/CosmeticLibraryBase.h>
#include <FortniteGame/Source/FortniteGame/Public/NavigationSystem/Public/NavigationSystem.h>
#include <FortniteGame/Source/FortniteAI/Public/InventoryBaseOnSpawned/Public/InventoryBaseOnSpawned.h>
#include <FortniteGame/Source/FortniteAI/Public/OnPossessedPawnDied/OnPossessedPawnDied.h>
#include <FortniteGame/Source/FortniteAI/Public/GetTrackingModifierInternal/Public/GetTrackingModifierInternal.h>
#include <FortniteGame/Source/FortniteGame/Public/Creative/Public/Creative.h>
#include <FortniteGame/Source/FortniteGame/Public/FortTeams/Public/Teams.h>
#include <FortniteGame/Source/FortniteGame/Public/FortTeams/Markers/Public/AthenaMarkerComponent.h>
#include <FortniteGame/Source/FortniteAI/Public/UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned/Public/UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

// ============================================================
// Vectored Exception Handler � SO LOGGING
//
// NAO faz ret simulation (causava o server fechar sem callstack).
// Apenas loga no arquivo C:\gs_veh.log para diagnostico.
// A estabilidade real vem dos __try wrappers nas funcoes de hook.
// ============================================================
// ============================================================
// PATCH: dllmain.cpp - GameServerVEH melhorado
// PROBLEMA: O VEH s� capturava crashes dentro de gs.dll.
// Quando o VEH retornava null (simulating ret), o engine tentava
// usar o null retornado e crashava lendo null+0x30, mas esse
// segundo crash era em FortniteClient.exe e o VEH ignorava.
// FIX: Agora o VEH tamb�m intercepta null-deref (FaultAddr < 0x10000)
// em qualquer m�dulo, DESDE QUE gs.dll esteja na call stack
// (scan de return addresses no stack). Isso cobre o crash secund�rio
// que matava o servidor.
// ============================================================

// SUBSTITUIR a fun��o GameServerVEH e InitVEH no dllmain.cpp por esta vers�o:

static ULONG_PTR g_GsDllBase = 0;
static ULONG_PTR g_GsDllSize = 0;
static volatile LONG g_VehCount = 0;

// Limite de recupera��es por RIP �nico para evitar loop infinito
struct RipRecovery { ULONG_PTR Rip; LONG Count; };
static RipRecovery g_RipTable[64] = {};
static volatile LONG g_RipTableLock = 0;

static bool GsDllInStack(ULONG_PTR Rsp)
{
    if (g_GsDllBase == 0 || g_GsDllSize == 0)
        return false;
    __try
    {
        // Escaneia at� 48 slots do stack procurando return addresses da gs.dll
        const ULONG_PTR* p = reinterpret_cast<const ULONG_PTR*>(Rsp);
        for (int i = 0; i < 48; i++)
        {
            ULONG_PTR addr = p[i];
            if (addr >= g_GsDllBase && addr < g_GsDllBase + g_GsDllSize)
                return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}

static bool AllowRipRecovery(ULONG_PTR Rip)
{
    // Permite at� 32 recupera��es por RIP �nico para n�o mascarar loops infinitos
    while (InterlockedCompareExchange(&g_RipTableLock, 1, 0) != 0)
        Sleep(0);
    bool allowed = false;
    int emptySlot = -1;
    for (int i = 0; i < 64; i++)
    {
        if (g_RipTable[i].Rip == Rip)
        {
            if (g_RipTable[i].Count < 32)
            {
                g_RipTable[i].Count++;
                allowed = true;
            }
            InterlockedExchange(&g_RipTableLock, 0);
            return allowed;
        }
        if (g_RipTable[i].Rip == 0 && emptySlot == -1)
            emptySlot = i;
    }
    if (emptySlot != -1)
    {
        g_RipTable[emptySlot].Rip = Rip;
        g_RipTable[emptySlot].Count = 1;
        allowed = true;
    }
    InterlockedExchange(&g_RipTableLock, 0);
    return allowed;
}

static LONG WINAPI GameServerVEH(PEXCEPTION_POINTERS ExInfo)
{
    const DWORD Code = ExInfo->ExceptionRecord->ExceptionCode;
    if (Code != EXCEPTION_ACCESS_VIOLATION &&
        Code != EXCEPTION_ILLEGAL_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;

    const ULONG_PTR Rip = ExInfo->ContextRecord->Rip;
    const ULONG_PTR FaultAddr = (Code == EXCEPTION_ACCESS_VIOLATION)
        ? ExInfo->ExceptionRecord->ExceptionInformation[1] : 0;
    const ULONG_PTR Rsp = ExInfo->ContextRecord->Rsp;

    const bool isInGsDll = (g_GsDllSize > 0 &&
        Rip >= g_GsDllBase &&
        Rip < g_GsDllBase + g_GsDllSize);
    const bool isNullDeref = (Code == EXCEPTION_ACCESS_VIOLATION &&
        FaultAddr < 0x10000ULL);

    // ANTI-CRASH FIX: recupera APENAS crashes onde o RIP esta em gs.dll.
    // Crashes no ENGINE devem ir para CONTINUE_SEARCH para que os __try/__except
    // no call stack (em ServerLoadingScreenDropped, ClientOnPawnDied, etc.) tratem.
    // O VEH antigo "recuperava" crashes do engine (RAX=0, pula caller) causando
    // cascata de null-deref que matava o servidor mesmo com __try/__except.
    if (!isInGsDll)
        return EXCEPTION_CONTINUE_SEARCH;

    // Anti-loop: descarta se recuperamos demais no mesmo RIP
    if (!AllowRipRecovery(Rip))
    {
        // Crash real � deixa cair para o handler do engine
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const LONG Count = InterlockedIncrement(&g_VehCount);

    // Log para diagn�stico
    {
        FILE* F = nullptr;
        fopen_s(&F, "C:\\gs_veh.log", "a");
        if (F)
        {
            const char* where = isInGsDll ? "gs.dll" : "engine (secondary)";
            fprintf(F,
                "[VEH] #%ld Code=0x%08X RIP=%s+0x%llX FaultAddr=0x%llX RSP=0x%llX\n",
                Count, (unsigned)Code,
                where,
                (unsigned long long)(isInGsDll ? (Rip - g_GsDllBase) : Rip),
                (unsigned long long)FaultAddr,
                (unsigned long long)Rsp);
            fclose(F);
        }
    }

    printf("[GS-VEH] #%ld Code=0x%08X RIP=0x%llX FaultAddr=0x%llX (inGS=%d)\n",
        Count, (unsigned)Code,
        (unsigned long long)Rip,
        (unsigned long long)FaultAddr,
        (int)isInGsDll);
    fflush(stdout);

    // Valida RSP e l� o endere�o de retorno
    if (Rsp == 0 || (Rsp & 0x7) != 0)
        return EXCEPTION_CONTINUE_SEARCH;

    ULONG_PTR RetAddr = 0;
    __try { RetAddr = *(ULONG_PTR*)Rsp; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return EXCEPTION_CONTINUE_SEARCH; }
    if (RetAddr == 0)
        return EXCEPTION_CONTINUE_SEARCH;

    // Simula "return 0/null" � RAX=0, avan�a RSP, pula para o caller
    ExInfo->ContextRecord->Rax = 0;
    ExInfo->ContextRecord->Rip = RetAddr;
    ExInfo->ContextRecord->Rsp += 8;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void InitVEH(HMODULE hDll)
{
    MODULEINFO ModInfo{};
    if (GetModuleInformation(GetCurrentProcess(), hDll, &ModInfo, sizeof(ModInfo)))
    {
        g_GsDllBase = (ULONG_PTR)ModInfo.lpBaseOfDll;
        g_GsDllSize = (ULONG_PTR)ModInfo.SizeOfImage;
    }
    else
    {
        g_GsDllBase = (ULONG_PTR)hDll;
        g_GsDllSize = 0x1000000;
    }
    ZeroMemory(g_RipTable, sizeof(g_RipTable));
    AddVectoredExceptionHandler(1, GameServerVEH);
    printf("[GS-VEH] Instalado (modo expandido: captura secondary crashes via stack scan).\n");
    printf("[GS-VEH] gs.dll range: 0x%llX - 0x%llX\n",
        (unsigned long long)g_GsDllBase,
        (unsigned long long)(g_GsDllBase + g_GsDllSize));
    fflush(stdout);
}

// ============================================================

inline void NullFuckingHOok() {}

DWORD WINAPI Main(LPVOID) {
    AllocConsole();
    FILE* File = nullptr;
    freopen_s(&File, "CONOUT$", "w+", stdout);
    MH_Initialize();
    SetConsoleTitleA("Initializating GameServer...");

    Sleep(5000);

    MH_CreateHook((LPVOID)(ImageBase + 0x5AD99C0), NullFuckingHOok, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + 0x36E1E30), NullFuckingHOok, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + 0x2653520), NullFuckingHOok, nullptr);
    MH_CreateHook((LPVOID)(ImageBase + 0x2b92040), NullFuckingHOok, nullptr);
    *(bool*)(InSDKUtils::GetImageBase() + Addresses::GIsClient) = false;
    *(bool*)(InSDKUtils::GetImageBase() + Addresses::GIsClient + 1) = true;

    if (Globals::bEnablephoebeDebug)
        *(bool*)(InSDKUtils::GetImageBase() + 0x9275BC0) = true;

    FortGameModeAthena::InitFortGameModeAthena();
    NetDriver::InitNetDriver();
    McpProfileGroup::InitMcpProfileGroup();
    FortGameSessionDedicated::InitializeHooks();
    PlayerController::InitFortPlayerController();
    Abilities::InitAbilitiesHooks();
    Challenges::InitChallenges();
    BuildingSMActor::InitBuildingSMActor();
    Looting::InitLooting();
    FortPlayerPawn::InitFortPlayerPawn();
    FortWeapon::InitFortWeapon();
    CosmeticLibraryBase::InitCosmeticLibraryBase();
    NavigationSystem::InitNavigationSystem();
    InventoryBaseOnSpawned::InitInventoryBaseOnSpawned();
    OnPossessedPawnDie::InitOnPossessedPawnDie();
    GetTrackingModifierInternal::InitGetTrackingModifierInternal();
    Creative::InitCreativeHooks();
    Teams::InitTeams();
    UFortAthenaAISpawnerData_BehaviorOnSpawned::InitUFortAthenaAISpawnerDataComponent_BehaviorOnSpawned();

    UFortEngine::GetEngine()->GameInstance->LocalPlayers.Remove(0);
    UGameplayStatics::OpenLevel(UWorld::GetWorld(), UKismetStringLibrary::Conv_StringToName(Globals::bCreative ? L"Creative_NoApollo_Terrain" : L"Apollo_Terrain"), true, FString());
    if (Globals::bCreative)
    {
        // Substitui o GameSession por uma classe base que não faz validação de reserva
        // Zerando o ponteiro faz o engine não chamar ValidatePlayer
        auto World = UWorld::GetWorld();
        if (World && World->AuthorityGameMode && World->AuthorityGameMode->GameSession)
        {
            // Destroi o GameSession atual que tem a validação de reserva
            World->AuthorityGameMode->GameSession->K2_DestroyActor();
            World->AuthorityGameMode->GameSession = nullptr;
        }
    }
    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, const DWORD ulReasonForCall)
{
    switch (ulReasonForCall)
    {
    case DLL_PROCESS_ATTACH:
        InitVEH(hModule);
        CreateThread(nullptr, 0, Main, nullptr, 0, nullptr);
        break;
    default:
        break;
    }
    return TRUE;
}