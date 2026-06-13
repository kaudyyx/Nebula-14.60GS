#include "pch.h"
#include "../Header/API.h"
#include <thread>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <Windows.h>
#include <winhttp.h>
#include <Engine/Source/ThirdParty/Globals/Globals.h>

namespace NebulaApiInternal
{
    static const char* kInternalApiKey = "?n3b7x!ayz";
    static const char* kMatchStatsBaseUrl = "http://187.77.254.242:5555";

    static void NebulaLog(const std::string& msg)
    {
        std::string line = "[Nebula] " + msg + "\n";
        OutputDebugStringA(line.c_str());
        std::fwrite(line.data(), 1, line.size(), stdout);
        std::fflush(stdout);
    }

    static std::string TrimString(const std::string& inp)
    {
        size_t b = 0;
        size_t e = inp.size();
        while (b < e && (inp[b] == ' ' || inp[b] == '\t' || inp[b] == '\r' || inp[b] == '\n'))
            ++b;
        while (e > b && (inp[e - 1] == ' ' || inp[e - 1] == '\t' || inp[e - 1] == '\r' || inp[e - 1] == '\n'))
            --e;
        return inp.substr(b, e - b);
    }

    static std::wstring Utf8ToWstring(const std::string& utf8)
    {
        if (utf8.empty())
            return std::wstring();

        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
        if (len <= 0)
            return std::wstring();

        std::wstring w;
        w.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &w[0], len);
        return w;
    }

    static std::string UrlEncode(const std::string& input)
    {
        std::ostringstream out;
        for (unsigned char c : input)
        {
            if ((c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~')
            {
                out << c;
            }
            else
            {
                out << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
                out << std::nouppercase << std::dec;
            }
        }
        return out.str();
    }

    static std::string EscapeJson(const std::string& input)
    {
        std::ostringstream out;
        for (unsigned char c : input)
        {
            if (c == '\\' || c == '"')
            {
                out << '\\' << c;
            }
            else if (c == '\b')
            {
                out << "\\b";
            }
            else if (c == '\f')
            {
                out << "\\f";
            }
            else if (c == '\n')
            {
                out << "\\n";
            }
            else if (c == '\r')
            {
                out << "\\r";
            }
            else if (c == '\t')
            {
                out << "\\t";
            }
            else if (c < 32)
            {
                out << "\\u00";
                out << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
                out << std::nouppercase << std::dec;
            }
            else
            {
                out << c;
            }
        }
        return out.str();
    }

    static std::string BuildBaseUrl()
    {
        std::string base = TrimString(Globals::NebulaBaseUrl);
        if (base.empty())
            base = "http://127.0.0.1:5555";
        if (base.rfind("http://", 0) != 0 && base.rfind("https://", 0) != 0)
            base = "http://" + base;
        while (!base.empty() && base.back() == '/')
            base.pop_back();
        return base;
    }

    static int ComputeBeaconPort(int gamePort)
    {
        const int baseGame = 7777;
        const int baseBeacon = 15009;
        if (gamePort <= 0)
            return baseBeacon;
        int delta = gamePort - baseGame;
        if (delta < 0)
            delta = 0;
        return baseBeacon + delta;
    }

    static bool HttpRequest(const std::string& fullUrl, const std::string& method, const std::string& reqBody, std::string& outBody, DWORD* pOutStatus = nullptr, const std::string& authToken = Globals::NebulaAuthToken)
    {
        outBody.clear();
        if (pOutStatus)
            *pOutStatus = 0;

        std::wstring wUrl = Utf8ToWstring(fullUrl);

        URL_COMPONENTSW uc{};
        wchar_t wScheme[16]{};
        wchar_t wHost[256]{};
        wchar_t wPath[2048]{};
        wchar_t wExtra[2048]{};

        uc.dwStructSize = sizeof(uc);
        uc.lpszScheme = wScheme;
        uc.dwSchemeLength = _countof(wScheme);
        uc.lpszHostName = wHost;
        uc.dwHostNameLength = _countof(wHost);
        uc.lpszUrlPath = wPath;
        uc.dwUrlPathLength = _countof(wPath);
        uc.lpszExtraInfo = wExtra;
        uc.dwExtraInfoLength = _countof(wExtra);

        if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &uc))
            return false;

        std::wstring hostW = uc.lpszHostName ? uc.lpszHostName : L"";
        INTERNET_PORT port = uc.nPort;
        std::wstring pathW = uc.lpszUrlPath ? uc.lpszUrlPath : L"";
        std::wstring extraW = uc.lpszExtraInfo ? uc.lpszExtraInfo : L"";
        pathW += extraW;

        bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);

        HINTERNET hSession = WinHttpOpen(
            L"NebulaSessions/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession)
            return false;

        WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);

        HINTERNET hConnect = WinHttpConnect(hSession, hostW.c_str(), port, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::wstring wMethod = Utf8ToWstring(method);
        DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            wMethod.c_str(),
            pathW.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        std::wstring reqHeaders;
        std::string token = TrimString(authToken);
        if (!token.empty())
            reqHeaders += L"x-session-token: " + Utf8ToWstring(token) + L"\r\n";
        reqHeaders += L"Content-Type: application/json\r\n";
        reqHeaders += L"Accept: application/json\r\n";
        reqHeaders += L"User-Agent: NebulaGS/1.0\r\n";

        DWORD totalLen = reqBody.empty() ? 0 : (DWORD)reqBody.size();

        BOOL bResults = WinHttpSendRequest(
            hRequest,
            reqHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : reqHeaders.c_str(),
            reqHeaders.empty() ? 0 : (DWORD)-1L,
            reqBody.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)reqBody.data(),
            totalLen,
            totalLen,
            0);

        if (!bResults)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        bResults = WinHttpReceiveResponse(hRequest, nullptr);
        if (!bResults)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        if (pOutStatus)
        {
            DWORD sc = 0;
            DWORD scSize = sizeof(sc);
            WinHttpQueryHeaders(
                hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &sc,
                &scSize,
                WINHTTP_NO_HEADER_INDEX);
            *pOutStatus = sc;
        }

        for (;;)
        {
            DWORD dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                break;
            if (dwSize == 0)
                break;

            std::string buffer;
            buffer.resize(dwSize);

            DWORD dwDownloaded = 0;
            if (!WinHttpReadData(hRequest, &buffer[0], dwSize, &dwDownloaded))
                break;
            if (dwDownloaded == 0)
                break;

            buffer.resize(dwDownloaded);
            outBody.append(buffer);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return true;
    }

    static std::string GenerateSessionId(const std::string& serverName, int port)
    {
        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve(32);

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        srand((unsigned int)(counter.QuadPart ^ GetTickCount64() ^ port));

        for (int i = 0; i < 32; i++)
            out.push_back(hex[rand() % 16]);

        return out;
    }
}

namespace NebulaSessions
{
    static std::atomic<bool> bHeartbeatRunning{ false };
    static std::thread heartbeatThread;
    static std::string sSessionId;
    static std::string sPlaylistName;
    static std::string sRegion = "BR";
    static std::string sHost;
    static int sPort = 0;
    static int sBeaconPort = 0;
    static std::atomic<int> sStatus{ 0 };
    static std::atomic<int> sPlayerCount{ 0 };

    static const char* CurrentStatusString()
    {
        int v = sStatus.load();
        if (v == 1)
            return "lobby";
        if (v == 2)
            return "em_partida";
        return "reiniciando";
    }

    static bool RegisterSession()
    {
        std::string base = NebulaApiInternal::BuildBaseUrl();
        if (base.empty())
        {
            NebulaApiInternal::NebulaLog("RegisterSession: base URL is empty!");
            return false;
        }

        std::string serverName = Globals::NebulaServerName;
        std::string portStr = std::to_string(sPort);
        std::string capStr = std::to_string(Globals::NebulaPlayerCap);
        std::string customCode = Globals::NebulaCustomCode.empty() ? "none" : Globals::NebulaCustomCode;

        if (sSessionId.empty())
            sSessionId = NebulaApiInternal::GenerateSessionId(serverName.empty() ? "server" : serverName, sPort);

        DWORD httpStatus = 0;
        std::string resp;

        std::string regUrl = base + "/nebula/gs/register/"
            + NebulaApiInternal::UrlEncode(sRegion) + "/"
            + NebulaApiInternal::UrlEncode(sHost) + "/"
            + NebulaApiInternal::UrlEncode(portStr) + "/"
            + NebulaApiInternal::UrlEncode(sPlaylistName) + "/"
            + NebulaApiInternal::UrlEncode(serverName);

        NebulaApiInternal::NebulaLog("RegisterSession: [1/4] GET " + regUrl);
        NebulaApiInternal::HttpRequest(regUrl, "GET", "", resp, &httpStatus);
        NebulaApiInternal::NebulaLog("RegisterSession: gs/register HTTP " + std::to_string(httpStatus));

        std::string statusUrl = base + "/nebula/gs/status/"
            + NebulaApiInternal::UrlEncode(serverName) + "/online?port=" + NebulaApiInternal::UrlEncode(portStr);

        NebulaApiInternal::NebulaLog("RegisterSession: [2/4] GET " + statusUrl);
        httpStatus = 0;
        resp.clear();
        NebulaApiInternal::HttpRequest(statusUrl, "GET", "", resp, &httpStatus);
        NebulaApiInternal::NebulaLog("RegisterSession: gs/status HTTP " + std::to_string(httpStatus));

        std::string addUrl = base + "/nebula/session/add/"
            + NebulaApiInternal::UrlEncode(sSessionId) + "/"
            + NebulaApiInternal::UrlEncode(sRegion) + "/"
            + NebulaApiInternal::UrlEncode(portStr) + "/"
            + NebulaApiInternal::UrlEncode(sPlaylistName) + "/"
            + NebulaApiInternal::UrlEncode(capStr) + "/"
            + NebulaApiInternal::UrlEncode(customCode)
            + "?ip=" + NebulaApiInternal::UrlEncode(sHost);

        NebulaApiInternal::NebulaLog("RegisterSession: [3/4] GET " + addUrl);
        httpStatus = 0;
        resp.clear();
        NebulaApiInternal::HttpRequest(addUrl, "GET", "", resp, &httpStatus);
        NebulaApiInternal::NebulaLog("RegisterSession: session/add HTTP " + std::to_string(httpStatus));

        std::string updateUrl = base + "/nebula/session/update/" + NebulaApiInternal::UrlEncode(sSessionId);
        std::string updateBody = "{\"players\":0,\"status\":\"open\",\"locked\":false,\"host\":\""
            + NebulaApiInternal::EscapeJson(sHost) + "\",\"port\":" + portStr + "}";

        NebulaApiInternal::NebulaLog("RegisterSession: [4/4] POST " + updateUrl);
        httpStatus = 0;
        resp.clear();
        bool ok = NebulaApiInternal::HttpRequest(updateUrl, "POST", updateBody, resp, &httpStatus);
        NebulaApiInternal::NebulaLog("RegisterSession: session/update HTTP " + std::to_string(httpStatus));

        if (ok && httpStatus >= 200 && httpStatus < 300)
        {
            NebulaApiInternal::NebulaLog("RegisterSession: OK");
            Globals::NebulaSessionId = sSessionId;
            return true;
        }

        NebulaApiInternal::NebulaLog("RegisterSession: FAILED");
        return false;
    }

    static void HeartbeatLoop()
    {
        while (bHeartbeatRunning.load())
        {
            std::string base = NebulaApiInternal::BuildBaseUrl();
            if (!base.empty() && !sSessionId.empty())
            {
                DWORD httpStatus = 0;
                std::string resp;
                int players = sPlayerCount.load();
                std::string portStr = std::to_string(sPort);
                bool locked = (sStatus.load() == 2);
                std::string status = locked ? "locked" : "open";

                std::string updateUrl = base + "/nebula/session/update/" + NebulaApiInternal::UrlEncode(sSessionId);
                std::string updateBody = "{\"players\":" + std::to_string(players)
                    + ",\"status\":\"" + status + "\""
                    + ",\"locked\":" + (locked ? "true" : "false")
                    + ",\"host\":\"" + NebulaApiInternal::EscapeJson(sHost) + "\""
                    + ",\"port\":" + portStr + "}";

                NebulaApiInternal::HttpRequest(updateUrl, "POST", updateBody, resp, &httpStatus);

                if (httpStatus < 200 || httpStatus >= 300)
                    NebulaApiInternal::NebulaLog("HeartbeatLoop: session/update failed (HTTP " + std::to_string(httpStatus) + ")");
            }

            int sleepSec = Globals::NebulaHeartbeatSec > 0 ? Globals::NebulaHeartbeatSec : 5;
            for (int i = 0; i < sleepSec && bHeartbeatRunning.load(); ++i)
                Sleep(1000);
        }
    }

    bool StartSession(const std::string& host, int port, const std::string& playlistName)
    {
        sHost = NebulaApiInternal::TrimString(host);
        sPort = port;
        sBeaconPort = NebulaApiInternal::ComputeBeaconPort(port);
        sRegion = Globals::NebulaRegion.empty() ? "BR" : Globals::NebulaRegion;
        sPlaylistName = playlistName.empty() ? "playlist_showdownalt_duos" : playlistName;

        sStatus.store(1);

        NebulaApiInternal::NebulaLog("StartSession: host=" + sHost + " port=" + std::to_string(sPort) + " playlist=" + sPlaylistName);
        NebulaApiInternal::NebulaLog("StartSession: baseUrl=" + NebulaApiInternal::BuildBaseUrl());

        bool regOk = RegisterSession();
        if (!regOk)
            NebulaApiInternal::NebulaLog("StartSession: initial registration failed, heartbeat will retry");

        if (!bHeartbeatRunning.load())
        {
            bHeartbeatRunning.store(true);
            heartbeatThread = std::thread(HeartbeatLoop);
            NebulaApiInternal::NebulaLog("StartSession: heartbeat thread started");
        }

        return true;
    }

    void StopSession()
    {
        if (!bHeartbeatRunning.load())
            return;

        bHeartbeatRunning.store(false);

        if (heartbeatThread.joinable())
            heartbeatThread.join();
    }

    void SetStatusLobby()
    {
        sStatus.store(1);
    }

    void SetStatusInMatch()
    {
        sStatus.store(2);
    }

    void SetStatusRebooting()
    {
        sStatus.store(0);
    }

    void SetPlayerCount(int count)
    {
        if (count < 0)
            count = 0;
        sPlayerCount.store(count);
    }

}

// ═════════════════════════════════════════════════════════════════════════════
//  NebulaMatchStats  —  arena
//  /nebula/applyMatchStats?auth=?n3b7x!ayz&username=X&reason=changehype&amount=N
// ═════════════════════════════════════════════════════════════════════════════
namespace NebulaMatchStats
{
    bool ApplyMatchStats(const std::string& username, int32_t amount)
    {
        std::string u = NebulaApiInternal::TrimString(username);
        if (u.empty())
        {
            NebulaApiInternal::NebulaLog("ApplyMatchStats: username vazio");
            return false;
        }
        if (amount == 0)
        {
            NebulaApiInternal::NebulaLog("ApplyMatchStats: amount=0, ignorado (" + u + ")");
            return false;
        }

        std::string url = std::string(NebulaApiInternal::kMatchStatsBaseUrl)
            + "/nebula/applyMatchStats"
            + "?auth=" + NebulaApiInternal::UrlEncode(NebulaApiInternal::kInternalApiKey)
            + "&username=" + NebulaApiInternal::UrlEncode(u)
            + "&reason=changehype"
            + "&amount=" + std::to_string(amount);

        DWORD httpStatus = 0;
        std::string resp;

        bool netOk = NebulaApiInternal::HttpRequest(
            url, "GET", "", resp, &httpStatus, NebulaApiInternal::kInternalApiKey);

        if (!netOk || httpStatus < 200 || httpStatus >= 300)
        {
            NebulaApiInternal::NebulaLog("ApplyMatchStats: FALHOU user=" + u
                + " amount=" + std::to_string(amount)
                + " HTTP " + std::to_string(httpStatus));
            return false;
        }

        NebulaApiInternal::NebulaLog("ApplyMatchStats: OK user=" + u
            + " amount=" + (amount > 0 ? "+" : "") + std::to_string(amount));
        return true;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  NebulaPlayerData  —  cache de divisão por player
//
//  Endpoint: GET /nebula/getBalanceById?username=X
//  Response: {"accountId":"...","username":"...","level":N,"vbucks":N,"hype":N,"division":N}
//
//  FetchDivisionAsync é chamado no join do player (ServerLoadingScreenDropped).
//  É async e não bloqueia a game thread.
//  GetPlayerDivision retorna o valor cacheado (default=1 se não resolveu ainda).
// ═════════════════════════════════════════════════════════════════════════════
namespace NebulaPlayerData
{
    // ANTI-CRASH: SRWLOCK em vez de std::mutex.
    // std::mutex usa ponteiro interno (_Mtx_t) inicializado pelo construtor via MSVCP140.dll.
    // Se o construtor não rodou (inicialização estática falhou), _Mtx_t é null →
    // lock() lê 0x0 → crash EXCEPTION_ACCESS_VIOLATION no MSVCP140.dll.
    // SRWLOCK é zero-initialized por padrão (SRWLOCK_INIT = {0}), sem construtor,
    // e AcquireSRWLockExclusive funciona corretamente em qualquer estado zerado.
    static SRWLOCK                              sCacheLock = SRWLOCK_INIT;
    static std::unordered_map<std::string, int> sDivisionCache;

    static int ParseDivisionFromJson(const std::string& json)
    {
        // Procura "division": N no JSON retornado pelo backend
        auto pos = json.find("\"division\":");
        if (pos == std::string::npos) return 1;
        pos += 11; // pula "division":
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
        int div = 0;
        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
        {
            div = div * 10 + (json[pos] - '0');
            ++pos;
        }
        return (div >= 1 && div <= 10) ? div : 1;
    }

    void FetchDivisionAsync(const std::string& username)
    {
        if (username.empty()) return;

        // Se já tem no cache, não busca de novo
        {
            AcquireSRWLockExclusive(&sCacheLock);
            bool cached = sDivisionCache.count(username) > 0;
            ReleaseSRWLockExclusive(&sCacheLock);
            if (cached) return;
        }

        std::thread([username]()
            {
                try
                {
                    std::string base = NebulaApiInternal::BuildBaseUrl();
                    if (base.empty()) return;

                    std::string url = base
                        + "/nebula/getBalanceById?username="
                        + NebulaApiInternal::UrlEncode(username);

                    DWORD hs = 0;
                    std::string resp;
                    bool ok = NebulaApiInternal::HttpRequest(url, "GET", "", resp, &hs);

                    if (!ok || hs < 200 || hs >= 300)
                    {
                        NebulaApiInternal::NebulaLog("PlayerData: FALHOU user=" + username
                            + " HTTP " + std::to_string(hs) + " → div=1 (default)");
                        // Armazena default para não tentar de novo
                        AcquireSRWLockExclusive(&sCacheLock);
                        sDivisionCache[username] = 1;
                        ReleaseSRWLockExclusive(&sCacheLock);
                        return;
                    }

                    int div = ParseDivisionFromJson(resp);

                    {
                        AcquireSRWLockExclusive(&sCacheLock);
                        sDivisionCache[username] = div;
                        ReleaseSRWLockExclusive(&sCacheLock);
                    }

                    NebulaApiInternal::NebulaLog("PlayerData: OK user=" + username
                        + " div=" + std::to_string(div));
                }
                catch (...) {}
            }).detach();
    }

    int GetPlayerDivision(const std::string& username)
    {
        AcquireSRWLockShared(&sCacheLock);
        auto it = sDivisionCache.find(username);
        int result = (it != sDivisionCache.end()) ? it->second : 1;
        ReleaseSRWLockShared(&sCacheLock);
        return result;
    }

    void ClearCache()
    {
        AcquireSRWLockExclusive(&sCacheLock);
        sDivisionCache.clear();
        ReleaseSRWLockExclusive(&sCacheLock);
        NebulaApiInternal::NebulaLog("PlayerData: cache limpo");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  NebulaXP  —  /addxpamount?auth=!n6eb7u?2la&username=X&amount=N
// ═════════════════════════════════════════════════════════════════════════════
namespace NebulaXP
{
    bool AddXP(const std::string& username, int32_t xpAmount)
    {
        std::string u = NebulaApiInternal::TrimString(username);
        if (u.empty())
        {
            NebulaApiInternal::NebulaLog("AddXP: username vazio");
            return false;
        }
        if (xpAmount <= 0)
        {
            NebulaApiInternal::NebulaLog("AddXP: xp invalido=" + std::to_string(xpAmount) + " (" + u + ")");
            return false;
        }

        // auth separado: !n6eb7u?2la  (diferente do arena auth)
        static const char* kXPAuth = "!n6eb7u?2la";

        std::string url = std::string(NebulaApiInternal::kMatchStatsBaseUrl)
            + "/addxpamount"
            + "?auth=" + NebulaApiInternal::UrlEncode(kXPAuth)
            + "&username=" + NebulaApiInternal::UrlEncode(u)
            + "&amount=" + std::to_string(xpAmount);

        DWORD httpStatus = 0;
        std::string resp;

        bool netOk = NebulaApiInternal::HttpRequest(
            url, "GET", "", resp, &httpStatus, kXPAuth);

        if (!netOk || httpStatus < 200 || httpStatus >= 300)
        {
            NebulaApiInternal::NebulaLog("AddXP: FALHOU user=" + u
                + " xp=" + std::to_string(xpAmount)
                + " HTTP " + std::to_string(httpStatus));
            return false;
        }

        NebulaApiInternal::NebulaLog("AddXP: OK user=" + u + " xp=+" + std::to_string(xpAmount));
        return true;
    }
}

namespace NebulaTournament
{
    static std::string GetEventId()
    {
        std::string eventId = NebulaApiInternal::TrimString(Globals::NebulaTournamentEventId);
        if (!Globals::IsTournamentModeEnabled())
            return std::string();
        if (eventId.empty() || eventId == "none" || eventId == "false" || eventId == "0" || eventId == "disabled")
            return std::string();
        return eventId;
    }

    static std::string StableMatchSessionId(const std::string& requestedSessionId = std::string())
    {
        static std::string fallbackSessionId;
        std::string explicitId = NebulaApiInternal::TrimString(requestedSessionId);
        if (!explicitId.empty())
            return explicitId;

        std::string liveId = NebulaApiInternal::TrimString(Globals::NebulaSessionId);
        if (!liveId.empty())
        {
            fallbackSessionId = liveId;
            return fallbackSessionId;
        }

        if (fallbackSessionId.empty())
        {
            int port = Globals::NebulaListeningPort > 0 ? Globals::NebulaListeningPort : 7777;
            fallbackSessionId = std::string("Tournament_") + std::to_string(port);
        }

        return fallbackSessionId;
    }

    static std::string BuildTournamentBody(const std::string& username, const std::string& sessionId, int currentKills, int placement, bool includeKills, bool includePlacement, bool victory, int timeAlive)
    {
        std::ostringstream out;
        std::string internalKey = NebulaApiInternal::TrimString(Globals::NebulaInternalApiKey);
        if (internalKey.empty())
            internalKey = NebulaApiInternal::TrimString(Globals::NebulaAuthToken);
        out << "{\"auth\":\"" << NebulaApiInternal::EscapeJson(internalKey) << "\"";
        out << ",\"username\":\"" << NebulaApiInternal::EscapeJson(username) << "\"";
        out << ",\"sessionId\":\"" << NebulaApiInternal::EscapeJson(sessionId) << "\"";
        if (includeKills)
            out << ",\"currentKills\":" << (currentKills < 0 ? 0 : currentKills);
        if (includePlacement)
            out << ",\"placement\":" << (placement < 0 ? 0 : placement);
        if (victory)
            out << ",\"victory\":true";
        if (timeAlive > 0)
            out << ",\"timeAlive\":" << timeAlive;
        out << "}";
        return out.str();
    }

    static bool PostTournamentRoute(const std::string& route, const std::string& body, const std::string& logLabel)
    {
        std::string base = NebulaApiInternal::BuildBaseUrl();
        if (base.empty())
            return false;

        std::string url = base + route;
        DWORD httpStatus = 0;
        std::string resp;
        std::string internalKey = NebulaApiInternal::TrimString(Globals::NebulaInternalApiKey);
        if (internalKey.empty())
            internalKey = NebulaApiInternal::TrimString(Globals::NebulaAuthToken);
        bool ok = NebulaApiInternal::HttpRequest(url, "POST", body, resp, &httpStatus, internalKey);

        if (!ok || httpStatus < 200 || httpStatus >= 300)
        {
            NebulaApiInternal::NebulaLog(logLabel + " FAILED HTTP " + std::to_string(httpStatus) + " url=" + url);
            return false;
        }

        NebulaApiInternal::NebulaLog(logLabel + " OK url=" + url);
        return true;
    }

    static void PostTournamentAsync(const std::string& route, const std::string& body, const std::string& logLabel)
    {
        std::thread([route, body, logLabel]()
            {
                try
                {
                    PostTournamentRoute(route, body, logLabel);
                }
                catch (...)
                {
                }
            }).detach();
    }

    void StartMatchAsync(const std::string& username, const std::string& sessionId)
    {
        std::string cleanUsername = NebulaApiInternal::TrimString(username);
        if (cleanUsername.empty())
            return;

        std::string eventId = GetEventId();
        if (eventId.empty())
            return;

        std::string cleanSessionId = StableMatchSessionId(sessionId);
        std::string route = "/api/v1/tournaments/" + NebulaApiInternal::UrlEncode(eventId) + "/match/start";
        std::string body = BuildTournamentBody(cleanUsername, cleanSessionId, 0, 0, false, false, false, 0);
        PostTournamentAsync(route, body, "TournamentStart");
    }

    void SyncKillsAsync(const std::string& username, int currentKills, const std::string& sessionId, int timeAlive)
    {
        std::string cleanUsername = NebulaApiInternal::TrimString(username);
        if (cleanUsername.empty())
            return;

        std::string eventId = GetEventId();
        if (eventId.empty())
            return;

        std::string cleanSessionId = StableMatchSessionId(sessionId);
        std::string route = "/api/v1/tournaments/" + NebulaApiInternal::UrlEncode(eventId) + "/match/kill-sync";
        std::string body = BuildTournamentBody(cleanUsername, cleanSessionId, currentKills, 0, true, false, false, timeAlive);
        PostTournamentAsync(route, body, "TournamentKillSync");
    }

    void FinishMatchAsync(const std::string& username, int placement, int currentKills, const std::string& sessionId, bool victory, int timeAlive)
    {
        std::string cleanUsername = NebulaApiInternal::TrimString(username);
        if (cleanUsername.empty())
            return;

        std::string eventId = GetEventId();
        if (eventId.empty())
            return;

        std::string cleanSessionId = StableMatchSessionId(sessionId);
        std::string route = "/api/v1/tournaments/" + NebulaApiInternal::UrlEncode(eventId) + "/match/finish";
        std::string body = BuildTournamentBody(cleanUsername, cleanSessionId, currentKills, placement, true, true, victory || placement == 1, timeAlive);
        PostTournamentAsync(route, body, "TournamentFinish");
    }

    void ReportWinAsync(const std::string& username, int currentKills, const std::string& sessionId, int timeAlive)
    {
        FinishMatchAsync(username, 1, currentKills, sessionId, true, timeAlive);
    }

    void ChangePointsAsync(const std::string& username, int32_t amount)
    {
        std::string cleanUsername = NebulaApiInternal::TrimString(username);
        if (cleanUsername.empty() || amount == 0)
            return;

        std::string eventId = GetEventId();
        if (eventId.empty())
            return;

        std::thread([cleanUsername, amount, eventId]()
            {
                try
                {
                    std::string base = NebulaApiInternal::BuildBaseUrl();
                    if (base.empty())
                        return;

                    std::string internalKey = NebulaApiInternal::TrimString(Globals::NebulaInternalApiKey);
                    if (internalKey.empty())
                        internalKey = NebulaApiInternal::TrimString(Globals::NebulaAuthToken);
                    if (internalKey.empty())
                        internalKey = NebulaApiInternal::kInternalApiKey;

                    std::string url = base
                        + "/api/v1/tournaments/" + NebulaApiInternal::UrlEncode(eventId)
                        + "/changepoints"
                        + "?auth=" + NebulaApiInternal::UrlEncode(internalKey)
                        + "&username=" + NebulaApiInternal::UrlEncode(cleanUsername)
                        + "&amount=" + std::to_string(amount);

                    DWORD httpStatus = 0;
                    std::string resp;
                    bool ok = NebulaApiInternal::HttpRequest(url, "GET", "", resp, &httpStatus, internalKey);

                    if (!ok || httpStatus < 200 || httpStatus >= 300)
                        NebulaApiInternal::NebulaLog("TournamentChangePoints: FAILED user=" + cleanUsername
                            + " amount=" + std::to_string(amount)
                            + " HTTP " + std::to_string(httpStatus));
                    else
                        NebulaApiInternal::NebulaLog("TournamentChangePoints: OK user=" + cleanUsername
                            + " amount=" + (amount > 0 ? "+" : "") + std::to_string(amount));
                }
                catch (...) {}
            }).detach();
    }
}