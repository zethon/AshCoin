#include <fstream>

#include <boost/algorithm/string.hpp>

#include "PeerManager.h"

namespace ash
{

PeerManager::PeerManager()
    : _logger(ash::initializeLogger("PeerManager"))
{
}

PeerManager::~PeerManager()
{
    _reconnectWorker.shutdown();
    _reconnectThread.join();

    for (auto&[peer, data] : _peers)
    {
        if (data.worker.joinable())
        {
            _peers.at(peer).client->stop();
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
    
void PeerManager::savePeers(std::string_view filename)
{
}
    
void PeerManager::connectAll(std::function<void(WsClientConnPtr)> cb)
{
    for (const auto& [peer, data] : _peers)
    {
        const auto endpoint = fmt::format("{}/chain", peer);
        _logger->debug("connecting to {}", endpoint);

        auto client = std::make_shared<WsClient>(endpoint);
        _peers[peer].client = client;

        client->on_open =
            [this, client, peer = peer, cb = cb](WsClientConnPtr connection)
            {
                _logger->trace("wsc:/chain opened connection {}", static_cast<void*>(connection.get()));
                _peers[peer].connection = connection;
                if (cb)
                {
                    cb(connection);
                }
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
                this->onChainResponse(connection, message->string());
            };

        auto thread = std::thread(
            [client]() 
            {
                client->start();
            });

        _peers[peer].worker = std::move(thread);
    }

    _reconnectThread = std::thread(
        [this]()
        {
            this->_reconnectWorker.run();
        });
}
    
void PeerManager::broadcast(std::string_view message)
{
    for (const auto& [peer, data] : _peers)
    {
        if (!data.connection) continue;
        _logger->trace("broadcasting to {}: {}", peer, message);
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
