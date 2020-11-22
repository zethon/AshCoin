#pragma once

#include <server_http.hpp>

#include "Blockchain.h"
#include "ChainDatabase.h"
#include "Settings.h"

namespace ash
{

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpRequest = HttpServer::Request;
using HttpResponse = HttpServer::Response;

class MinerApp
{

public:
    MinerApp(SettingsPtr settings);
    ~MinerApp();

    void run();

private:
    void initRest();
    void doMine();

private:
    ChainDatabasePtr        _database;
    BlockChainPtr           _blockchain;
    SettingsPtr             _settings;

    HttpServer              _httpServer;
    std::thread             _httpThread;

    bool                    _done = false;
    std::thread             _mineThread;
};

} // namespace
