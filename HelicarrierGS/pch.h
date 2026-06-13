#pragma once
#define WIN32_LEAN_AND_MEAN
#define CURL_STATICLIB
#define NOMINMAX
#define _HAS_STD_BYTE 0   // you need this for imgui includes bruh

#include <windows.h>
#include <thread>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <map>
#include <intrin.h>
#include <sstream>
#include <array>
#include <tlhelp32.h>
#include <future>
#include <set>
#include <random>
#include <chrono>

#pragma warning(disable : 4099)
#pragma warning(disable : 4309)
#pragma warning(disable : 4369)
#pragma warning(disable : 4244)

#include <SDK/Source/Runtime/DevelopmentKit/SDK.hpp>
using namespace SDK;
using namespace std;
#include <SDK/Source/Runtime/DevelopmentKit/UnrealContainers.hpp>

#include <Engine/Source/ThirdParty/Libraries/Minhook/MinHook.h>
#include <Engine/Source/ThirdParty/Libraries/curl/curl.h>
#include <Engine/Source/ThirdParty/Libraries/Memcury/Memcury.h>
#include <Engine/Source/ThirdParty/Offsets/Offsets.h>
#include <Engine/Source/ThirdParty/FFrame/Public/FFrame.h>
#include <Engine/Source/ThirdParty/Globals/Globals.h>

static void WaitForLogin() {
    // Wait until we are at the login screen smh ig it works
    FName Frontend = UKismetStringLibrary::Conv_StringToName(L"Frontend");
    FName MatchState = UKismetStringLibrary::Conv_StringToName(L"InProgress");

    while (true) {
        UWorld* CurrentWorld = ((UWorld*)UWorld::GetWorld());
        if (CurrentWorld) {
            if (CurrentWorld->Name == Frontend) {
                auto GameMode = (AGameMode*)CurrentWorld->AuthorityGameMode;
                if (GameMode->GetMatchState() == MatchState) {
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 1));
}

using namespace UC;
template <class X>
using xset = set<X, TMemoryAllocator<X>>;
template <class X>
using xvector = vector<X, TMemoryAllocator<X>>;
template <class X, class Y>
using xmap = map<X, Y, less<X>, TMemoryAllocator<pair<const X, Y>>>;

struct FSpawnPickupData
{
    UFortItemDefinition* ItemDefinition;
    FVector Location;
    FRotator Rotation = FRotator(0, 0, 0);
    bool bRandomRotation = Rotation == FRotator(0, 0, 0);
    int Count = 1;
    int LoadedAmmo = -1;
    AFortPawn* PickupOwner = nullptr;
    EFortPickupSourceTypeFlag FortPickupSourceTypeFlag = EFortPickupSourceTypeFlag::Tossed;
    EFortPickupSpawnSource FortPickupSpawnSource = EFortPickupSpawnSource::Unset;
};


inline void ModifyInstruction(uintptr_t pinstrAddr, uintptr_t pDetourAddr)
{
    uint8_t* DetourAddr = (uint8_t*)pDetourAddr;
    uint8_t* instrAddr = (uint8_t*)pinstrAddr;
    int64_t delta = (int64_t)(DetourAddr - (instrAddr + 5));
    auto addr = reinterpret_cast<int32_t*>(instrAddr + 1);
    DWORD dwProtection;
    VirtualProtect(addr, 4, PAGE_EXECUTE_READWRITE, &dwProtection);

    *addr = static_cast<int32_t>(delta);

    DWORD dwTemp;
    VirtualProtect(addr, 4, dwProtection, &dwTemp);
}

inline std::vector<std::string> BlockedIDS = {
    "CID_563_Athena_Commando_M_RebirthDefaultD",
    "CID_562_Athena_Commando_M_RebirthDefaultC",
    "CID_561_Athena_Commando_M_RebirthDefaultB",
    "CID_560_Athena_Commando_M_RebirthDefaultA",
    "CID_559_Athena_Commando_F_RebirthDefaultD",
    "CID_558_Athena_Commando_F_RebirthDefaultC",
    "CID_557_Athena_Commando_F_RebirthDefaultB",
    "CID_556_Athena_Commando_F_RebirthDefaultA",
    //"CID_TBD_Athena_Commando_M_Nutcracker_CINE",
    //"CID_TBD_Athena_Commando_M_ConstructorTest",
    //"CID_TBD_Athena_Commando_M_Banana_CINE",
    //"CID_TBD_Athena_Commando_F_ConstructorTest",
    //"CID_NPC_Athena_Commando_M_TacticalFishermanOil",
    //"CID_NPC_Athena_Commando_M_PaddedArmorArctic",
    //"CID_NPC_Athena_Commando_M_HenchmanGood",
    //"CID_NPC_Athena_Commando_M_HenchmanBad",
    //"CID_NPC_Athena_Commando_M_PaddedArmorArctic",

    "CID_636_Athena_Commando_M_GalileoGondola_78MFZ",
    "CID_637_Athena_Commando_M_GalileoOutrigger_7Q0YU",
    "CID_707_Athena_Commando_M_HenchmanGood",
    "CID_706_Athena_Commando_M_HenchmanBad",

    // emotes

    "EID_HolidayCracker",
    "EID_SecretHandshake",
    "EID_Galileo4_PXPE0",
    "EID_Galileo3_T4DKO",
    "EID_Galileo2_2VYEJ",
    "EID_Galileo1_B3EX6",
    "EID_Galileo1_B3EX6",
    "EID_Galileo1_B3EX6",
};

static inline void ModifyInstructionLEA(uintptr_t instrAddr, uintptr_t targetAddr, int offset)
{
    int64_t delta = static_cast<int64_t>(targetAddr) -
        static_cast<int64_t>(instrAddr + offset + 4);

    auto patchLocation = reinterpret_cast<int32_t*>(instrAddr + offset);

    DWORD oldProtect;
    VirtualProtect(patchLocation, sizeof(int32_t), PAGE_EXECUTE_READWRITE, &oldProtect);

    *patchLocation = static_cast<int32_t>(delta);

    DWORD temp;
    VirtualProtect(patchLocation, sizeof(int32_t), oldProtect, &temp);
}

static void Hook(uint64 Address, void* Detour, void** OG = nullptr)
{
    if (Address == 0x0) return;
    MH_CreateHook(LPVOID(Address), Detour, OG);
    MH_EnableHook(LPVOID(Address));
}


template <typename _Is>
static __forceinline void Patch(uintptr_t ptr, _Is byte)
{
    DWORD og;
    VirtualProtect(LPVOID(ptr), sizeof(_Is), PAGE_EXECUTE_READWRITE, &og);
    *(_Is*)ptr = byte;
    VirtualProtect(LPVOID(ptr), sizeof(_Is), og, &og);
}

inline __forceinline void PatchByte(uintptr_t ptr, uint8_t byte)
{
    DWORD og;
    VirtualProtect(LPVOID(ptr), 1, PAGE_EXECUTE_READWRITE, &og);
    *(uint8_t*)(ptr) = byte;
    VirtualProtect(LPVOID(ptr), 1, og, &og);
}

template<typename Class = UObject>
static void VirtualSwap(void** VTable, int Index, void* Detour, void** Original = nullptr)
{
    if (Original)
        *Original = VTable[Index];

    DWORD dwProt;
    VirtualProtect(&VTable[Index], sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProt);

    VTable[Index] = Detour;

    DWORD dwTemp;
    VirtualProtect(&VTable[Index], sizeof(void*), dwProt, &dwTemp);
}

