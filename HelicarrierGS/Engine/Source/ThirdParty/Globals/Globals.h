
#pragma once
#include <string_view>
#include <cctype>
#include <vector>
#include "pch.h"

class Globals
{
public:
	static inline std::string NebulaBaseUrl = "http://187.77.254.242:5555";    // backend URL (no trailing slash)
	static inline std::string NebulaAuthToken = "?n3b7x!ayz";               // matches SESSIONS_TOKEN on the backend
	static inline std::string NebulaInternalApiKey = "?n3b7x!ayz";           // matches API_KEY for internal routes
	static inline std::string NebulaApiKey = "!n6eb7u?2la";           // matches API_KEY for internal routes
	static inline std::string NebulaRegion = "BR";
	static inline std::string NebulaServerName = "PlayerBotsSolos";
	static inline std::string NebulaSessionId = "";                         // filled at runtime
	static inline std::string NebulaTournamentEventId = "Testing_duos_vbucks_cup";
	static inline std::string NebulaCustomCode = "none";
	static inline std::string NebulaServerIp = "181.214.221.36";           // server public IP
	static inline int NebulaPlayerCap = 100;
	static inline int NebulaHeartbeatSec = 5;                              // keep-alive interval (must be < backend TTL)
	static inline int NebulaListeningPort = 0;                              // filled at runtime
	// Auto shutdown after match win. Enable to have the server exit after someone wins.
	static inline std::string startingplayers = "2";
	static inline std::vector<UAthenaCharacterItemDefinition*> CharacterItemDefs;
	static inline std::vector<UAthenaBackpackItemDefinition*> BackpackItemDefs;
	static inline std::vector<UAthenaDanceItemDefinition*> EmoteItemDefs;
	constexpr static bool bLateGame = true;
	constexpr static bool bEnableBattleLab = false;
	constexpr static bool bArena = false;
	static inline bool bDynamicLateGameZones = false; // if true, the late game zones will shrink based on the number of alive players
	static inline int Uptime = 0; // dont change
	static inline bool bEnablephoebe = false;
	static inline bool bTournament = true;
	static inline bool bEnablephoebeDebug = false;
	static inline bool bCreative = false;
	static inline bool bEventEnabled = false;
	static inline bool bEnableBackendMode = true;

	static inline bool HasTournamentEventId()
	{
		std::string id = NebulaTournamentEventId;
		size_t start = 0;
		while (start < id.size() && std::isspace((unsigned char)id[start]))
			++start;
		size_t end = id.size();
		while (end > start && std::isspace((unsigned char)id[end - 1]))
			--end;
		id = id.substr(start, end - start);
		if (id.empty())
			return false;
		for (char& c : id)
			c = (char)std::tolower((unsigned char)c);
		return id != "none" && id != "false" && id != "0" && id != "disabled";
	}

	static inline bool IsTournamentModeEnabled()
	{
		return bTournament || HasTournamentEventId();
	}

	static inline bool IsArenaModeEnabled()
	{
		return bArena && !IsTournamentModeEnabled();
	}

};
//#define NO_CPR
