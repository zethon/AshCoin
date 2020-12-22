#include <fstream>
#include <thread>

#include <boost/algorithm/string.hpp>

#include "PeerManager.h"

#ifdef _RELEASE
constexpr auto ConnectionRetryTimeout = 60u; // seconds
#else
constexpr auto ConnectionRetryTimeout = 10u; // seconds
#endif

namespace ash
{

PeerManager::PeerManager()
    : _logger(ash::initializeLogger("PeerManager"))
{
    _reconnectWorker = std::make_unique<ReconnectWorker>(
        ConnectionRetryTimeout * 1000u, [this]()
        {
            std::lock_guard<std::mutex> lock{ _peerMutex };
            for (const auto& peer : this->_peers)
            {
                if (peer.second.state == PeerData::State::OFFLINE)
                {
                    createClient(peer.first);
                }
            }
        });
}

PeerManager::~PeerManager()
{
    if (_reconnectWorker)
    {
        _reconnectWorker->shutdown();
    }

    if (_reconnectThread.joinable())
    {
        _reconnectThread.join();
    }

    for (auto&[peer, data] : _peers)
    {
        if (data.worker)
        {
            if (_peers.at(peer).client)
            {
                _peers.at(peer).client->stop();
            }
            
            if (data.worker->joinable())
            {
                data.worker->join();
            }
        }
    }

    if (_wsThread.joinable())
    {
        _logger->debug("wss:/chain shutting down");
        _wsServer.stop();
        _wsThread.join();
    }
}

void PeerManager::loadPeers(std::string_view filename)
{
    _logger->info("attempting to load peers file '{}'", filename);
    std::ifstream in(filename.data());

    if (!in)
    {
        _logger->warn("no peers file found at '{}'", filename);
        return;
    }

    std::lock_guard<std::mutex> lock{ _peerMutex };

    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty())
        {
            boost::algorithm::trim(line);
            _peers.insert_or_assign(line, PeerData{});
        }
    }
}

void PeerManager::createClient(const std::string& peer)
{
    using namespace std::chrono_literals;
    
    _logger->trace("attempting to connect to {}", peer);
    
    if (_peers[peer].worker
        && _peers[peer].worker->joinable())
    {
        _peers[peer].worker->join();
    }
    
    const auto endpoint = fmt::format("{}/chain", peer);
    _peers[peer].client = std::make_shared<WsClient>(endpoint);

#ifdef _RELEASE
    _peers[peer].client->config.timeout_request = 60; // seconds
#else
    _peers[peer].client->config.timeout_request = 10; // seconds
#endif

    _peers[peer].state = PeerData::State::CONNECTING;

    _peers[peer].client->on_open =
        [this, peer = peer](WsClientConnPtr connection)
        {
            std::lock_guard<std::mutex> lock{this->_peerMutex};
            _logger->trace("wsc:/chain opened connection to node {}", peer);
            assert(this->_peers.find(peer) != this->_peers.end());

            _peers[peer].connection.reset();
            _peers[peer].connection = connection;
            _peers[peer].state = PeerData::State::CONNECTED;
            
            if (_connectCallback)
            {
                _connectCallback(connection);
            }
        };

    _peers[peer].client->on_error =
        [this, peer = peer](WsClientConnPtr connection, const SimpleWeb::error_code &ec)
        {
            std::lock_guard<std::mutex> lock{ this->_peerMutex };
            _logger->trace("wsc:/chain error on peer {}: {}", peer, ec.message());

            assert(this->_peers.find(peer) != this->_peers.end());

            _peers[peer].client->stop();
            _peers[peer].state = PeerData::State::OFFLINE;
        };

    _peers[peer].client->on_close = 
        [this, peer=peer](WsClientConnPtr connection, int status, const std::string& reason)
        {
            std::lock_guard<std::mutex> lock{this->_peerMutex};
            _logger->trace("wsc:/chain closing peer {}: ({}) {}", 
                peer, status, reason);

            assert(this->_peers.find(peer) != this->_peers.end());

            _peers[peer].client->stop();
            _peers[peer].state = PeerData::State::OFFLINE;
        };

    _peers[peer].client->on_message =
        [this](WsClientConnPtr connection, std::shared_ptr<WsClient::InMessage> message)
        {
            auto conn = std::make_shared<ConnectionProxy>(connection);
            this->onChainMessage(conn, message->string());
        };

    _peers[peer].worker = std::make_unique<std::thread>(
        [this, peer = peer]() 
        {
            _peers[peer].client->start();
        });
}

void PeerManager::connectAll(std::function<void(WsClientConnPtr)> cb)
{
    _connectCallback = cb;

    std::lock_guard<std::mutex> lock{_peerMutex};
    for (const auto& [peer, data] : _peers)
    {
        createClient(peer);
    }

    _reconnectThread = std::thread(
        [this]()
        {
            this->_reconnectWorker->run();
        });
}

void PeerManager::broadcast(std::string_view message)
{
    for (const auto& [peer, data] : _peers)
    {
        if (data.state == PeerData::State::CONNECTED)
        {
            assert(data.connection);
            data.connection->send(message);
        }
    }
}

void PeerManager::initWebSocketServer(std::uint32_t port)
{
    _wsServer.config.port = port;
    _wsServer.endpoint["^/chain$"].on_open = 
        [this](WsServerConnPtr connection) 
        {
            _logger->trace("wss:/chain opened connection {}", static_cast<void*>(connection.get()));
        };

    _wsServer.endpoint["^/chain$"].on_close = 
        [this](WsServerConnPtr connection, int /*status*/, const std::string& /*reason*/) 
        {
            _logger->trace("wss:/chain closed connection {}", static_cast<void*>(connection.get()));
        };

    _wsServer.endpoint["^/chain$"].on_message = 
        [this](WsServerConnPtr connection, std::shared_ptr<WsServer::InMessage> message)
        {
            auto conn = std::make_shared<ConnectionProxy>(connection);
            this->onChainMessage(conn, message->string());
        };

    _wsThread = std::thread(
        [this]()
        {
            _logger->info("websocket server listening on port {}", _wsServer.config.port);
            _wsServer.start();
        });
}

} // namespace ash
