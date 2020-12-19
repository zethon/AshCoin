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
                if (!peer.second.connection)
                {
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
    const auto endpoint = fmt::format("{}/chain", peer);
    
    if (_peers[peer].client)
    {
        _peers[peer].client->stop();
    }

    if (_peers[peer].worker.joinable())
    {
        _peers[peer].worker.join();
    }

    auto client = std::make_shared<WsClient>(endpoint);
    _peers[peer].client = client;

    client->on_open =
        [this, client, peer = peer](WsClientConnPtr connection)
        {
            std::lock_guard<std::mutex> lock{this->_peerMutex};
            _logger->trace("wsc:/chain opened connection to node {}", peer);
            _peers[peer].connection = connection;
            
            if (_connectCallback)
            {
                _connectCallback(connection);
            }
        };

    client->on_error =
        [this, peer = peer](WsClientConnPtr connection, const SimpleWeb::error_code &ec)
        {
            // find the connection in our map
            auto entry = std::find_if(_peers.begin(), _peers.end(),
                [connection](const auto& el)
                {
                    return el.second.connection == connection;
                });

            // if the machine did not connect at startup then it will
            // not have a connection entry in the map
            if (entry != _peers.end())
            {
                entry->second.connection.reset();
            }

            _peers[peer].client->stop();
            _peers[peer].client.reset();
        };

    client->on_close = 
        [this](WsClientConnPtr connection, int, const std::string&)
        {
            std::lock_guard<std::mutex> lock{this->_peerMutex};

            // find the connection in our map
            auto entry = std::find_if(_peers.begin(), _peers.end(),
                [connection](const auto& el)
                {
                    return el.second.connection == connection;
                });

            if (entry == _peers.end())
            {
                _logger->error("closing connection that is not in peer map!");
                return;
            }

            _logger->trace("wsc:/chain closed connection to node {}", entry->first);
            entry->second.connection.reset();
        };

    client->on_message =
        [this](WsClientConnPtr connection, std::shared_ptr<WsClient::InMessage> message)
        {
            // this->onChainResponse(connection, message->string());
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
