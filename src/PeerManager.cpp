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
    std::ifstream in(filename.data());

    if (!in) return;

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
        const auto endpoint = fmt::format("{}/blocks", peer);
        _logger->debug("connecting to {}", endpoint);
        auto client = std::make_shared<WsClient>(endpoint);

        client->on_open =
            [this, client, peer = peer](WsClientConnPtr connection)
            {
                _connections.insert_or_assign(peer, connection);
            };

        client->on_error =
            [this, peer = peer](WsClientConnPtr /*connection*/, const SimpleWeb::error_code &ec)
            {
                _logger->warn("could not connect to {} because: {}", peer, ec.message());
            };

        client->on_message =
            [](WsClientConnPtr connection, std::shared_ptr<WsClient::InMessage> message)
            {
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

} // namespace ash
