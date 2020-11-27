#pragma once

#include <server_http.hpp>

#if _WINDOWS
#pragma warning(push)
#pragma warning(disable:4267)
#endif
#include <simple-websocket-server/server_ws.hpp>
#if _WINDOWS
#pragma warning(pop)
#endif

#include "AshLogger.h"
#include "Blockchain.h"
#include "ChainDatabase.h"
#include "Settings.h"
#include "PeerManager.h"

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

class MinerApp
{

public:
    MinerApp(SettingsPtr settings);
    ~MinerApp();

    void run();
    void signalExit() { _done = true; }
    void stopMining() { _miningDone = true; }

private:
    void initRest();
    void initWebSocket();

    void runMineThread();
    void syncBlockchain();

    void printIndex(HttpResponsePtr response);

private:
    ChainDatabasePtr        _database;
    BlockChainPtr           _blockchain;
    SettingsPtr             _settings;
    PeerManager             _peers;

    HttpServer              _httpServer;
    std::thread             _httpThread;

    WsServer                _wsServer;
    std::thread             _wsThread;

    bool                    _done = false;
    bool                    _miningDone = false;
    std::thread             _mineThread;

    SpdLogPtr                _logger;
};

template<typename NumberT>
class CumulativeMovingAverage
{
    NumberT     _total = 0;
    std::size_t _count = 0;
public:
    float value() const
    {
        // TODO: pretty sure only one cast is need, but not 100% sure
        return (static_cast<float>(_total) / static_cast<float>(_count));
    }

    void addValue(NumberT v)
    {
        _total += v;
        _count++;
    }

    void reset()
    {
        _total = 0;
        _count = 0;
    }
};

} // namespace
