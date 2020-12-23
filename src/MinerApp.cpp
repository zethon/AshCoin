#include <charconv>
#include <cassert>

#include <nlohmann/json.hpp>
#include <fmt/chrono.h>

#include "index_html.h"

#include "CryptoUtils.h"
#include "utils.h"
#include "core.h"
#include "ComputerUUID.h"
#include "Transactions.h"

#include "MinerApp.h"

namespace nl = nlohmann;

namespace ash
{

constexpr std::string_view GENESIS_BLOCK = "HenryCoin Genesis";

MinerApp::MinerApp(SettingsPtr settings)
    : _settings{ std::move(settings) },
      _httpThread{},
      _mineThread{},
      _logger(ash::initializeLogger("MinerApp"))
{
    const std::string dbfolder = _settings->value("database.folder", "");

    utils::ComputerUUID uuid;
    uuid.setCustomData(dbfolder);
    _uuid = uuid.getUUID();
    
    _logger->info("current miner uuid is {}", _uuid);

    _logger->debug("target block generation interval is {} seconds", TARGET_TIMESPAN);
    _logger->debug("difficulty adjustment interval is every {} blocks", BLOCK_INTERVAL);

    _blockchain = std::make_unique<Blockchain>();
    _database = std::make_unique<ChainDatabase>(dbfolder);
}

MinerApp::~MinerApp()
{
    if (_mineThread.joinable())
    {
        _logger->trace("shutting down mining thread");
        _mineThread.join();
    }

    if (_httpThread.joinable())
    {
        _logger->trace("shutting down http server thread");
        _httpServer.stop();
        _httpThread.join();
    }
}

void MinerApp::printIndex(HttpResponsePtr response)
{
    utils::Dictionary dict;
    dict["%app-title%"] = APP_NAME_LONG;
    dict["%app-domain%"] = APP_DOMAIN;
    dict["%app-github%"] = GITHUB_PAGE;
    dict["%app-copyright%"] = COPYRIGHT;
    dict["%build-date%"] = BUILDTIMESTAMP;
    dict["%build-version%"] = VERSION;
    dict["%chain-size%"] = std::to_string(_blockchain->size() - 1);
    dict["%chain-diff%"] = std::to_string(_miner.difficulty());
    dict["%chain-cumdiff%"] = std::to_string(_blockchain->cumDifficulty());
    dict["%mining-status%"] = (_miningDone ? "stopped" : "started");
    dict["%mining-uuid%"] = _uuid;
    dict["%rest-port%"] 
        = std::to_string(_settings->value("rest.port", HTTPServerPortDefault));

    std::stringstream out;
    out << utils::DoDictionary(index_html, dict);
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
                nl::json json = _blockchain->at(index);
                ss << "<pre>" << json.dump(4) << "</pre>";
                ss << "<br/>";
                if (index > 0) ss << "<a href='/block-idx/" << (index - 1) << "'>prev</a>&nbsp;";
                ss << "current: " << index;
                if (index < _blockchain->size()) ss << "&nbsp;<a href='/block-idx/" << (index + 1) << "'>next</a>&nbsp;";
            }
            
            response->write(ss);
        };

    _httpServer.resource["^/shutdown$"]["POST"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            _logger->debug("shutdown request from {}", 
                request->remote_endpoint().address().to_string());

            response->write(SimpleWeb::StatusCode::success_ok, "OK");
            this->signalExit();
        };

    _httpServer.resource["^/stopMining$"]["POST"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            _logger->debug("stopMining request from {}", 
                request->remote_endpoint().address().to_string());

            if (!this->_miningDone)
            {
                this->stopMining();
                if (this->_mineThread.joinable())
                {
                    this->_mineThread.join();
                }
                response->write(SimpleWeb::StatusCode::success_ok, "OK");
            }
            else
            {
                response->write(SimpleWeb::StatusCode::client_error_bad_request);
            }
        };

    _httpServer.resource["^/startMining$"]["POST"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            _logger->trace("startMining request from {}", 
                request->remote_endpoint().address().to_string());

            if (this->_miningDone)
            {
                this->_miningDone = false;
                this->_mineThread = std::thread(&MinerApp::runMineThread, this);
                response->write(SimpleWeb::StatusCode::success_ok, "OK");
            }
            else
            {
                response->write(SimpleWeb::StatusCode::client_error_bad_request);
            }
        };

    _httpServer.resource["^/summary$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request)
        {
            nl::json jresponse;
            jresponse["blocks"].push_back(_blockchain->back());
            jresponse["cumdiff"] = _blockchain->cumDifficulty();
            jresponse["difficulty"] = _miner.difficulty();

            if (!this->_miningDone)
            {
                jresponse["status"] = "mining #" + std::to_string(_blockchain->back().index() + 1);
            }
            else
            {
                jresponse["status"] = "stopped";
            }

            response->write(jresponse.dump());
        };

    _httpServer.resource["^/block/([0-9]+)$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            const auto indexStr = request->path_match[1].str();
            int blockIndex = 0;
            auto result = 
                std::from_chars(indexStr.data(), indexStr.data() + indexStr.size(), blockIndex);

            if (result.ec == std::errc::invalid_argument
                || blockIndex >= _blockchain->size())
            {
                response->write(SimpleWeb::StatusCode::client_error_bad_request);
                return;
            }

            nl::json json = _blockchain->at(blockIndex);
            response->write(json.dump());
        };

    _httpServer.resource["^/blocks/last/([0-9]+)$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            const auto indexStr = request->path_match[1].str();
            std::uint64_t startingIdx = 0;
            auto result = 
                std::from_chars(indexStr.data(), indexStr.data() + indexStr.size(), startingIdx);

            if (result.ec == std::errc::invalid_argument)
            {
                return;
            }

            nl::json json;

            startingIdx = std::max(
		static_cast<std::uint64_t>(0), static_cast<std::uint64_t>(_blockchain->size() - startingIdx));

            for (auto idx = startingIdx; idx < _blockchain->size(); idx++)
            {
                json["blocks"].push_back(_blockchain->at(idx));
            }
            response->write(json.dump());
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

    _peers.onChainMessage.connect(
        [this](PeerManager::ConnectionProxyPtr connection, std::string_view rawmsg)
        {
            const nl::json json = nl::json::parse(rawmsg, nullptr, false);
            if (json.is_discarded() 
                || !json.contains("message")
                || !json.contains("message-type"))
            {
                _logger->warn("ws:/chain received malformed message from node {}",
                    connection->address());

                connection->sendError("the recieved message was malformed");
                return;
            }
            
            _logger->debug("message='{}' message-type='{}' received from {}",
                json["message"].get<std::string>(), 
                json["message-type"].get<std::string>(),
                connection->address()); 

            if (auto msgtype = json["message-type"].get<std::string>(); 
                msgtype == "request")
            {
                this->dispatchRequest(connection, json);
            }
            else if (msgtype == "response")
            {
                this->handleResponse(connection, json);
            }
            else if (msgtype == "error")
            {
                this->handleError(connection, json);
            }
            else
            {
                _logger->warn("ws:/chain received unknown message-type '{}' from node {}",
                    msgtype, connection->address());

                connection->sendErrorFmt("the received message-type was unknown '{}'", msgtype);
                return;
            }
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
            // when connecting to a peer, ask if for its peer
            conn->send(R"({"message":"summary", "message-type":"request"})");
        });
}

void MinerApp::run()
{
    _rewardAddress= _settings->value("mining.miner.public_key", "");
    if (_rewardAddress.empty() || _rewardAddress == "<CHANGE ME>")
    {
        _logger->critical("the setting 'mining.miner.public_key' in the config file must be updated");
        return;
    }

    initRest();
    initWebSocket();
    initPeers();

    _database->initialize(*_blockchain,
        [this]() -> Block
        {
            Transactions txs;
            txs.push_back(ash::CreateCoinbaseTransaction(0, _rewardAddress));
            Block gen{ 0, "", std::move(txs) };

            std::time_t t = std::time(nullptr);
            const auto gendata = fmt::format("{} {:%Y-%m-%d %H:%M:%S %Z}.", GENESIS_BLOCK, *std::localtime(&t));
            gen.setData(gendata);
            _logger->debug("creating gensis block with data '{}'", gendata);

            return gen;
        });

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

        const auto localUrl = fmt::format("http://{}:{}", address, _httpServer.config.port);
        utils::openBrowser(localUrl);
    }

    while (!_done)
    {
        std::this_thread::yield();
    }
}

void MinerApp::runMineThread()
{
    std::uint64_t index = 0;

    auto keepMiningCallback = 
        [this](std::uint64_t index) -> bool
        {
            std::lock_guard<std::mutex> lock{_chainMutex};
            return !_tempchain
                || _tempchain->size() == 0
                || _tempchain->back().index() < index;
        };

    while (!_miningDone && !_done)
    {
        std::unique_ptr<Block> newblock;

        std::string prevHash;
        std::string data;

        {
            std::lock_guard<std::mutex> lock{_chainMutex};

            auto newDifficulty = _blockchain->getAdjustedDifficulty();
            _miner.setDifficulty(newDifficulty);

            index = _blockchain->size();

            const auto& lastBlock = _blockchain->back();
            prevHash = lastBlock.hash();

            Transactions txs;
            txs.push_back(ash::CreateCoinbaseTransaction(index, _rewardAddress));

            newblock = std::make_unique<Block>(index, prevHash, std::move(txs));
            newblock->setMiner(_uuid);
        }
    
        _logger->debug("mining block #{} with difficulty {}", 
            index, _miner.difficulty());

        auto result = _miner.mineBlock(*newblock, keepMiningCallback);

        if (result == Miner::ABORT)
        {
            _logger->debug("mining block #{} was aborted", index);
            syncBlockchain();
            continue;
        }

        // TODO: need better locking/syncing on the chain from 
        //       point on

        // append the block to the chain
        if (!_blockchain->addNewBlock(*newblock))
        {
            _logger->error("could not add new block #{} to blockchain, stopping mining", newblock->index());
            _miningDone = true;
            break;
        }

        // write the block to the database
        _database->write(*newblock);

        // see if there's an update waiting for the local
        // copy of the chain
        if (!syncBlockchain())
        {
            // let the network know about our new coin
            broadcastNewBlock(*newblock);
        }
    }
}

void MinerApp::broadcastNewBlock(const Block& block)
{
    std::lock_guard<std::mutex> lock{_chainMutex};

    nl::json msg;
    msg["message"] = "newblock";
    msg["message-type"] = "request";
    msg["block"] = _blockchain->back();
    msg["cumdiff"] = _blockchain->cumDifficulty();
    _peers.broadcast(msg.dump());
}

// the blockchain is synced at startup and
// after each block is mined. returns 'true'
// if the local blockchain was modified by 
// the temp blockchain
bool MinerApp::syncBlockchain()
{
    bool retval = false;
    if (std::lock_guard<std::mutex> lock{_chainMutex}; 
        _tempchain)
    {
        if (_tempchain->front().index() == 0)
        {
            // we're replacing the full chain
            _blockchain.swap(_tempchain);
            _database->reset();
            _database->writeChain(*_blockchain);
            retval = true;
        }
        else if (_tempchain->front().index() <= _blockchain->back().index())
        {
            auto startIdx = _tempchain->front().index();
            _blockchain->resize(startIdx);
            for (const auto& block : *_tempchain)
            {
                // add up until a point of failure (if there
                // is one)
                if (!_blockchain->addNewBlock(block))
                {
                    _logger->warn("failed to add block while updating chain at index");
                }
            }

            _database->reset();
            _database->writeChain(*_blockchain);
            retval = true;
        }
        else if (_tempchain->front().index() == _blockchain->back().index() + 1)
        {
            for (const auto& block : *_tempchain)
            {
                if (_blockchain->addNewBlock(block))
                {
                    _database->write(block);
                }
            }
            retval = true;
        }
        else
        {
            _logger->warn("temp chain is too far ahead with blocks {}-{} and local chain {}-{}",
                _tempchain->front().index(), _tempchain->back().index(),
                _blockchain->front().index(), _blockchain->back().index());
        }

        _tempchain.reset();
    }
    
    return retval;
}

// handle requests in which WE are the SERVER
void MinerApp::dispatchRequest(HcConnectionPtr connection, const nl::json& json)
{
    const auto message = json["message"].get<std::string>();

    nl::json jresponse;
    if (message == "summary")
    {
        jresponse["blocks"].push_back(_blockchain->front());
        jresponse["blocks"].push_back(_blockchain->back());
        jresponse["cumdiff"] = _blockchain->cumDifficulty();
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
            auto id1 = json["id1"].get<std::uint64_t>();
            auto id2 = json["id2"].get<std::uint64_t>();

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
    else if (message == "newblock")
    {
        std::lock_guard<std::mutex> _lock(_chainMutex);

        const auto newblock = json["block"].get<Block>();
        auto remote_cumdiff = json["cumdiff"].get<std::uint64_t>();
        auto local_cumdiff = _blockchain->cumDifficulty();

        _logger->trace("received 'newblock' message with block #{} and cumulative diff of {}",
            newblock.index(), remote_cumdiff);

        if (remote_cumdiff > local_cumdiff
            || (remote_cumdiff == local_cumdiff && newblock.index() > _blockchain->back().index()))
        {
            // TODO: it would probably be best here to check if `newblock` is the next
            // in our chain and add it to our tempchain

            // get a summary from the machine that sent us this longer chain
            connection->sendRequest("summary");
            return;
        }
    }
    else
    {
        _logger->warn("unknown message '{}' sent from {}", connection->address(), message);
        return;
    }

    connection->sendResponse(message, jresponse.dump());
}

// handle responses form where WE were the CLIENT
void MinerApp::handleResponse(HcConnectionPtr connection, const nl::json& json)
{
    const auto message = json["message"].get<std::string>();

    if (message == "summary")
    {
        if (!json.contains("blocks")
            || !json["blocks"].is_array()
            || json["blocks"].size() != 2
            || !json.contains("cumdiff"))
        {
            _logger->warn("malformed wsc:/chain 'summary' response on connection {}", 
                static_cast<void*>(connection.get()));
            return;
        }

        const auto& remote_gen = json["blocks"].at(0).get<ash::Block>();
        const auto& remote_last = json["blocks"].at(1).get<ash::Block>();

        auto local_cumdiff = _blockchain->cumDifficulty();
        auto remote_cumdiff = json["cumdiff"].get<std::uint64_t>();
        
        // TODO: we're using the 'summary' command to determine
        // if we need to replace/update the chain, but we only
        // do those checks in 'summary'. We should do the thing
        // in the 'chain' command.
        const auto& genesis = _tempchain ? _tempchain->front() : _blockchain->front();
        const auto& lastblock = _tempchain ? _tempchain->back() : _blockchain->back();

        if (genesis != remote_gen)
        {
            _logger->warn("wsc:/chain 'summary' returned unknown chain on connection {}", 
                static_cast<void*>(connection.get()));

            if (_settings->value("chain.reset.enable", false))
            {
                _logger->info("requesting full remote chain");
                connection->sendRequest("chain");
            }
        }
        else if (local_cumdiff < remote_cumdiff)
        {
            auto startIdx = lastblock.index() + 1;
            auto stopIdx = remote_last.index();

            _logger->info("remote chain has a greater cumulative difficulty ({}) than local chain ({}), requesting #{}-#{}",
                remote_cumdiff, local_cumdiff, startIdx, stopIdx);

            connection->sendRequestFmt("chain", R"({{ "id1":{},"id2":{} }})", startIdx, stopIdx);
        }
        else
        {
            _logger->info("local blockchain up to date with connection {}",
                static_cast<void*>(connection.get()));
        }
    }
    else if (message == "chain")
    {
        if (const auto tempchain = json["blocks"].get<ash::Blockchain>();
                tempchain.size() <= 0 || !tempchain.isValidChain())
        {
            _logger->info("received invalid chain from connection {}", 
                static_cast<void*>(connection.get()));

            return;
        }
        else
        {
            std::lock_guard<std::mutex> lock(_chainMutex);
            handleChainResponse(connection, tempchain);
        }        
    }

    if (this->_miningDone)
    {
        syncBlockchain();
    }
}

void MinerApp::handleChainResponse(HcConnectionPtr connection, const Blockchain& tempchain)
{
    if (tempchain.front().index() == 0)
    {
        _logger->info("queuing replacement for local chain with with blocks {}-{}",
            tempchain.front().index(), tempchain.back().index());

        _tempchain = std::make_unique<ash::Blockchain>();
        *_tempchain = std::move(tempchain);
    }
    else if (tempchain.front().index() > _blockchain->back().index() + 1)
    {
        // we have a gap, so request the blocks in between
        auto startIdx = _blockchain->back().index() + 1;
        auto stopIdx = tempchain.back().index();

        _logger->info("temp chain has gap, requesting remote blocks {}-{}", startIdx, stopIdx);
        connection->sendRequestFmt("chain", R"({{ "message":"chain","id1":{},"id2":{} }})", startIdx, stopIdx);
    }
    else
    {
        // at this point the first block of the temp chain exists within our
        // local chain or is just ONE ahead, but we don't know if that first 
        // block is valid, and if it is not then we will have to walk backwards 
        // by requesting one more block from the remote chain

        auto tempStartIdx = tempchain.front().index() - 1;
        assert(tempStartIdx < _blockchain->size() && tempStartIdx > 0);

        if (tempchain.front().previousHash() == _blockchain->at(tempStartIdx).hash())
        {
            _logger->info("caching update for local chain with remote blocks {}-{}",
                tempchain.front().index(), tempchain.back().index());

            _tempchain = std::make_unique<ash::Blockchain>();
            *_tempchain = tempchain;
        }
        else
        {
            auto startIdx = tempchain.front().index() - 1;
            auto stopIdx = tempchain.back().index();
            
            _logger->debug("temp chain is misaligned, requesting remote blocks {}-{}", startIdx, stopIdx);
            connection->sendRequestFmt("chain", R"x({{ "id1":{},"id2":{} }})x", startIdx, stopIdx);
        }
    }
}

void MinerApp::handleError(HcConnectionPtr connection, const nl::json& json)
{
    _logger->debug("node {} reported an 'error' message: {}", 
        connection->address(), json["message"].get<std::string>());
}

} // namespace
