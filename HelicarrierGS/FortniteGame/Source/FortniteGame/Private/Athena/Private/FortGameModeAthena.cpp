#include "pch.h"
#include "../../../Public/Athena/Public/FortGameModeAthena.h"
#include <Engine/Source/Runtime/Engine/Public/NetDriver.h>
#include <Engine/Source/ThirdParty/Utils/Utils.h>
#include <Engine/Source/ThirdParty/Utils/CrashGuard.h>
#include <Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/Abilities.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>
#include <FortniteGame/Source/FortniteGame/Public/QuestManager/Public/QuestManage.h>
#include <FortniteGame/Source/FortniteAI/Public/Spawners/Public/AISpawner.h>
#include <FortniteGame/Source/FortnitePhoebe/Public/FortnitePhoebe.h>
#include <FortniteGame/Source/FortniteGame/Public/Creative/Public/Creative.h>
#include <Engine/Source/ThirdParty/API/Header/API.h>
#include <winhttp.h>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <Engine/Source/ThirdParty/Libraries/json/json.hpp>
#include <random>
#include <chrono>
#include <thread>

static __declspec(noinline) void SafeCallStartNewSafeZonePhaseOG(AFortGameModeAthena* GameMode, int a2)
{
    if (!GameMode || !GameMode->GameState)
        return;
    __try
    {
        FortGameModeAthena::StartNewSafeZonePhaseOG(GameMode, a2);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {

    }
}


static __declspec(noinline) void SafeAircraftJump(void* RawPlayer)
{
    __try
    {

        if (!RawPlayer)
            return;
        auto* Player = (AFortPlayerControllerAthena*)RawPlayer;
        if (!Player->PlayerState)
            return;
        if (!Player->IsInAircraft())
            return;
        auto* AircraftComp = Player->GetAircraftComponent();
        if (AircraftComp)
            AircraftComp->ServerAttemptAircraftJump({});
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {

    }
}

static int GetNebulaListenPort()
{
    static int Port = 0;
    if (Port == 0)
    {
        auto Seed = static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::mt19937 Rng(Seed);
        std::uniform_int_distribution<int> Dist(20000, 60000);
        Port = Dist(Rng);
    }
    return Port;
}

template<typename T>
T* GetClosestActor(AActor* FromActor, float Max = 500)
{
    TArray<AActor*> ActorArray;
    UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), T::StaticClass(), &ActorArray);
    AActor* Ret = nullptr;
    for (auto Actor : ActorArray)
    {
        if (!Actor || Actor->GetDistanceTo(FromActor) > Max)
            continue;
        if (!Ret || Actor->GetDistanceTo(FromActor) < Ret->GetDistanceTo(FromActor))
            Ret = Actor;
    }
    ActorArray.Free();
    return (T*)Ret;
}

struct FTeamReservationState
{
    std::string GroupKey;
    int TeamIndex = -1;
    int ExpectedSize = 0;
    std::unordered_set<std::string> JoinedUsernames;
};

static std::vector<FTeamReservationState> GTeamSyncReservations;

static std::string TeamSyncJoinUsernames(const std::vector<std::string>& Usernames)
{
    std::string Out;
    for (size_t i = 0; i < Usernames.size(); ++i)
    {
        if (i > 0)
            Out += ", ";
        Out += Usernames[i];
    }
    return Out;
}

static void TeamSyncLog(const std::string& Message)
{
    // printf("[TeamSync] %s\n", Message.c_str());
    fflush(stdout);
    FILE* File = nullptr;
    fopen_s(&File, "C:\\team_sync.log", "a");
    if (File)
    {
        //  fprintf(File, "[TeamSync] %s\n", Message.c_str());
        fclose(File);
    }
}

static std::string TeamSyncTrim(const std::string& Value)
{
    size_t Start = 0;
    size_t End = Value.size();
    while (Start < End && isspace((unsigned char)Value[Start]))
        ++Start;
    while (End > Start && isspace((unsigned char)Value[End - 1]))
        --End;
    return Value.substr(Start, End - Start);
}

static std::string TeamSyncNormalizeUsername(const std::string& Value)
{
    std::string Out = TeamSyncTrim(Value);
    std::transform(Out.begin(), Out.end(), Out.begin(), [](unsigned char C) { return (char)std::tolower(C); });
    return Out;
}

static bool TeamSyncContainsUsername(const std::vector<std::string>& Members, const std::string& Username)
{
    for (const std::string& Member : Members)
    {
        if (Member == Username)
            return true;
    }
    return false;
}

static void TeamSyncAddUsername(std::vector<std::string>& OutUsernames, const std::string& Username)
{
    std::string Normalized = TeamSyncNormalizeUsername(Username);
    if (Normalized.empty())
        return;
    if (!TeamSyncContainsUsername(OutUsernames, Normalized))
        OutUsernames.push_back(Normalized);
}

static bool TeamSyncIsTeamPlaylist(UFortPlaylistAthena* Playlist)
{
    return Playlist && Playlist->MaxTeamSize > 1 && Playlist->MaxTeamCount > 1;
}

static void TeamSyncResetState()
{
    GTeamSyncReservations.clear();
    TeamSyncLog("state reset for new playlist/match");
}

static bool UniqueNetIdReplEquals(const FUniqueNetIdRepl& A, const FUniqueNetIdRepl& B)
{
    const unsigned char* PA = reinterpret_cast<const unsigned char*>(&A);
    const unsigned char* PB = reinterpret_cast<const unsigned char*>(&B);
    for (int i = 0; i < (int)sizeof(FUniqueNetIdRepl); ++i)
    {
        if (PA[i] != PB[i])
            return false;
    }
    return true;
}

static std::wstring TeamSyncUtf8ToWide(const std::string& Value)
{
    if (Value.empty())
        return std::wstring();
    int Required = MultiByteToWideChar(CP_UTF8, 0, Value.c_str(), (int)Value.size(), nullptr, 0);
    if (Required <= 0)
        return std::wstring();
    std::wstring Out;
    Out.resize(Required);
    MultiByteToWideChar(CP_UTF8, 0, Value.c_str(), (int)Value.size(), &Out[0], Required);
    return Out;
}

static std::string TeamSyncUrlEncode(const std::string& Value)
{
    static const char* Hex = "0123456789ABCDEF";
    std::string Out;
    for (unsigned char C : Value)
    {
        if ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '-' || C == '_' || C == '.' || C == '~')
        {
            Out.push_back((char)C);
        }
        else
        {
            Out.push_back('%');
            Out.push_back(Hex[(C >> 4) & 0xF]);
            Out.push_back(Hex[C & 0xF]);
        }
    }
    return Out;
}

static bool TeamSyncHttpGet(const std::string& Url, std::string& OutBody, DWORD* OutStatus = nullptr)
{
    OutBody.clear();
    if (OutStatus)
        *OutStatus = 0;
    std::wstring WideUrl = TeamSyncUtf8ToWide(Url);
    if (WideUrl.empty())
        return false;
    URL_COMPONENTSW UrlComp{};
    wchar_t HostName[256]{};
    wchar_t UrlPath[2048]{};
    wchar_t ExtraInfo[2048]{};
    UrlComp.dwStructSize = sizeof(UrlComp);
    UrlComp.lpszHostName = HostName;
    UrlComp.dwHostNameLength = _countof(HostName);
    UrlComp.lpszUrlPath = UrlPath;
    UrlComp.dwUrlPathLength = _countof(UrlPath);
    UrlComp.lpszExtraInfo = ExtraInfo;
    UrlComp.dwExtraInfoLength = _countof(ExtraInfo);
    if (!WinHttpCrackUrl(WideUrl.c_str(), 0, 0, &UrlComp))
        return false;
    std::wstring Host = UrlComp.lpszHostName ? UrlComp.lpszHostName : L"";
    std::wstring Path = UrlComp.lpszUrlPath ? UrlComp.lpszUrlPath : L"";
    if (UrlComp.lpszExtraInfo)
        Path += UrlComp.lpszExtraInfo;
    HINTERNET Session = WinHttpOpen(L"NebulaPlaylistTeamSync/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!Session)
        return false;
    WinHttpSetTimeouts(Session, 2000, 2000, 2000, 2000);
    HINTERNET Connect = WinHttpConnect(Session, Host.c_str(), UrlComp.nPort, 0);
    if (!Connect)
    {
        WinHttpCloseHandle(Session);
        return false;
    }
    DWORD Flags = UrlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET Request = WinHttpOpenRequest(Connect, L"GET", Path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, Flags);
    if (!Request)
    {
        WinHttpCloseHandle(Connect);
        WinHttpCloseHandle(Session);
        return false;
    }
    BOOL Sent = WinHttpSendRequest(Request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!Sent || !WinHttpReceiveResponse(Request, nullptr))
    {
        WinHttpCloseHandle(Request);
        WinHttpCloseHandle(Connect);
        WinHttpCloseHandle(Session);
        return false;
    }
    DWORD StatusCode = 0;
    DWORD StatusCodeSize = sizeof(StatusCode);
    WinHttpQueryHeaders(Request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &StatusCode, &StatusCodeSize, WINHTTP_NO_HEADER_INDEX);
    if (OutStatus)
        *OutStatus = StatusCode;
    for (;;)
    {
        DWORD Available = 0;
        if (!WinHttpQueryDataAvailable(Request, &Available) || Available == 0)
            break;
        std::string Buffer;
        Buffer.resize(Available);
        DWORD Read = 0;
        if (!WinHttpReadData(Request, &Buffer[0], Available, &Read) || Read == 0)
            break;
        OutBody.append(Buffer.data(), Read);
    }
    WinHttpCloseHandle(Request);
    WinHttpCloseHandle(Connect);
    WinHttpCloseHandle(Session);
    return StatusCode >= 200 && StatusCode < 300;
}

static void TeamSyncCollectUsernamesFromMemberEntry(const nlohmann::json& Value, std::vector<std::string>& OutUsernames)
{
    if (Value.is_string())
    {
        TeamSyncAddUsername(OutUsernames, Value.get<std::string>());
        return;
    }
    if (!Value.is_object())
        return;
    static const std::vector<std::string> UsernameKeys = {
        "username", "userName", "displayName", "name", "accountId", "accountID", "id"
    };
    for (const std::string& Key : UsernameKeys)
    {
        auto It = Value.find(Key);
        if (It != Value.end() && It->is_string())
            TeamSyncAddUsername(OutUsernames, It->get<std::string>());
    }
}

static std::string TeamSyncBuildGroupKey(const std::vector<std::string>& Members)
{
    std::vector<std::string> SortedMembers = Members;
    std::sort(SortedMembers.begin(), SortedMembers.end());
    return std::string("members:") + TeamSyncJoinUsernames(SortedMembers);
}

static std::vector<std::string> TeamSyncParsePartyMembers(const std::string& Body, const std::string& Username)
{
    std::vector<std::string> Members;
    try
    {
        nlohmann::json Parsed = nlohmann::json::parse(Body);
        if (Parsed.is_object())
        {
            auto MembersIt = Parsed.find("members");
            if (MembersIt != Parsed.end() && MembersIt->is_array())
            {
                for (const auto& Entry : *MembersIt)
                    TeamSyncCollectUsernamesFromMemberEntry(Entry, Members);
            }
            else
            {
                TeamSyncCollectUsernamesFromMemberEntry(Parsed, Members);
            }
        }
        else if (Parsed.is_array())
        {
            for (const auto& Entry : Parsed)
                TeamSyncCollectUsernamesFromMemberEntry(Entry, Members);
        }
        else if (Parsed.is_string())
        {
            TeamSyncAddUsername(Members, Parsed.get<std::string>());
        }
    }
    catch (...)
    {
        size_t Start = 0;
        while (Start < Body.size())
        {
            size_t End = Body.find(',', Start);
            std::string Token = Body.substr(Start, End == std::string::npos ? std::string::npos : End - Start);
            TeamSyncAddUsername(Members, Token);
            if (End == std::string::npos)
                break;
            Start = End + 1;
        }
    }
    TeamSyncAddUsername(Members, Username);
    TeamSyncLog(std::string("parsed party members for ") + Username + ": [" + TeamSyncJoinUsernames(Members) + "]");
    return Members;
}

static std::vector<std::string> TeamSyncFetchPartyMembers(const std::string& Username)
{
    std::string Url = std::string("http://187.77.254.242/team?username=") + TeamSyncUrlEncode(Username);
    TeamSyncLog(std::string("requesting api for ") + Username + " url=" + Url);
    std::string Body;
    DWORD StatusCode = 0;
    bool bOk = TeamSyncHttpGet(Url, Body, &StatusCode);
    std::string LoggedBody = Body;
    if (LoggedBody.size() > 1024)
        LoggedBody = LoggedBody.substr(0, 1024) + "...";
    TeamSyncLog(std::string("api response for ") + Username + " status=" + std::to_string(StatusCode) + " body=" + LoggedBody);
    if (!bOk)
    {
        std::vector<std::string> FallbackMembers;
        TeamSyncAddUsername(FallbackMembers, Username);
        return FallbackMembers;
    }
    return TeamSyncParsePartyMembers(Body, Username);
}

static int TeamSyncFindExistingGroupTeam(AFortGameStateAthena* GameState, const std::vector<std::string>& PartyMembers)
{
    if (!GameState)
        return -1;
    for (int32 i = 0; i < GameState->PlayerArray.Num(); ++i)
    {
        AFortPlayerStateAthena* ExistingPlayerState = Utils::Cast<AFortPlayerStateAthena>(GameState->PlayerArray[i]);
        if (!ExistingPlayerState)
            continue;
        std::string ExistingUsername = TeamSyncNormalizeUsername(ExistingPlayerState->GetPlayerName().ToString());
        if (!TeamSyncContainsUsername(PartyMembers, ExistingUsername))
            continue;
        if (ExistingPlayerState->TeamIndex > 0)
        {
            TeamSyncLog(std::string("found existing party member ") + ExistingUsername + " already on team " + std::to_string(ExistingPlayerState->TeamIndex));
            return ExistingPlayerState->TeamIndex;
        }
    }
    return -1;
}

static int TeamSyncFindReservedGroupTeam(const std::string& GroupKey, const std::string& Username)
{
    if (GroupKey.empty())
        return -1;
    for (size_t i = 0; i < GTeamSyncReservations.size(); ++i)
    {
        FTeamReservationState& Reservation = GTeamSyncReservations[i];
        if (Reservation.GroupKey != GroupKey || Reservation.TeamIndex <= 0)
            continue;
        if (!Username.empty())
            Reservation.JoinedUsernames.insert(Username);
        TeamSyncLog(std::string("using reserved team ") + std::to_string(Reservation.TeamIndex) + " for groupKey=" + GroupKey + " joined=" + std::to_string((int)Reservation.JoinedUsernames.size()) + "/" + std::to_string(Reservation.ExpectedSize));
        return Reservation.TeamIndex;
    }
    return -1;
}

static void TeamSyncReserveGroupTeam(const std::string& GroupKey, int TeamIndex, int ExpectedSize, const std::string& Username)
{
    if (GroupKey.empty() || TeamIndex <= 0)
        return;
    for (size_t i = 0; i < GTeamSyncReservations.size(); ++i)
    {
        FTeamReservationState& Reservation = GTeamSyncReservations[i];
        if (Reservation.GroupKey != GroupKey)
            continue;
        Reservation.TeamIndex = TeamIndex;
        Reservation.ExpectedSize = std::max(Reservation.ExpectedSize, ExpectedSize);
        if (!Username.empty())
            Reservation.JoinedUsernames.insert(Username);
        TeamSyncLog(std::string("updated reserved team ") + std::to_string(TeamIndex) + " for groupKey=" + GroupKey + " expected=" + std::to_string(Reservation.ExpectedSize) + " joined=" + std::to_string((int)Reservation.JoinedUsernames.size()));
        return;
    }
    FTeamReservationState Reservation;
    Reservation.GroupKey = GroupKey;
    Reservation.TeamIndex = TeamIndex;
    Reservation.ExpectedSize = ExpectedSize;
    if (!Username.empty())
        Reservation.JoinedUsernames.insert(Username);
    GTeamSyncReservations.push_back(Reservation);
    TeamSyncLog(std::string("reserved team ") + std::to_string(TeamIndex) + " for groupKey=" + GroupKey + " expected=" + std::to_string(Reservation.ExpectedSize) + " joined=" + std::to_string((int)Reservation.JoinedUsernames.size()));
}

static int TeamSyncCountPendingReservationsForTeam(int TeamIndex)
{
    if (TeamIndex <= 0)
        return 0;
    int PendingReservations = 0;
    for (size_t i = 0; i < GTeamSyncReservations.size(); ++i)
    {
        const FTeamReservationState& Reservation = GTeamSyncReservations[i];
        if (Reservation.TeamIndex != TeamIndex)
            continue;
        int StillPending = Reservation.ExpectedSize - (int)Reservation.JoinedUsernames.size();
        if (StillPending > 0)
            PendingReservations += StillPending;
    }
    return PendingReservations;
}

static EFortTeam TeamSyncChooseAvailableTeam(AFortGameStateAthena* GameStateAthena, UFortPlaylistAthena* PlaylistAthena, int RequiredOpenSlots)
{
    if (!GameStateAthena || !PlaylistAthena)
        return EFortTeam::MAX;
    int32 MaxTeamCount = PlaylistAthena->MaxTeamCount;
    int32 MaxTeamSize = PlaylistAthena->MaxTeamSize;
    EFortTeam ChosenTeam = EFortTeam::MAX;
    int32 MinimumPlayers = INT32_MAX;
    for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
    {
        AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
        if (!TeamInfo)
            continue;
        if (i >= MaxTeamCount)
            break;
        int32 PendingReservations = TeamSyncCountPendingReservationsForTeam(TeamInfo->Team);
        int32 TeamMembersSize = TeamInfo->TeamMembers.Num();
        int32 EffectivePlayers = TeamMembersSize + PendingReservations;
        int32 OpenSlots = MaxTeamSize - EffectivePlayers;
        TeamSyncLog(std::string("team candidate ") + std::to_string(TeamInfo->Team) + " members=" + std::to_string(TeamMembersSize) + " pending=" + std::to_string(PendingReservations) + " effective=" + std::to_string(EffectivePlayers) + " open=" + std::to_string(OpenSlots) + " required=" + std::to_string(RequiredOpenSlots));
        if (OpenSlots < RequiredOpenSlots)
            continue;
        if (PlaylistAthena->bIsLargeTeamGame || PlaylistAthena->bAllowTeamSwitching)
        {
            if (EffectivePlayers < MinimumPlayers)
            {
                MinimumPlayers = EffectivePlayers;
                ChosenTeam = EFortTeam(TeamInfo->Team);
            }
        }
        else
        {
            ChosenTeam = EFortTeam(TeamInfo->Team);
            break;
        }
    }
    if (ChosenTeam != EFortTeam::MAX)
        TeamSyncLog(std::string("selected available team ") + std::to_string(static_cast<int>(ChosenTeam)) + " for required slots " + std::to_string(RequiredOpenSlots));
    else
        TeamSyncLog(std::string("no available team found for required slots ") + std::to_string(RequiredOpenSlots));
    return ChosenTeam;
}

// Guard para garantir que StartAircraftPhase rode apenas uma vez por partida.
// O engine chama a funcao duas vezes no mesmo tick em alguns casos.
static bool s_bAircraftPhaseInitialized = false;
// Centro XY da safe zone do lategame (preenchido em StartAircraftPhase_Impl)
static FVector s_LateGameSafeZoneCenter{};
static bool    s_LateGameSafeZoneCenterSet = false;

// Forward declarations — funcoes livres cujos corpos vem depois dos wrappers
static EFortTeam PickTeam_Impl(AFortGameModeAthena* GameModeAthena, EFortTeam PreferredTeam, AFortPlayerControllerAthena* PlayerControllerAthena);
static void __fastcall Storm_Impl(__int64 a1, int a2);
// _Impl como funcoes estaticas livres — evita declaracao no header e problemas de escopo
static bool ReadyToStartMatch_Impl(AFortGameModeAthena* GameMode);
static APawn* SpawnDefaultPawnFor_Impl(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot);
static void StartNewSafeZonePhase_Impl(AFortGameModeAthena* GameMode, int a2);
static bool StartAircraftPhase_Impl(AFortGameModeAthena* GameMode, char a2);
static void HandleStartingNewPlayerHook_Impl(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer);

bool FortGameModeAthena::ReadyToStartMatch(AFortGameModeAthena* GameMode)
{
    bool _r{};
    __try { _r = ReadyToStartMatch_Impl(GameMode); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

static bool ReadyToStartMatch_Impl(AFortGameModeAthena* GameMode)
{
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    auto World = UWorld::GetWorld();
    if (!GameState || !World) return false;
    if (!Globals::bCreative)
    {
        TArray<AActor*> WarmupSpots;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &WarmupSpots);
        int WarmupSpotsNum = WarmupSpots.Num();
        WarmupSpots.Free();
        if (WarmupSpotsNum == 0)
            return false;
    }
    if (!Globals::bCreative)
    {
        bool bInitializatePOIManager = false;
        if (!bInitializatePOIManager)
        {
            UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), &Utils::BuildingFoundations);
            for (size_t i = 0; i < Utils::BuildingFoundations.Num(); i++)
            {
                ABuildingFoundation* Foundation = Utils::Cast<ABuildingFoundation>(Utils::BuildingFoundations[i]);
                if (!Foundation) continue;
                if (Foundation->GetName().contains("Lobby")) continue;
                Utils::ShowFoundation(Foundation);
            }
            bInitializatePOIManager = true;
        }
    }
    if (!GameState->MapInfo) return false;
    static bool bInitialized = false;
    if (!bInitialized)
    {
        bInitialized = true;
        s_bAircraftPhaseInitialized = false;
        if (GameMode->CurrentPlaylistId == -1) {
            TeamSyncResetState();
            UFortPlaylistAthena* Playlist = Utils::StaticFindObject<UFortPlaylistAthena>(Globals::bEnableBattleLab ? "/Game/Athena/Playlists/Unvaulted/Playlist_Unvaulted_Solo.Playlist_Unvaulted_Solo" : (Globals::bCreative ? "/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2" : (Globals::IsTournamentModeEnabled() ? "/Game/Athena/Playlists/Showdown/Tournament/playlist_showdownalt_duos.playlist_showdownalt_duos" : (Globals::IsArenaModeEnabled() ? "/Game/Athena/Playlists/Showdown/Playlist_showdownalt_solo.Playlist_showdownalt_solo" : "/Game/Athena/Playlists/Low/Playlist_Low_Solo.Playlist_Low_Solo"))));

            if (!Playlist) return false;

            GameMode->CurrentPlaylistId = GameState->CurrentPlaylistId = Playlist->PlaylistId;
            GameMode->CurrentPlaylistName = Playlist->PlaylistName;
            GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
            GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
            GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
            GameState->CurrentPlaylistInfo.MarkArrayDirty();
            GameState->GamePhase = EAthenaGamePhase::Warmup;
            GameState->OnRep_GamePhase(EAthenaGamePhase::Setup);
            GameState->OnRep_CurrentPlaylistId();
            GameState->OnRep_CurrentPlaylistInfo();
            GameMode->WarmupRequiredPlayerCount = 1;
            GameMode->DefaultPawnClass = APlayerPawn_Athena_C::StaticClass();
            Playlist->GarbageCollectionFrequency = 9999999999999999.f;
            GameMode->PlaylistHotfixOriginalGCFrequency = 9999999999999999.f;
            GameMode->bDisableGCOnServerDuringMatch = true;
            GameMode->bPlaylistHotfixChangedGCDisabling = true;
            GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
            GameState->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;
            if (Globals::bCreative)
            {
                GameState->bGameModeWillSkipAircraft = true;
                Playlist->bSkipAircraft = true;
                Playlist->bSkipWarmup = true;
                GameMode->WarmupRequiredPlayerCount = 0;
            }
            GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
            if (!Globals::bCreative)
            {
                GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
            }
            GameState->OnRep_Aircraft();
            if (Globals::bCreative)
            {
                GameState->bGameModeWillSkipAircraft = true;
                Playlist->bSkipAircraft = true;
                GameMode->WarmupRequiredPlayerCount = 0;
                GameState->WarmupCountdownEndTime = 0.f;
                GameState->SafeZonesStartTime = 0.f;
                GameState->OnRep_Aircraft();
            }
            if (Globals::bLateGame)
            {
                GameState->DefaultParachuteDeployTraceForGroundDistance = 1000.f;
            }
            if (!Globals::bCreative)
            {
                GameState->DefaultRebootMachineHotfix = 1;
                Playlist->bAutoAcquireSpawnChip = true;
                TArray<AActor*> SpawnMachines;
                UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingGameplayActorSpawnMachine::StaticClass(), &SpawnMachines);
                for (auto Machine : SpawnMachines)
                {
                    auto Van = Utils::Cast<ABuildingGameplayActorSpawnMachine>(Machine);
                    if (!Van)
                        continue;
                    Van->ResurrectLocation = GetClosestActor<AFortPlayerStart>(Van);
                }
                SpawnMachines.Free();
            }
            if (Globals::bEnableBattleLab || Globals::bCreative)
                if (Globals::bCreative)
                {
                    if (GameMode->GameSession)
                    {
                        GameMode->GameSession->MaxPlayers = 100;
                        auto FortSession = (AFortGameSession*)GameMode->GameSession;
                        if (FortSession->ReservationBeaconHost)
                        {
                            FortSession->ReservationBeaconHost->K2_DestroyActor();
                            FortSession->ReservationBeaconHost = nullptr;
                        }
                    }
                }
            {
                Playlist->bAllowJoinInProgress = true;
                Playlist->JoinInProgressMatchType = UKismetTextLibrary::Conv_StringToText(FString(L"Creative"));
            }
            if (!Globals::bCreative || !Globals::bEnableBattleLab || !Globals::bLateGame)
            {
                if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
                {
                    GameMode->ServerBotManager = BotManager;
                    BotManager->CachedGameState = GameState;
                    BotManager->CachedGameMode = GameMode;
                    BotManager->CachedBotMutator = Utils::SpawnActor22<AFortAthenaMutator_Bots>({});
                    Utils::BotMutator = Utils::SpawnActor22<AFortAthenaMutator_Bots>({}, {}, nullptr, Utils::StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_Phoebe_Mutator.BP_Phoebe_Mutator_C"));
                    Utils::BotMutator->CachedGameMode = GameMode;
                    Utils::BotMutator->CachedGameState = GameState;
                    GameMode->ServerBotManager->CachedAIPopulationTracker = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AIPopulationTracker;
                    if (!GameMode->SpawningPolicyManager)
                    {
                        GameMode->SpawningPolicyManager = Utils::SpawnActor22<AFortAthenaSpawningPolicyManager>({}, {});
                    }
                    GameMode->SpawningPolicyManager->GameModeAthena = GameMode;
                    GameMode->SpawningPolicyManager->GameStateAthena = GameState;
                    GameMode->AISettings = GameState->CurrentPlaylistInfo.BasePlaylist->AISettings;
                    AAthenaAIDirector* Director = Utils::SpawnActor22<AAthenaAIDirector>({});
                    GameMode->AIDirector = Director;
                    Director->Activate();
                    AFortAIGoalManager* GoalManager = Utils::SpawnActor<AFortAIGoalManager>({});
                    GameMode->AIGoalManager = GoalManager;
                    *reinterpret_cast<bool*>(__int64(GameMode->ServerBotManager) + 0x420) = 1;
                    UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingFoundation::StaticClass(), (TArray<AActor*>*) & GameMode->ServerBotManager->CachedBuildingFoundations);
                    GameMode->ServerBotManager->CachedAIPopulationTracker = ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AIPopulationTracker;
                    ((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->PlayerBotManager = GameMode->ServerBotManager;
                    UAISystem::GetDefaultObj()->AILoggingVerbose();
                    if (Globals::bEnablephoebeDebug)
                    {
                        GameMode->ServerBotManager->DebugMinimapData.DebugMinimapIconBrush.ImageSize = FVector2D(36, 36);
                        GameMode->ServerBotManager->DebugMinimapData.DebugMinimapIconBrush.ResourceObject = Utils::StaticLoadObject<UTexture2D>("/Game/Athena/HUD/Spectator/DeimosRiftMapIcon.DeimosRiftMapIcon");
                        GameMode->ServerBotManager->DebugMinimapData.DebugMinimapIconScale = FVector2D(0.5, 0.5);
                        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"Phoebe.Minimap 1", nullptr);
                    }
                }
            }
            if (Globals::bEnablephoebe)
            {
                CIDs = Utils::GetAllObjectsOfClass<UAthenaCharacterItemDefinition>();
                Pickaxes = Utils::GetAllObjectsOfClass<UAthenaPickaxeItemDefinition>();
                Backpacks = Utils::GetAllObjectsOfClass<UAthenaBackpackItemDefinition>();
                Gliders = Utils::GetAllObjectsOfClass<UAthenaGliderItemDefinition>();
                Contrails = Utils::GetAllObjectsOfClass<UAthenaSkyDiveContrailItemDefinition>();
                Dances = Utils::GetAllObjectsOfClass<UAthenaDanceItemDefinition>();
            }
            for (auto& Level : Playlist->AdditionalLevels)
            {
                bool Success = false;
                ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(World, Level, FVector(), FRotator(), &Success, FString());
                FAdditionalLevelStreamed level{};
                level.bIsServerOnly = false;
                level.LevelName = Level.ObjectID.AssetPathName;
                if (Success) GameState->AdditionalPlaylistLevelsStreamed.Add(level);
            }
            for (auto& Level : Playlist->AdditionalLevelsServerOnly)
            {
                bool Success = false;
                ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(World, Level, FVector(), FRotator(), &Success, FString());
                FAdditionalLevelStreamed level{};
                level.bIsServerOnly = true;
                level.LevelName = Level.ObjectID.AssetPathName;
                if (Success) GameState->AdditionalPlaylistLevelsStreamed.Add(level);
            }
            GameState->OnRep_AdditionalPlaylistLevelsStreamed();
            GameState->OnFinishedStreamingAdditionalPlaylistLevel();
            GameMode->HandleAllPlaylistLevelsVisible();
        }
        if (Globals::bEnableBackendMode)
        {
            static bool bNebulaSessionStarted = false;
            if (!bNebulaSessionStarted)
            {
                std::string playlistName = "playlist_defaultsolo";
                if (Globals::bEventEnabled)
                    playlistName = "playlist_showdownalt_solo";
                else if (Globals::bCreative)
                    playlistName = "playlist_playground";
                else if (Globals::bEnableBattleLab)
                    playlistName = "playlist_battlelab";
                else if (Globals::IsTournamentModeEnabled())
                    playlistName = "playlist_showdownalt_duos";
                else if (Globals::IsArenaModeEnabled())
                    playlistName = "playlist_showdownalt_solo";
                std::string host = Globals::NebulaServerIp;
                const int ListenPort = GetNebulaListenPort();
                // STATUS: starting — servidor subindo, antes de abrir a porta
                NebulaSessions::SetStatusStarting();
                NebulaSessions::StartSession(host, ListenPort, playlistName);
                // SetStatusWarmup() será chamado após NetDriver::Listen (abaixo)
                // SetStatusInMatch() será chamado em StartAircraftPhase quando o ônibus começar
                // SetStatusEnded() será chamado em PlayerController quando alguém vencer

                Sleep(5000);
                bNebulaSessionStarted = true;
            }
        }
        if (!GameMode->bWorldIsReady) {
            CrashGuard_Install();
            CrashGuard_HookProcessEvent();
            FURL URL;
            URL.Port = GetNebulaListenPort();
            NetDriver::Listen(UWorld::GetWorld(), URL);
            GameMode->bWorldIsReady = true;
            // STATUS: warmup — porta aberta, players podem conectar
            NebulaSessions::SetStatusWarmup();
            std::string title =
                "Nebula 14.60 - Listening on port " +
                std::to_string(URL.Port) +
                " - Joinable = true | Compiled at: " +
                std::string(__TIME__);
            SetConsoleTitleA(title.c_str());
        }
        return GameMode->AlivePlayers.Num() >= GameMode->WarmupRequiredPlayerCount;
    }
    // CRASH FIX: missing return when bInitialized == true caused UB (garbage in RAX).
    // Every tick after the first call must still return a valid bool.
    return GameMode->AlivePlayers.Num() >= GameMode->WarmupRequiredPlayerCount;
}

APawn* FortGameModeAthena::SpawnDefaultPawnFor(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot)
{
    APawn* _r{};
    __try { _r = SpawnDefaultPawnFor_Impl(GameMode, NewPlayer, StartSpot); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

static APawn* SpawnDefaultPawnFor_Impl(AGameModeBase* GameMode, AController* NewPlayer, AActor* StartSpot)
{
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    if (!GameState || !NewPlayer || !StartSpot)
        return nullptr;

    AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;
    if (!PlayerState)
        return nullptr;

    if (!NewPlayer->IsA(AFortPlayerControllerAthena::StaticClass()))
        return nullptr;

    auto PlayerController = (AFortPlayerControllerAthena*)NewPlayer;
    auto GM = (AFortGameModeAthena*)GameMode;

    if (PlayerState->AbilitySystemComponent)
        Abilities::GiveDefaultAbilitySet(PlayerState->AbilitySystemComponent);
    // else
        // printf("PlayerState->AbilitySystemComponent is null!\n");

    if (!PlayerController->bHasInitializedWorldInventory)
    {
        for (auto& StartingItem : GM->StartingItems)
            Inventory::GiveItem(PlayerController, StartingItem.Item, StartingItem.Count, 0, false);
        if (PlayerController->CosmeticLoadoutPC.Pickaxe && PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition)
            Inventory::GiveItem(PlayerController, PlayerController->CosmeticLoadoutPC.Pickaxe->WeaponDefinition, 1, 0, false);
        PlayerController->bHasInitializedWorldInventory = true;
    }

    if (PlayerController->XPComponent)
    {
        PlayerController->XPComponent->bRegisteredWithQuestManager = true;
        PlayerController->XPComponent->OnRep_bRegisteredWithQuestManager();
        PlayerState->SeasonLevelUIDisplay = PlayerController->XPComponent->CurrentLevel;
        PlayerState->OnRep_SeasonLevelUIDisplay();
    }

    auto QuestManager = PlayerController->GetQuestManager(ESubGame::Athena);
    if (!QuestManager)
        return nullptr;

    APawn* SpawnedPawn = nullptr;
    if (GameState->GamePhase >= EAthenaGamePhase::Warmup)
    {
        GameMode->ChoosePlayerStart(PlayerController);
        SpawnedPawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, StartSpot->GetTransform());
    }
    else
    {
        if (!PlayerController->WarmupPlayerStart)
            SpawnedPawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, StartSpot->GetTransform());
        else
            SpawnedPawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, PlayerController->WarmupPlayerStart->GetTransform());
    }

    if (SpawnedPawn)
    {
        QuestManager->InitializeQuestAbilities(SpawnedPawn);
    }
    else
    {
        printf("[SpawnDefaultPawnFor] Aviso: SpawnDefaultPawnAtTransform retornou null para controller %p\n",
            (void*)PlayerController);
    }

    if (Globals::bCreative && SpawnedPawn)
    {
        auto FortPawn = (AFortPlayerPawnAthena*)SpawnedPawn;
        FortPawn->SetMaxHealth(100.f);
        FortPawn->SetHealth(100.f);
        FortPawn->SetMaxShield(100.f);
        FortPawn->SetShield(100.f);
    }

    return SpawnedPawn;
}

void FortGameModeAthena::StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int a2)
{
    __try { StartNewSafeZonePhase_Impl(GameMode, a2); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void StartNewSafeZonePhase_Impl(AFortGameModeAthena* GameMode, int a2)
{
    if (Globals::bCreative)
        return;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    if (!GameState || !GameState->MapInfo)
    {
        SafeCallStartNewSafeZonePhaseOG(GameMode, a2);
        return;
    }
    FFortSafeZoneDefinition* SafeZoneDefinition = &GameState->MapInfo->SafeZoneDefinition;
    TArray<float>& Durations = *(TArray<float>*)(__int64(SafeZoneDefinition) + 0x1F8);
    TArray<float>& HoldDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + 0x1E8);
    constexpr static std::array<float, 8> LateGameDurations{
        0.f, 65.f, 60.f, 50.f, 45.f, 35.f, 30.f, 40.f,
    };
    constexpr static std::array<float, 8> LateGameHoldDurations{
        0.f, 60.f, 55.f, 50.f, 45.f, 30.f, 0.f, 0.f,
    };
    float Duration = 0.f;
    float HoldDuration = 0.f;
    if (Globals::bLateGame)
    {
        int Idx = GameMode->SafeZonePhase - 2;
        if (Idx < 0)
            Idx = 0;
        if (Idx >= (int)LateGameDurations.size())
            Idx = (int)LateGameDurations.size() - 1;
        Duration = LateGameDurations[Idx];
        HoldDuration = LateGameHoldDurations[Idx];
        if (Duration <= 0.f)
            Duration = 1.f;
    }
    else
    {
        int32 PhaseIdx = GameMode->SafeZonePhase + 1;
        if (PhaseIdx < 0 || PhaseIdx >= Durations.Num())
        {
            // printf("[SafeZone] SafeZonePhase %d fora do range (%d entradas), usando fallback.\n",
                // GameMode->SafeZonePhase, Durations.Num());
            SafeCallStartNewSafeZonePhaseOG(GameMode, a2);
            return;
        }
        Duration = Durations[PhaseIdx];
        HoldDuration = (PhaseIdx < HoldDurations.Num()) ? HoldDurations[PhaseIdx] : 0.f;
    }
    if (!GameMode->SafeZoneIndicator && GameMode->SafeZoneIndicatorClass.Get())
    {
        GameMode->SafeZoneIndicator = Utils::SpawnActor<AFortSafeZoneIndicator>(
            FVector(0.f, 0.f, 0.f),
            FRotator(),
            GameMode->SafeZoneIndicatorClass.Get()
        );
        // printf("[SafeZone] SafeZoneIndicator criado antes do OG (fase %d)\n", GameMode->SafeZonePhase);
    }
    SafeCallStartNewSafeZonePhaseOG(GameMode, a2);
    if (!GameMode->SafeZoneIndicator)
        return;
    float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
    GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = Now + HoldDuration;
    GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = Now + HoldDuration + Duration;
}

bool FortGameModeAthena::StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
{
    bool _r{};
    __try { _r = StartAircraftPhase_Impl(GameMode, a2); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

static bool StartAircraftPhase_Impl(AFortGameModeAthena* GameMode, char a2)
{
    if (Globals::bCreative) {
        return false;
    }
    const bool bFirstCall = !s_bAircraftPhaseInitialized;
    s_bAircraftPhaseInitialized = true;
    if (!bFirstCall)
    {
        // printf("[StartAircraftPhase] 2a chamada bloqueada para prevenir crash 0x30 no FlushAsyncLoading.\n");
        return false;
    }
    NebulaSessions::SetStatusInMatch();

    auto Ret = FortGameModeAthena::StartAircraftPhaseOG(GameMode, a2);
    if (bFirstCall && Globals::bLateGame)
    {
        auto GameState = (AFortGameStateAthena*)GameMode->GameState;
        if (!GameState || GameState->Aircrafts.Num() == 0 || !GameState->Aircrafts[0])
            return Ret;
        auto Aircraft = GameState->Aircrafts[0];
        Aircraft->FlightInfo.FlightSpeed = 0.f;
        FVector Loc{ 0.f, 0.f, 0.f };
        if (GameMode->SafeZoneLocations.Num() > 3)
            Loc = GameMode->SafeZoneLocations.At(3);
        if (Loc.X == 0.f && Loc.Y == 0.f && Loc.Z == 0.f)
            Loc = FVector{ 46678.f, 31872.f, 0.f };
        Loc.Z = 17500.f;
        s_LateGameSafeZoneCenter = Loc;
        s_LateGameSafeZoneCenterSet = true;
        Aircraft->FlightInfo.FlightStartLocation = Loc.Quantize100();
        Aircraft->FlightInfo.TimeTillFlightEnd = 7.f;
        Aircraft->FlightInfo.TimeTillDropEnd = 0.f;
        Aircraft->FlightInfo.TimeTillDropStart = 0.f;
        float Now = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        Aircraft->FlightStartTime = Now;
        Aircraft->FlightEndTime = Now + 7.f;
        GameState->SafeZonesStartTime = Now + 7.f;
    }
    return Ret;
}

__int64 (*OnAircraftEnteredDropZoneOG)(AFortGameModeAthena*);
__int64 OnAircraftEnteredDropZone(AFortGameModeAthena* a1)
{
    if (Globals::bCreative) return false;
    auto GS_Enter = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    if (GS_Enter)
        GS_Enter->GamePhase = EAthenaGamePhase::Aircraft;
    if (Globals::bEnablephoebe)
    {
        for (size_t i = 0; i < SpawnedBots.size(); i++)
        {
            if (SpawnedBots[i])
                SpawnedBots[i]->State = EBotState::InBus;
        }
    }
    return OnAircraftEnteredDropZoneOG(a1);
}

void FortGameModeAthena::OnAircraftExitedDropZone(AFortGameModeAthena* GM, AFortAthenaAircraft* Aircraft)
{
    auto GameState = (AFortGameStateAthena*)GM->GameState;
    if (!GameState)
    {
        OnAircraftExitedDropZoneOG(GM, Aircraft);
        return;
    }
    int32 Total = GM->AlivePlayers.Num();
    int32 Idx = 0;
    const bool bShouldSpread = Globals::bLateGame && s_LateGameSafeZoneCenterSet && Total > 1;
    const float SpreadRadius = 4000.f;

    for (auto& Player : GM->AlivePlayers)
    {
        if (!Player) { Idx++; continue; }
        __try
        {
            auto* PC = (AFortPlayerControllerAthena*)Player;
            if (!PC->PlayerState || !PC->IsInAircraft()) { Idx++; continue; }

            auto* AircraftComp = PC->GetAircraftComponent();
            if (AircraftComp)
            {
                FRotator JumpRot{};
                if (bShouldSpread && Total > 1)
                    JumpRot.Yaw = (360.f / (float)Total) * (float)Idx;
                AircraftComp->ServerAttemptAircraftJump(JumpRot);
            }
            if (bShouldSpread && PC->MyFortPawn)
            {
                float AngleRad = (2.f * 3.14159265f / (float)Total) * (float)Idx;
                FVector TargetLoc = s_LateGameSafeZoneCenter;
                TargetLoc.X += SpreadRadius * cosf(AngleRad);
                TargetLoc.Y += SpreadRadius * sinf(AngleRad);
                PC->MyFortPawn->K2_TeleportTo(TargetLoc, FRotator{});
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        Idx++;
    }
    GameState->GamePhase = EAthenaGamePhase::SafeZones;
    GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);
    if (Globals::bEnablephoebe)
    {
        for (size_t i = 0; i < SpawnedBots.size(); i++)
        {
            if (SpawnedBots[i])
                SpawnedBots[i]->State = EBotState::Landed;
        }
    }
    if (!GM->GameState)
        return;
    OnAircraftExitedDropZoneOG(GM, Aircraft);
}

EFortTeam PickTeam(AFortGameModeAthena* GameModeAthena, EFortTeam PreferredTeam, AFortPlayerControllerAthena* PlayerControllerAthena)
{
    EFortTeam _r{};
    __try { _r = PickTeam_Impl(GameModeAthena, PreferredTeam, PlayerControllerAthena); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

EFortTeam PickTeam_Impl(AFortGameModeAthena* GameModeAthena, EFortTeam PreferredTeam, AFortPlayerControllerAthena* PlayerControllerAthena)
{
    if (!GameModeAthena)
    {
        //TeamSyncLog("pick team aborted: game mode null");
        return EFortTeam::MAX;
    }
    AFortGameStateAthena* GameStateAthena = Utils::Cast<AFortGameStateAthena>(GameModeAthena->GameState);
    if (!GameStateAthena)
    {
        //TeamSyncLog("pick team aborted: game state null");
        return EFortTeam::MAX;
    }
    UFortPlaylistAthena* PlaylistAthena = GameStateAthena->CurrentPlaylistInfo.BasePlaylist;
    if (!PlaylistAthena)
    {
        //  TeamSyncLog("pick team aborted: playlist null");
        return EFortTeam::MAX;
    }
    std::string Username;
    if (PlayerControllerAthena && PlayerControllerAthena->PlayerState)
        Username = TeamSyncNormalizeUsername(PlayerControllerAthena->PlayerState->GetPlayerName().ToString());
    //  TeamSyncLog(std::string("pick team start username=") + (Username.empty() ? "<empty>" : Username) + " maxTeamSize=" + std::to_string(PlaylistAthena->MaxTeamSize) + " maxTeamCount=" + std::to_string(PlaylistAthena->MaxTeamCount));
    if (!TeamSyncIsTeamPlaylist(PlaylistAthena) || Username.empty())
    {
        TeamSyncLog(std::string("fallback normal team selection reason=") + (!TeamSyncIsTeamPlaylist(PlaylistAthena) ? "not_team_playlist" : "empty_username"));
        EFortTeam FallbackTeam = TeamSyncChooseAvailableTeam(GameStateAthena, PlaylistAthena, 1);
        TeamSyncLog(std::string("fallback selected team=") + std::to_string(static_cast<int>(FallbackTeam)));
        return FallbackTeam;
    }
    std::vector<std::string> PartyMembers = TeamSyncFetchPartyMembers(Username);
    if (PartyMembers.empty())
        PartyMembers.push_back(Username);
    std::string GroupKey = TeamSyncBuildGroupKey(PartyMembers);
    //  TeamSyncLog(std::string("party members resolved for ") + Username + ": [" + TeamSyncJoinUsernames(PartyMembers) + "] groupKey=" + GroupKey);
    int ReservedTeamIndex = TeamSyncFindReservedGroupTeam(GroupKey, Username);
    if (ReservedTeamIndex > 0)
        return EFortTeam(ReservedTeamIndex);
    int ExistingTeamIndex = TeamSyncFindExistingGroupTeam(GameStateAthena, PartyMembers);
    if (ExistingTeamIndex > 0)
    {
        TeamSyncReserveGroupTeam(GroupKey, ExistingTeamIndex, std::min((int)PartyMembers.size(), (int)PlaylistAthena->MaxTeamSize), Username);
        // TeamSyncLog(std::string("using existing in-match team ") + std::to_string(ExistingTeamIndex));
        return EFortTeam(ExistingTeamIndex);
    }
    int RequiredOpenSlots = std::min((int)PartyMembers.size(), (int)PlaylistAthena->MaxTeamSize);
    if (RequiredOpenSlots <= 0)
        RequiredOpenSlots = 1;
    EFortTeam ChosenTeam = TeamSyncChooseAvailableTeam(GameStateAthena, PlaylistAthena, RequiredOpenSlots);
    if (ChosenTeam == EFortTeam::MAX)
        ChosenTeam = TeamSyncChooseAvailableTeam(GameStateAthena, PlaylistAthena, 1);
    if (ChosenTeam != EFortTeam::MAX)
        TeamSyncReserveGroupTeam(GroupKey, static_cast<int>(ChosenTeam), RequiredOpenSlots, Username);
    //TeamSyncLog(std::string("final chosen team for ") + Username + " = " + std::to_string(static_cast<int>(ChosenTeam)) + " requiredOpenSlots=" + std::to_string(RequiredOpenSlots));
    return ChosenTeam;
}

void FortGameModeAthena::HandleStartingNewPlayerHook(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer)
{
    __try { HandleStartingNewPlayerHook_Impl(GameMode, NewPlayer); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void HandleStartingNewPlayerHook_Impl(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer)
{
    auto GameModeAuto = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    auto PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;
    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    if (!PlayerState || !GameState || !GameModeAuto)
        return;
    if (GameState->GamePhase < EAthenaGamePhase::Warmup)
        return;
    {
        int32 ComputedSquadId = (int32)PlayerState->TeamIndex - 3;
        PlayerState->SquadId = (uint8)(ComputedSquadId >= 0 ? ComputedSquadId : 0);
    }
    PlayerState->OnRep_SquadId();
    FGameMemberInfo Member;
    Member.MostRecentArrayReplicationKey = -1;
    Member.ReplicationID = -1;
    Member.ReplicationKey = -1;
    Member.TeamIndex = PlayerState->TeamIndex;
    Member.SquadId = PlayerState->SquadId;
    Member.MemberUniqueId = PlayerState->UniqueId;
    GameState->GameMemberInfoArray.Members.Add(Member);
    GameState->GameMemberInfoArray.MarkItemDirty(Member);
    auto NotifyGameMemberAdded = (void(*)(AFortGameStateAthena*, uint8, uint8, FUniqueNetIdRepl*))(InSDKUtils::GetImageBase() + 0x20AA8F0);
    NotifyGameMemberAdded(GameState, Member.SquadId, Member.TeamIndex, &Member.MemberUniqueId);
    if (!NewPlayer->MatchReport)
        NewPlayer->MatchReport = reinterpret_cast<UAthenaPlayerMatchReport*>(UGameplayStatics::SpawnObject(UAthenaPlayerMatchReport::StaticClass(), NewPlayer));
    if (NewPlayer->MatchReport)
    {
        NewPlayer->MatchReport->bHasMatchStats = true;
        NewPlayer->MatchReport->bHasRewards = true;
        NewPlayer->MatchReport->bHasTeamStats = true;
    }
    if (!Globals::bEnableBackendMode)
    {
        if (!NewPlayer->CheatManager)
        {
            UCheatManager* CheatManager = Utils::Cast<UCheatManager>(UGameplayStatics::SpawnObject(UCheatManager::StaticClass(), NewPlayer));
            if (CheatManager)
            {
                NewPlayer->CheatManager = CheatManager;
                NewPlayer->CheatManager->ReceiveInitCheatManager();
            }
        }
    }

    if (Globals::bCreative && !PlayerState->bIsSpectator)
    {
        if (!GameState->CreativePortalManager)
        {
            NewPlayer->bBuildFree = true;
            NewPlayer->CreativePlotLinkedVolume = nullptr;
            NewPlayer->OnRep_CreativePlotLinkedVolume();
            NewPlayer->bHasServerFinishedLoading = true;
            NewPlayer->OnRep_bHasServerFinishedLoading();
            return;
        }
        AFortAthenaCreativePortal* Portal = nullptr;
        for (int i = 0; i < GameState->CreativePortalManager->AllPortals.Num(); i++)
        {
            auto CurrentPortal = GameState->CreativePortalManager->AllPortals[i];
            if (!CurrentPortal) continue;
            if (!CurrentPortal->GetLinkedVolume()) continue;
            if (CurrentPortal->GetLinkedVolume()->VolumeState == EVolumeState::Ready) continue;
            Portal = CurrentPortal;
            break;
        }
        if (!Portal)
        {
            NewPlayer->CreativePlotLinkedVolume = nullptr;
            NewPlayer->OnRep_CreativePlotLinkedVolume();
            NewPlayer->bBuildFree = true;
            NewPlayer->bHasServerFinishedLoading = true;
            NewPlayer->OnRep_bHasServerFinishedLoading();
            return;
        }
        Portal->OwningPlayer = PlayerState->UniqueId;
        Portal->OnRep_OwningPlayer();
        if (!Portal->bPortalOpen) {
            Portal->bPortalOpen = true;
            Portal->OnRep_PortalOpen();
        }
        Portal->PlayersReady.Add(PlayerState->UniqueId);
        Portal->OnRep_PlayersReady();
        Portal->bIsPublishedPortal = false;
        Portal->OnRep_PublishedPortal();
        Portal->bUserInitiatedLoad = true;
        Portal->bInErrorState = false;
        Portal->IslandInfo.AltTitle = UKismetTextLibrary::Conv_StringToText(L"");
        Portal->IslandInfo.CreatorName = PlayerState->GetPlayerName();
        Portal->IslandInfo.Version = 1;
        Portal->IslandInfo.SupportCode = L"Andreu";
        Portal->IslandInfo.Mnemonic = L"";
        Portal->IslandInfo.ImageUrl = L"";
        Portal->OnRep_IslandInfo();
        Creative::LoadIslands(NewPlayer, PlayerState->GetPlayerName());
        NewPlayer->OwnedPortal = Portal;
        NewPlayer->CreativePlotLinkedVolume = Portal->LinkedVolume;
        NewPlayer->OnRep_CreativePlotLinkedVolume();
        auto LevelStreamComponent = Portal->GetLinkedVolume()->GetComponentByClass(UPlaysetLevelStreamComponent::StaticClass())->Cast<UPlaysetLevelStreamComponent>();
        auto LevelSaveComponent = Portal->GetLinkedVolume()->GetComponentByClass(UFortLevelSaveComponent::StaticClass())->Cast<UFortLevelSaveComponent>();
        auto Playset = Utils::StaticLoadObject<UFortPlaysetItemDefinition>("/Game/Playsets/PID_Playset_60x60_Composed.PID_Playset_60x60_Composed");
        if (!LevelStreamComponent || !LevelSaveComponent || !Playset)
        {
            NewPlayer->bBuildFree = true;
            NewPlayer->bHasServerFinishedLoading = true;
            NewPlayer->OnRep_bHasServerFinishedLoading();
            return;
        }
        if (NewPlayer->CreativePlotLinkedVolume)
        {
            LevelSaveComponent->AccountIdOfOwner = PlayerState->UniqueId;
            LevelSaveComponent->PlotPermissions.Permission = EFortCreativePlotPermission::Public;
            LevelSaveComponent->LoadedLinkData = Portal->IslandInfo;
            LevelSaveComponent->bIsLoaded = true;
            LevelSaveComponent->bAutoLoadFromRestrictedPlotDefinition = true;
            if (LevelSaveComponent->LoadedPlot)
                LevelSaveComponent->LoadedPlot->IslandTitle = PlayerState->GetPlayerName();
            LevelSaveComponent->OnRep_LoadedPlotInstanceId();
            LevelSaveComponent->OnRep_LoadedLinkData();
        }
        Portal->LinkedVolume->CurrentPlayset = Playset;
        Portal->LinkedVolume->OnRep_CurrentPlayset();
        LevelStreamComponent->bAutoLoadLevel = true;
        Utils::SpawnActorV3<AFortMinigameSettingsBuilding>(
            Portal->GetLinkedVolume()->K2_GetActorLocation(),
            FRotator(),
            Utils::StaticLoadObject<UClass>("/Game/Athena/Items/Gameplay/MinigameSettingsControl/MinigameSettingsMachine.MinigameSettingsMachine_C"),
            Portal->LinkedVolume
        );
        Creative::ShowPlayset(Playset, Portal->GetLinkedVolume());
        Portal->LinkedVolume->VolumeState = EVolumeState::Ready;
        Portal->LinkedVolume->OnRep_VolumeState();
        NewPlayer->bBuildFree = true;
        NewPlayer->bHasServerFinishedLoading = true;
        NewPlayer->OnRep_bHasServerFinishedLoading();
        std::thread([NewPlayer]()
            {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                if (NewPlayer && NewPlayer->GetPlayerPawn())
                {
                    auto Pawn = (AFortPlayerPawnAthena*)NewPlayer->GetPlayerPawn();
                    Pawn->SetMaxHealth(100.f);
                    Pawn->SetHealth(100.f);
                    Pawn->SetMaxShield(100.f);
                    Pawn->SetShield(100.f);
                }
            }).detach();
    }
}

void (*StormOG)(__int64, int);
void __fastcall Storm(__int64 a1, int a2)
{
    __try { Storm_Impl(a1, a2); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void __fastcall Storm_Impl(__int64 a1, int a2)
{
    if (!StormOG)
        return;
    if (Globals::bCreative || !Globals::bEnablephoebe)
    {
        StormOG(a1, a2);
        return;
    }
    StormOG(a1, a2);
    auto GS_Storm = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    if (GS_Storm && GS_Storm->GamePhase > EAthenaGamePhase::Aircraft)
    {
        for (auto& bot : SpawnedBots)
        {
            if (bot && bot->PC && bot->TickEnabled)
            {
                bot->OnSafeZoneStateChange();
                break;
            }
        }
    }
}

void FortGameModeAthena::InitFortGameModeAthena()
{
    Utils::SwapVFTs(AFortGameModeAthena::StaticClass()->DefaultObject, 0x102, ReadyToStartMatch, nullptr);
    Utils::SwapVFTs(AFortGameModeAthena::StaticClass()->DefaultObject, 0xc9, SpawnDefaultPawnFor, (LPVOID*)&SpawnDefaultPawnForOG);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2080090), StartNewSafeZonePhase, (PVOID*)&StartNewSafeZonePhaseOG);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x207c610), StartAircraftPhase, (PVOID*)&StartAircraftPhaseOG);
    MH_CreateHook((LPVOID)(ImageBase + 0x2060410), OnAircraftExitedDropZone, (LPVOID*)&OnAircraftExitedDropZoneOG);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2066c40), PickTeam, nullptr);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2640ad0), HandleStartingNewPlayerHook, (LPVOID*)&HandleStartingNewPlayer);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2060370), OnAircraftEnteredDropZone, (LPVOID*)&OnAircraftEnteredDropZoneOG);
    if (!Globals::bCreative)
    {
        MH_CreateHook((LPVOID)(ImageBase + 0x207FDB0), Storm, (LPVOID*)&StormOG);
    }
}