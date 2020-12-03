#include <charconv>

#include <nlohmann/json.hpp>

#include "utils.h"
#include "MinerApp.h"

namespace nl = nlohmann;

namespace ash
{

#ifdef _RELEASE
constexpr auto BLOCK_GENERATION_INTERVAL = 10u * 60u; // in seconds
constexpr auto DIFFICULTY_ADJUSTMENT_INTERVAL = 25u; // in blocks
#else
constexpr auto BLOCK_GENERATION_INTERVAL = 60u; // in seconds
constexpr auto DIFFICULTY_ADJUSTMENT_INTERVAL = 10; // in blocks
#endif

MinerApp::MinerApp(SettingsPtr settings)
    : _settings{ std::move(settings) },
      _httpThread{},
      _mineThread{},
      _logger(ash::initializeLogger("MinerApp"))
{
    std::uint32_t difficulty = _settings->value("chain.difficulty", 5u);

    _logger->info("current difficulty set to {}", difficulty);
    _logger->debug("target block generation interval is {} seconds", BLOCK_GENERATION_INTERVAL);
    _logger->debug("difficulty adjustment interval is every {} blocks", DIFFICULTY_ADJUSTMENT_INTERVAL);

    initRest();
    initWebSocket();
    initPeers();

    const std::string dbfolder = _settings->value("database.folder", "");
    _database = std::make_unique<ChainDatabase>(dbfolder);
   
    _blockchain = std::make_unique<Blockchain>(difficulty);

    _database->initialize(*_blockchain);

    _peers.broadcast(R"({"message":"summary"})");
}

MinerApp::~MinerApp()
{
    if (_mineThread.joinable())
    {
        _mineThread.join();
    }

    if (_httpThread.joinable())
    {
        _httpServer.stop();
        _httpThread.join();
    }
}

void MinerApp::printIndex(HttpResponsePtr response)
{
    std::stringstream out;
    out << "<html><body>";
    out << "<h3>mining status: <b>"
        << (_miningDone ? "stopped" : "started")
        << "</b></h3>";

    out << "<h3>blockchain size: <b>"
        << "<a href='/block-idx/" 
            << (_blockchain->size() - 1) 
            << "'>" 
            << (_blockchain->size() - 1) 
            << "</a>"
        << "</b></h3>";

    if (_miningDone)
    {
        out << "<a href='/startMining'>start mining</a><br/>";
    }
    else
    {
        out << "<a href='/stopMining'>stop mining</a><br/>";
    }
    
    out << "<a href='/quit'>quit</a>";
   
    out << "</body></html>";
    response->write(out);
}

void MinerApp::initRest()
{
    _httpServer.config.port = _settings->value("rest.port", HTTPServerPortDefault);
    _httpServer.resource["^/$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->printIndex(response);
        };

    _httpServer.resource["^/block-idx/([0-9]+)$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request)
        {
            const auto indexStr = request->path_match[1].str();
            int index = 0;
            auto result = 
                std::from_chars(indexStr.data(), indexStr.data() + indexStr.size(), index);

            std::stringstream ss;
            if (result.ec != std::errc() || index >= _blockchain->size())
            {
                ss << R"xx(<html><body><h2 stye="color:red">Invalid Block</h2></body></html>)xx";
            }
            else
            {
                nl::json json = _blockchain->getBlockByIndex(index);
                ss << "<pre>" << json.dump(4) << "</pre>";
                ss << "<br/>";
                if (index > 0) ss << "<a href='/block-idx/" << (index - 1) << "'>prev</a>&nbsp;";
                ss << "current: " << index;
                if (index < _blockchain->size()) ss << "&nbsp;<a href='/block-idx/" << (index + 1) << "'>next</a>&nbsp;";
            }
            
            response->write(ss);
        };

    _httpServer.resource["^/quit$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->signalExit();
            std::stringstream stream;
            stream << "<html><body><h1>Shutdown requested</h1></body></html>";
            response->write(stream);
        };

    _httpServer.resource["^/stopMining$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->stopMining();
            std::stringstream stream;
            stream << "<html><body><h1>Mining shutdown requested</h1></body></html>";
            response->write(stream);

            if (this->_mineThread.joinable())
            {
                this->_mineThread.join();
            }
        };

    _httpServer.resource["^/startMining$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->_miningDone = false;
            this->_mineThread = std::thread(&MinerApp::runMineThread, this);
            std::stringstream stream;
            stream << "<html><body><h1>Mining startup requested</h1></body></html>";
            response->write(stream);
        };

    _httpServer.resource["^/addPeer/((?:[0-9]{1,3}\\.){3}[0-9]{1,3})$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request)
        {
            response->write("<h1>peer added</h1>");
        };
}

void MinerApp::initWebSocket()
{
    auto port = _settings->value("websocket.port", WebSocketServerPorDefault);
    _peers.initWebSocketServer(port);

    _peers.onChainRequest.connect(
        [this](WsServerConnPtr connection, const std::string& message)
        {
            this->dispatchRequest(connection, message);
        });

    _peers.onChainResponse.connect(
        [this](WsClientConnPtr connection, const std::string& response)
        {
            this->handleResponse(connection, response);
        });
}

void MinerApp::initPeers()
{
    const std::string peersfile = _settings->value("peers.file", "");
    if (peersfile.size() == 0) return;
    _peers.loadPeers(peersfile);
    _peers.connectAll(
        [](WsClientConnPtr conn)
        {
            conn->send(R"({"message":"summary"})");
        });
}

void MinerApp::run()
{
    _httpThread = std::thread(
        [this]()
        {
            _logger->debug("http server listening on port {}", _httpServer.config.port);
            _httpServer.start();
        });

    if (_settings->value("mining.autostart", false))
    {
        _mineThread = std::thread(&MinerApp::runMineThread, this);
    }
    else
    {
        _miningDone = true;
    }
    
    if (_settings->value("rest.autoload", false))
    {
        const auto address = _httpServer.config.address.empty() ? 
            "localhost" : _httpServer.config.address;

        const auto localUrl = fmt::format("http://{}:{}",
            address, _httpServer.config.port);
        utils::openBrowser(localUrl);
    }

    while (!_done)
    {
        std::this_thread::yield();
    }
}

void MinerApp::runMineThread()
{
    auto blockAdjustmentCount = 0u;
    ash::CumulativeMovingAverage<std::uint64_t> avg;

    std::uint64_t index = 0;
    {
        std::lock_guard<std::mutex> lock{_chainMutex};
        index = _blockchain->size();
    }
    
    while (!_miningDone && !_done)
    {
        _logger->info("mining block at index {} with difficulty {}", 
            index, _blockchain->difficulty());

        const std::string data = fmt::format(":coinbase{}", index);

        auto prevTime = static_cast<std::uint64_t>(_blockchain->back().time());

        auto newblock = 
            ash::Block(static_cast<std::uint32_t>(index++), data);

        // TODO: LOCKING!!
        // do mining
        _blockchain->MineBlock(newblock);

        // persist to database
        _database->write(newblock);

        // adjust difficulty if needed
        if (blockAdjustmentCount > DIFFICULTY_ADJUSTMENT_INTERVAL)
        {
            auto difficulty = _blockchain->difficulty();
            auto avgTime = avg.value();
            if (avgTime < (BLOCK_GENERATION_INTERVAL / 2.0f))
            {
                _logger->info("increasing difficulty from {} to {}", difficulty++, difficulty);
                _blockchain->setDifficulty(difficulty);
            }
            else if (avgTime > (BLOCK_GENERATION_INTERVAL * 2))
            {
                _logger->info("decreasing difficulty from {} to {}", difficulty--, difficulty);
                _blockchain->setDifficulty(difficulty);
            }
            else
            {
                _logger->debug("no difficulty adjustment required");
            }

            blockAdjustmentCount = 0;
            avg.reset();
        }
        else
        {
            avg.addValue(newblock.time() - prevTime);
            blockAdjustmentCount++;
        }

        // inform the network of our chain
        _peers.broadcast(R"({"message":"summary"})");
    }
}

// the blockchain is synced at startup and
// after each block is mined
void MinerApp::syncBlockchain()
{
    
}

void MinerApp::dispatchRequest(WsServerConnPtr connection, std::string_view rawmsg)
{
    nl::json json = nl::json::parse(rawmsg, nullptr, false);
    if (json.is_discarded() || !json.contains("message"))
    {
        _logger->warn("malformed ws:/chain request on connection {}", 
            static_cast<void*>(connection.get()));

        nl::json response = R"({ "error": "malformed request" })";
        connection->send(response.dump());
        return;
    }

    const auto message = json["message"].get<std::string>();

    _logger->debug("ws:/chain received request on connection {}: {}", 
        static_cast<void*>(connection.get()), message);

    std::stringstream response;

    nl::json jresponse;
    jresponse["message"] = message;

    if (message == "summary")
    {
        jresponse["blocks"].push_back(_blockchain->front());
        jresponse["blocks"].push_back(_blockchain->back());
    }
    else if (message == "chain")
    {
        if (!json.contains("id1") && !json.contains("id2"))
        {
            jresponse["blocks"] = *(this->_blockchain);
        }
        else if (!json["id1"].is_number())
        {
            jresponse["error"] = "invalid 'id1' value";
        }
        else if (!json["id2"].is_number())
        {
            jresponse["error"] = "invalid 'id2' value";
        }
        else
        {
            auto id1 = json["id1"].get<std::uint32_t>();
            auto id2 = json["id2"].get<std::uint32_t>();

            auto startIt = std::find_if(_blockchain->begin(), _blockchain->end(),
                [id1](const Block& block)
                {
                    return block.index() == id1;
                });

            if (startIt == _blockchain->end())
            {
                jresponse["error"] = "could not find id1 in chain";
            }
            else
            {
                for (auto currentIt = startIt; 
                    currentIt != _blockchain->end() && currentIt->index() <= id2; currentIt++)
                {
                    jresponse["blocks"].push_back(*currentIt);
                }
            }
        }
    }

    response << jresponse.dump();
    connection->send(response.str());
}

void MinerApp::handleResponse(WsClientConnPtr connection, std::string_view rawmsg)
{
    nl::json json = nl::json::parse(rawmsg, nullptr, false);
    if (json.is_discarded() || !json.contains("message"))
    {
        _logger->warn("malformed wc:/chain response on connection {}", 
            static_cast<void*>(connection.get()));

        return;
    }

    const auto message = json["message"].get<std::string>();

    _logger->trace("wsc:/chain received response on connection {}, message='{}'", 
        static_cast<void*>(connection.get()), message);

    if (message == "summary")
    {
        if (!json.contains("blocks")
            || !json["blocks"].is_array()
            || json["blocks"].size() != 2)
        {
            _logger->warn("malformed ws:/chain 'latest' response on connection {}", 
                static_cast<void*>(connection.get()));
            return;
        }

        const auto& remote_gen = json["blocks"].at(0).get<ash::Block>();
        const auto& remote_last = json["blocks"].at(1).get<ash::Block>();
        
        const auto& genesis = _blockchain->front();
        const auto& lastblock = _blockchain->back();

        if (genesis != remote_gen)
        {
            _logger->warn("wsc:/chain 'summary' returned unknown chain on connection {}", 
                static_cast<void*>(connection.get()));

            if (_settings->value("chain.reset.enable", false))
            {
                _logger->info("requesting full remote block");
                connection->send(R"({"message":"chain"})");
            }
        }
        else if (lastblock.index() < remote_last.index())
        {
            auto startIdx = lastblock.index() + 1;
            auto stopIdx = remote_last.index();

            _logger->info("'summary' returned larger chain on connection {}, requesting blocks {}-{}", 
                static_cast<void*>(connection.get()), startIdx, stopIdx);

            const auto msg = fmt::format(R"({{ "message":"chain","id1":{},"id2":{} }})",startIdx, stopIdx);
            connection->send(msg);
        }
    }
    else if (message == "chain")
    {
        if (const auto tempchain = json["blocks"].get<ash::Blockchain>();
                tempchain.size() <= 0 || !tempchain.isValidChain())
        {
            _logger->info("received invalid chain from connection {}", 
                static_cast<void*>(connection.get()));
        }
        else if (tempchain.front().index() == 0)
        {
            std::lock_guard<std::mutex> lock(_chainMutex);

            _logger->info("using remote chain with blocks {}-{}",
                tempchain.front().index(), tempchain.back().index());

            *_blockchain = std::move(tempchain);
            _database->reset();
            _database->writeChain(*_blockchain);
        }
        else if (tempchain.front().index() == _blockchain->back().index() + 1)
        {
            std::lock_guard<std::mutex> lock(_chainMutex);

            _logger->info("adding blocks {}-{}",
                tempchain.front().index(), tempchain.back().index());

            for (const auto& block : tempchain)
            {
                if (_blockchain->addNewBlock(block))
                {
                    _database->write(block);
                }
            }
        }
        else if (tempchain.front().index() < _blockchain->back().index())
        {
            std::lock_guard<std::mutex> lock(_chainMutex);

            _logger->info("updating chain from {}-{} and adding blocks {}-{}",
                tempchain.front().index(), _blockchain->back().index(),
                _blockchain->back().index() + 1, tempchain.back().index());

            // need to cover cases where we mined a few blocks but
            // the remote chain already had them
        }
    }
}

} // namespace
