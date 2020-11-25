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

#include "Blockchain.h"
#include "ChainDatabase.h"
#include "Settings.h"

namespace ash
{

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpRequest = HttpServer::Request;
using HttpResponse = HttpServer::Response;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;

class MinerApp
{

public:
    MinerApp(SettingsPtr settings);
    ~MinerApp();

    void run();

private:
    void initRest();
    void runMineThread();

private:
    ChainDatabasePtr        _database;
    BlockChainPtr           _blockchain;
    SettingsPtr             _settings;

    HttpServer              _httpServer;
    WsServer                _wsServer;
    std::thread             _httpThread;

    bool                    _done = false;
    std::thread             _mineThread;
};

} // namespace
