#include <fstream>

#include "PeerManager.h"

namespace ash
{

PeerManager::PeerManager()
    : _logger(ash::initializeLogger("PeerManager"))
{
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

    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty())
        {
            _peers.insert(line);
        }
    }
}
    
void PeerManager::savePeers(std::string_view filename)
{
}
    
void PeerManager::connectAll()
{
    for (const auto& peer : _peers)
    {
        const auto endpoint = fmt::format("{}/chain", peer);
        _logger->debug("connecting to {}", endpoint);
        auto client = std::make_shared<WsClient>(endpoint);

        client->on_open =
            [this, client, peer = peer](WsClientConnPtr connection)
            {
                _logger->trace("wsc:/chain opened connection {}", static_cast<void*>(connection.get()));
                _connections.insert_or_assign(peer, connection);
            };

        client->on_error =
            [this, peer = peer](WsClientConnPtr, const SimpleWeb::error_code &ec)
            {
                _logger->warn("could not connect to {} because: {}", peer, ec.message());
            };

        client->on_message =
            [this](WsClientConnPtr connection, std::shared_ptr<WsClient::InMessage> message)
            {
                _logger->trace("wsc:/chain message on connection {}", static_cast<void*>(connection.get()));
                this->onChainResponse(message->string());
            };

        _peerMap.insert_or_assign(peer, client);

        auto thread = std::thread(
            [client]() 
            {
                client->start();
            });

        _threadPool.insert_or_assign(peer, std::move(thread));
    }
}
    
void PeerManager::broadcast(std::string_view message)
{
    for (const auto& [peer, connection] : _connections)
    {
        _logger->trace("broadcasting to {}: {}", peer, message);
        connection->send(message);
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
            _logger->trace("wss:/chain request from connection {}", static_cast<void*>(connection.get()));
            this->onChainRequest(connection, message->string());
        };

    _wsThread = std::thread(
        [this]()
        {
            _logger->debug("websocket server listening on port {}", _wsServer.config.port);
            _wsServer.start();
        });
}

} // namespace ash
