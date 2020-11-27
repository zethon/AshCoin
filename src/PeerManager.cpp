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

    }
}
    
void PeerManager::broadcast(std::string_view message)
{
}

} // namespace ash
