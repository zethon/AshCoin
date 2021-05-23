#pragma once

#include <server_http.hpp>

#if _WINDOWS
#pragma warning(push)
#pragma warning(disable:4267)
#endif
#include <Simple-WebSocket-Server/server_ws.hpp>
#if _WINDOWS
#pragma warning(pop)
#endif

#include "AshUtils.h"
#include "AshLogger.h"
#include "Blockchain.h"
#include "ChainDatabase.h"
#include "Settings.h"
#include "PeerManager.h"
#include "Miner.h"

namespace ash
{

constexpr auto HTTPServerPortDefault = 27182u;
constexpr auto WebSocketServerPorDefault = 14142u;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

using HttpRequest = HttpServer::Request;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;

using HttpResponse = HttpServer::Response;
using HttpResponsePtr = std::shared_ptr<HttpResponse>;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

struct UpdatePackage
{
    BlockChainPtr   tempchain;
    std::uint64_t   tempcumdiff;
};

class MinerApp
{

public:
    MinerApp(SettingsPtr settings);
    ~MinerApp();

    void run();

    void stopMining() 
    { 
        _miningDone = true; 
        _miner.abort();
    }

    void signalExit()
    { 
        stopMining();
        _done = true; 
    }

private:
    void initHttp();
    void initWebService();
    void initRestService();

    void initWebSocket();
    void initPeers();

    void runMineThread();
    [[maybe_unused]] bool syncBlockchain();
    void broadcastNewBlock(const Block& block);

    using HcConnection = PeerManager::ConnectionProxy;
    using HcConnectionPtr = std::shared_ptr<HcConnection>;

    void dispatchRequest(HcConnectionPtr, const nl::json& json);
    void handleResponse(HcConnectionPtr, const nl::json& json);
    void handleChainResponse(HcConnectionPtr, const Blockchain&);
    void handleError(HcConnectionPtr, const nl::json&);

    void servePage(HttpResponsePtr response, 
        std::string_view filename, const std::string& content, const utils::Dictionary& dict);
    void getStandardDictionary(utils::Dictionary& dict);

private:
    std::string             _uuid;
    std::string             _rewardAddress;

    IChainDatabasePtr        _database;
    
    std::mutex              _chainMutex;    // chain mutex
    
    BlockChainPtr           _blockchain;
    BlockChainPtr           _tempchain;

    SettingsPtr             _settings;
    PeerManager             _peers;

    HttpServer              _httpServer;
    std::thread             _httpThread;

    std::atomic_bool        _done = false;
    std::atomic_bool        _miningDone = false;
    
    Miner                   _miner;
    std::thread             _mineThread;

    SpdLogPtr               _logger;
};

} // namespace
