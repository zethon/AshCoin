#include <fstream>

#include "PeerManager.h"

namespace ash
{

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
        auto client = std::make_shared<WsClient>(peer);
        client->on_open =
            [this, peer = peer](WsClientConnPtr connection)
            {
                _connections.insert_or_assign(peer, connection);
            };

        client->start();
        _peerMap.insert_or_assign(peer, client);
    }
}
    
void PeerManager::broadcast(std::string_view message)
{
}

} // namespace ash
