#include <charconv>

#include <nlohmann/json.hpp>

#include "utils.h"
#include "MinerApp.h"

namespace nl = nlohmann;

namespace ash
{


#ifdef _RELEASE
constexpr auto BLOCK_GENERATION_INTERVAL = 10u * 60u; // in seconds
constexpr auto DIFFICULTY_ADJUSTMENT_INTERVAL = 10u; // in blocks
#else
constexpr auto BLOCK_GENERATION_INTERVAL = 10u; // in seconds
constexpr auto DIFFICULTY_ADJUSTMENT_INTERVAL = 10; // in blocks
#endif

MinerApp::MinerApp(SettingsPtr settings)
    : _settings{ std::move(settings) },
      _httpThread{},
      _mineThread{},
      _logger(ash::initializeLogger("MinerApp"))
{
    _logger->debug("target block generation interval is {} seconds", BLOCK_GENERATION_INTERVAL);
    _logger->debug("difficulty adjustment interval is every {} blocks", DIFFICULTY_ADJUSTMENT_INTERVAL);

    initRest();
    initWebSocket();

    const std::string dbfolder = _settings->value("database.folder", "");
    _database = std::make_unique<ChainDatabase>(dbfolder);

    std::uint32_t difficulty = _settings->value("chain.difficulty", 5u);
    _blockchain = std::make_unique<Blockchain>(difficulty);

    _database->initialize(*_blockchain);

    syncBlockchain();
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

    if (_wsThread.joinable())
    {
        _wsServer.stop();
        _wsThread.join();
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
    _wsServer.config.port = _settings->value("websocket.port", WebSocketServerPorDefault);
    _wsServer.endpoint["^/echo/?$"].on_open = 
        [this](std::shared_ptr<WsServer::Connection> connection) 
        {
            _logger->trace("ws:/echo opened connection {}", static_cast<void*>(connection.get()));
        };

    _wsServer.endpoint["^/echo/?$"].on_message =
        [this](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> in_message)
        {
            _logger->trace("ws:/echo received message on connection {}", static_cast<void*>(connection.get()));

            // connection->send is an asynchronous function (you can pass a lambda)
            auto out_message = in_message->string();
            connection->send(out_message);
        };

    _wsServer.endpoint["^/blocks/latest$"].on_message =
        [this](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage>)
        {
            _logger->trace("ws:/block/latest message received message on connection {}", static_cast<void*>(connection.get()));
            nl::json j = this->_blockchain->back();
            std::stringstream response;
            response << j;
            connection->send(response.str());
        };

    _wsServer.endpoint["^/blocks/chain$"].on_message = 
        [this](std::shared_ptr<WsServer::Connection> connection, std::shared_ptr<WsServer::InMessage> in_message)
        {
            _logger->trace("ws:/block/chain message received message on connection {}", static_cast<void*>(connection.get()));
            //nl::json json = nl::json::parse(in_message->string(), nullptr, false);
            //if (json.is_discarded())
            //{
            //    _logger->info("invalid message request on connection {}", 
            //        static_cast<void*>(connection.get()));


            //    nl::json response = R"({ "blocks": [] })";
            //    connection->send(response.dump());
            //    return;
            //}
            //
            //auto id1 = json["id1"].get<std::uint32_t>();
            //auto id2 = json["id2"].get<std::uint32_t>();

            nl::json j = *(this->_blockchain);
            std::stringstream response;
            response << j;
            connection->send(response.str());
        };
}

void MinerApp::run()
{
    _httpThread = std::thread(
        [this]()
        {
            _logger->debug("http server listening on port {}", _httpServer.config.port);
            _httpServer.start();
        });

    _wsThread = std::thread(
        [this]()
        {
            _logger->debug("websocket server listening on port {}", _wsServer.config.port);
            _wsServer.start();
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

    auto index = _blockchain->size();

    while (!_miningDone && !_done)
    {
        _logger->info("mining block at index {} with difficult {}", 
            index, _blockchain->difficulty());

        const std::string data = fmt::format(":coinbase{}", index);

        auto prevTime = static_cast<std::uint64_t>(_blockchain->back().time());

        auto newblock = 
            ash::Block(static_cast<std::uint32_t>(index++), data);

        // do mining
        _blockchain->AddBlock(newblock);

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

        // sync the network
        syncBlockchain();
    }
}

// the blockchain is synced at startup and
// after each block is mined
void MinerApp::syncBlockchain()
{

}

} // namespace
