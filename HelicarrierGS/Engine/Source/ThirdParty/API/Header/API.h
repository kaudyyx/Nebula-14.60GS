#pragma once
#include "pch.h"
#include <string>
#include <unordered_set>

// ─── Bases e auths ────────────────────────────────────────────────────────────
static inline const std::string NEBULA_BASE = "http://187.77.254.242:5555";
static inline const std::string NEBULA_AUTH_RAW = "?n3b7x!ayz";   // arena
static inline const std::string NEBULA_AUTH_XP = "!n6eb7u?2la";  // xp

// ─── Pontos por divisão (fonte: screenshots do jogo) ─────────────────────────
//
//  Div | Liga            | Kill | Bus Fare
//  ----|-----------------|------|----------
//   1  | Open I          |  20  |    0
//   2  | Open II         |  20  |    0
//   3  | Open III        |  20  |    0
//   4  | Open IV         |  20  |  -10
//   5  | Contender I     |  20  |  -20
//   6  | Contender II    |  20  |  -30
//   7  | Contender III   |  20  |  -40
//   8  | Champion I      |  20  |  -60
//   9  | Champion II     |  20  |  -60
//  10  | Champion        |  20  |  -70
//
//  Kill points = 20 em TODAS as divisões.
//  Bus fare = 0 nas divisões 1-3, negativo nas 4-10.
// ─────────────────────────────────────────────────────────────────────────────

// Kill points: fixo 20 em todas as divisões
static inline int32_t GetArenaKillPoints() { return 20; }

// Bus fare por divisão (valor negativo = dedução)
static inline int32_t GetArenaBusFare(int division)
{
    switch (division)
    {
    case 1: case 2: case 3: return   0;
    case 4:                 return -10;
    case 5:                 return -20;
    case 6:                 return -30;
    case 7:                 return -40;
    case 8: case 9:         return -60;
    case 10: default:       return -70;
    }
}

// ─── XP por ação ─────────────────────────────────────────────────────────────
static inline int32_t kXPPerKill = 130;   // XP por kill
static inline int32_t kXPBusFare = 0;     // XP ao entrar no ônibus (0 = desativado)

// ─── Cache de divisão por player ─────────────────────────────────────────────
// Populado assincronamente em ServerLoadingScreenDropped via /nebula/getBalanceById
namespace NebulaPlayerData
{
    // Busca a divisão do player no backend (async, não bloqueia game thread)
    void FetchDivisionAsync(const std::string& username);

    // Retorna divisão cacheada. Retorna 1 (mais seguro, sem bus fare) se ainda
    // não chegou a resposta do backend.
    int GetPlayerDivision(const std::string& username);

    // Limpa o cache (chamar no início de cada partida)
    void ClearCache();
}

static inline std::unordered_set<std::string> gBusfareDone{};

inline size_t write_callback(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

// ─── NebulaSessions ──────────────────────────────────────────────────────────
// API.cpp: NebulaSessions ORIGINAL - não modificar.
// Apenas as funções originais são implementadas em API.cpp.
// SetStatusStarting/Warmup/Ended são inline aliases aqui no header.
namespace NebulaSessions
{
    // Funções originais (implementadas em API.cpp)
    bool StartSession(const std::string& host, int port, const std::string& playlistName);
    void StopSession();
    void SetStatusLobby();
    void SetStatusInMatch();
    void SetStatusRebooting();
    void SetPlayerCount(int count);

    // Aliases inline — não geram corpo em API.cpp, sem conflito
    inline void SetStatusStarting() { SetStatusRebooting(); }
    inline void SetStatusWarmup() { SetStatusLobby(); }
    inline void SetStatusEnded() { SetStatusInMatch(); }
    inline void IncrementJoinedPlayers() {}
}

// ─── NebulaMatchStats ─────────────────────────────────────────────────────────
// /nebula/applyMatchStats?auth=?n3b7x!ayz&username=X&reason=changehype&amount=N
namespace NebulaMatchStats
{
    bool ApplyMatchStats(const std::string& username, int32_t amount);
}

// ─── NebulaXP ─────────────────────────────────────────────────────────────────
// /addxpamount?auth=!n6eb7u?2la&username=X&amount=N
namespace NebulaXP
{
    bool AddXP(const std::string& username, int32_t xpAmount);
}

// ─── NebulaTournament ─────────────────────────────────────────────────────────
namespace NebulaTournament
{
    void StartMatchAsync(const std::string& username,
        const std::string& sessionId = std::string());
    void SyncKillsAsync(const std::string& username, int currentKills,
        const std::string& sessionId = std::string(), int timeAlive = 0);
    void FinishMatchAsync(const std::string& username, int placement, int currentKills,
        const std::string& sessionId = std::string(),
        bool victory = false, int timeAlive = 0);
    void ReportWinAsync(const std::string& username, int currentKills,
        const std::string& sessionId = std::string(), int timeAlive = 0);
    void ChangePointsAsync(const std::string& username, int32_t amount);
}

// ─── URL encode helper ────────────────────────────────────────────────────────
static inline std::string NebulaUrlEncode(const std::string& s)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~')
            out.push_back((char)c);
        else
        {
            out.push_back('%');
            out.push_back(hex[(c >> 4) & 0xF]);
            out.push_back(hex[c & 0xF]);
        }
    }
    return out;
}

// ─── Função antiga mantida para compatibilidade (NÃO USAR para arena/xp) ─────
// Esta função usa a API antiga sem amount. Está aqui apenas para não quebrar
// chamadas existentes que ainda a referenciem.
// ANTI-CRASH: getResponseAsync legado — NÃO chama curl_global_init/cleanup em threads
// (não é thread-safe). Essa função não é mais usada; mantida apenas por compatibilidade.
// curl_global_init DEVE ser chamado uma vez na main thread antes de qualquer curl_easy_init.
// Use NebulaApiInternal::HttpRequest (WinHTTP) para novas chamadas.
inline void getResponseAsync(const std::string& url)
{
    std::thread([url = url]() {
        try
        {
            // Não chama curl_global_init/cleanup — não é thread-safe.
            CURL* curl = curl_easy_init();
            if (!curl) return;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            std::string response_body;
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        catch (...) {}
        }).detach();
}

// ─── DEPRECATED: NebulaApplyMatchStats (API antiga sem amount) ───────────────
// Mantida para não quebrar referências existentes no código.
// Internamente chama a nova API com amount=0 (ignorado pelo servidor se zero).
static inline void NebulaApplyMatchStats(const std::string& username, const std::string& reason)
{
    // Chamada legada — usar NebulaMatchStats::ApplyMatchStats(username, amount) em vez disso.
    (void)reason;
}

// ─── Helper de reason de kill (mantido para compatibilidade) ─────────────────
static inline std::string NebulaReasonKill()
{
    return Globals::IsArenaModeEnabled() ? "arenakill" : "kill";
}

// ─── Placement table (valores exatos desta Nebula) ───────────────────────────
// Fonte: screenshots do jogo. Iguais para todas as divisões.
struct TournamentPlacement { int placementThreshold; int pointsScored; };
const TournamentPlacement placements[] = {
    {1,  50}, // Vitória Royale
    {2,  25}, // Top 2
    {3,  10}, // Top 3
    {4,  10}, // Top 4
    {5,  20}, // Top 5
    {10, 10}, // Top 10
    {20, 15}, // Top 20
    {25, 15}  // Top 25
};