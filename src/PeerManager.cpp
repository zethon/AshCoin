#include <fstream>

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
                    _logger->trace("attempting reconnect to {}", peer.first);
                    createClient(peer.first);
                }
            }
        });
}

PeerManager::~PeerManager()
{
    _reconnectWorker->shutdown();
    _reconnectThread.join();

    for (auto&[peer, data] : _peers)
    {
        if (data.worker.joinable())
        {
            if (_peers.at(peer).client)
            {
                _peers.at(peer).client->stop();
            }
            
            data.worker.join();
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

    const auto endpoint = fmt::format("{}/chain", peer);
    
    if (_peers[peer].worker.joinable())
    {
        _peers[peer].worker.join();
    }

    auto client = std::make_shared<WsClient>(endpoint);

    _peers[peer].client = client;
    _peers[peer].state = PeerData::State::CONNECTING;

    client->on_open =
        [this, client, peer = peer](WsClientConnPtr connection)
        {
            std::lock_guard<std::mutex> lock{this->_peerMutex};
            _logger->trace("wsc:/chain opened connection to node {}", peer);
            assert(this->_peers.find(peer) != this->_peers.end());

            _peers[peer].connection = connection;
            _peers[peer].state = PeerData::State::CONNECTED;
            
            if (_connectCallback)
            {
                _connectCallback(connection);
            }
        };

    client->on_error =
        [client, this, peer = peer](WsClientConnPtr connection, const SimpleWeb::error_code &ec)
        {
            std::lock_guard<std::mutex> lock{ this->_peerMutex };
            _logger->trace("wsc:/chain error on peer {}: {}", peer, ec.message());

            assert(this->_peers.find(peer) != this->_peers.end());
            assert(this->_peers[peer].client == client);

            _peers[peer].client->stop();
            _peers[peer].state = PeerData::State::OFFLINE;
        };

    client->on_close = 
        [this, client, peer=peer](WsClientConnPtr connection, int status, const std::string& reason)
        {
            std::lock_guard<std::mutex> lock{this->_peerMutex};
            _logger->trace("wsc:/chain closing peer {}: ({}) {}", 
                peer, status, reason);

            assert(this->_peers.find(peer) != this->_peers.end());
            assert(this->_peers[peer].client == client);

            _peers[peer].client->stop();
            _peers[peer].state = PeerData::State::OFFLINE;
        };

    client->on_message =
        [this](WsClientConnPtr connection, std::shared_ptr<WsClient::InMessage> message)
        {
            auto conn = std::make_shared<ConnectionProxy>(connection);
            this->onChainMessage(conn, message->string());
        };

    _peers[peer].client = client;

    auto thread = std::thread(
        [&peerdata = _peers[peer]]() 
        {
            peerdata.client->start();
        });

    _peers[peer].worker = std::move(thread);
}

void PeerManager::connectAll(std::function<void(WsClientConnPtr)> cb)
{
    _connectCallback = cb;

    std::lock_guard<std::mutex> lock{_peerMutex};
    for (const auto& [peer, data] : _peers)
    {
        _logger->debug("connecting to {}", peer);
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
        if (!data.connection) continue;
        data.connection->send(message);
    }
}

void PeerManager::initWebSocketServer(std::uint32_t port)
{
    _wsServer.config.port = port;
    _wsServer.endpoint["^/echo/?$"].on_open = 
        [this](WsServerConnPtr connection) 
        {
            _logger->trace("ws:/echo opened connection {}", static_cast<void*>(connection.get()));
        };

    _wsServer.endpoint["^/echo/?$"].on_message =
        [this](WsServerConnPtr connection, std::shared_ptr<WsServer::InMessage> in_message)
        {
            _logger->trace("ws:/echo received message on connection {}", static_cast<void*>(connection.get()));

            // connection->send is an asynchronous function (you can pass a lambda)
            auto out_message = in_message->string();
            connection->send(out_message);
        };

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
            // this->onChainRequest(connection, message->string());
            auto conn = std::make_shared<ConnectionProxy>(connection);
            this->onChainMessage(conn, message->string());

        };

    _wsThread = std::thread(
        [this]()
        {
            _logger->debug("websocket server listening on port {}", _wsServer.config.port);
            _wsServer.start();
        });
}

} // namespace ash
