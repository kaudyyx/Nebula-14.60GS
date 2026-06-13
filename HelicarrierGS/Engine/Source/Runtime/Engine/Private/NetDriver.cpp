#include "pch.h"
#include "../Public/NetDriver.h"
#include <FortniteGame/Source/FortnitePhoebe/Public/FortnitePhoebe.h>

static bool                s_called = false;
static float               s_CloseTime = 0.f;
static bool                s_startedClose = false;
static bool                s_phoebeFirst = false;
static TArray<AActor*>     s_PlayerStarts;
static std::map<int, bool> s_ProcessedCases;
static EAthenaGamePhase    s_LastGamePhase = EAthenaGamePhase::Setup;

static void ResetMatchStatics()
{
    s_called = false;
    s_CloseTime = 0.f;
    s_startedClose = false;
    s_phoebeFirst = false;
    s_PlayerStarts.Free();
    s_ProcessedCases.clear();
    s_LastGamePhase = EAthenaGamePhase::Setup;
    printf("[NetDriver] ResetMatchStatics: statics resetados para nova sessao.\n");
    fflush(stdout);
}

bool NetDriver::Listen(UWorld* World, FURL& URL)
{
    static UNetDriver* (*CreateNetDriver_Local)(UEngine*, void*, FName, int) = decltype(CreateNetDriver_Local)(InSDKUtils::GetImageBase() + Addresses::CreateNetDriver);
    FName GameNetDriver = SDK::UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
    auto* newDriver = CreateNetDriver_Local(UEngine::GetEngine(), World, GameNetDriver, 0);

    HMODULE hMod = GetModuleHandleA(nullptr);
    uintptr_t base = (uintptr_t)hMod;
    uintptr_t nd = (uintptr_t)newDriver;
    if (!newDriver || nd < 0x10000 || (nd >= base && nd < base + 0x10000000))
    {
        // printf("[NetDriver] Invalid NetDriver ptr: %p\n", (void*)newDriver);
        return false;
    }

    UWorld::GetWorld()->NetDriver = newDriver;

    static void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + Addresses::SetWorld);
    static bool (*InitListen)(UNetDriver*, UWorld*, FURL, bool, FString) = decltype(InitListen)(InSDKUtils::GetImageBase() + Addresses::InitListen);

    UWorld::GetWorld()->NetDriver->NetDriverName = GameNetDriver;
    UWorld::GetWorld()->NetDriver->World = UWorld::GetWorld();
    UWorld::GetWorld()->NetDriver->NetServerMaxTickRate = 60.f;

    FString Error;
    for (auto& LevelCollection : UWorld::GetWorld()->LevelCollections)
        LevelCollection.NetDriver = UWorld::GetWorld()->NetDriver;

    SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());
    if (InitListen(UWorld::GetWorld()->NetDriver, UWorld::GetWorld(), URL, false, Error)) {}
    SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());
    return true;
}

void NetDriver::TickFlush(UNetDriver* Driver)
{
    static void (*ServerReplicateActors)(void*) =
        decltype(ServerReplicateActors)((*(void***)UFortReplicationGraph::StaticClass()->DefaultObject)[0x5e]);

    if (ServerReplicateActors && Driver->ReplicationDriver)
        ServerReplicateActors(Driver->ReplicationDriver);

    auto GM = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    auto GS = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

    if (!GM || !GS)
        return TickFlushOG(Driver);

    if (Globals::bCreative)
        return TickFlushOG(Driver);
    if (Globals::bCreative)
    {
        auto GM = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        if (GM && GM->GameSession)
        {
            auto FortSession = (AFortGameSession*)GM->GameSession;
            if (FortSession->ReservationBeaconHost)
            {
                FortSession->ReservationBeaconHost->K2_DestroyActor();
                FortSession->ReservationBeaconHost = nullptr;
            }
        }
        return TickFlushOG(Driver);
    }

    if (GM && GS)
    {
        EAthenaGamePhaseStep CurrentGamePhaseStep = GetCurrentGamePhaseStep(GM, GS);
        GS->GamePhaseStep = CurrentGamePhaseStep;
        if (GS->GamePhase <= EAthenaGamePhase::Warmup &&
            s_LastGamePhase > EAthenaGamePhase::Warmup)
        {
            ResetMatchStatics();
        }
        s_LastGamePhase = GS->GamePhase;
    }

    if (Globals::bEnablephoebe)
    {
        auto GameMode = (AFortGameModeAthena*)Driver->World->AuthorityGameMode;
        auto GameState = (AFortGameStateAthena*)Driver->World->GameState;

        if (!s_phoebeFirst)
        {
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(
                UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &s_PlayerStarts);
            s_phoebeFirst = true;
        }

        if (GameState->GamePhase < EAthenaGamePhase::Aircraft &&
            GameMode->AlivePlayers.Num() > 0 &&
            (GameMode->AlivePlayers.Num() + GameMode->AliveBots.Num()) < 100)
        {
            if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.07))
            {
                AActor* SpawnLocator = s_PlayerStarts[
                    UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(
                        0, s_PlayerStarts.Num() - 1)];
                if (SpawnLocator)
                {
                    SpawnedBots.push_back(new Bot(SpawnLocator));
                }
            }
        }

        for (size_t i = 0; i < SpawnedBots.size(); i++)
        {
            SpawnedBots[i]->Tick();
        }
    }

    if (GS->GamePhase > EAthenaGamePhase::Aircraft)
    {
        int AliveCount = GM->AlivePlayers.Num();
        if (s_ProcessedCases.find(AliveCount) == s_ProcessedCases.end())
        {
            std::map<int, int> PlacementPoints = {
                {25, 60}, {15, 30}, {5, 30}, {1, 60}
            };
            auto it = PlacementPoints.find(AliveCount);
            if (it != PlacementPoints.end())
            {
                for (auto& Player : GM->AlivePlayers)
                {
                    Player->ClientReportTournamentPlacementPointsScored(AliveCount, it->second);
                }
                s_ProcessedCases[AliveCount] = true;
            }
        }
    }

    float TimeSeconds = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
    if (TimeSeconds > GS->WarmupCountdownStartTime + GM->WarmupCountdownDuration && !s_called)
    {
        s_called = true;
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"startaircraft", nullptr);
        Globals::startingplayers = to_string(GM->AlivePlayers.Num());
    }

    if (GS->GamePhase == EAthenaGamePhase::EndGame)
    {
        if (!s_startedClose)
        {
            s_CloseTime = TimeSeconds + 15.f;
            s_startedClose = true;
        }
        else
        {
            if (TimeSeconds >= s_CloseTime)
            {
                TerminateProcess(GetCurrentProcess(), 0);
            }
        }
    }

    return TickFlushOG(Driver);
}

void NetDriver::InitNetDriver()
{
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + Addresses::TickFlush), TickFlush, (LPVOID*)&TickFlushOG);
}