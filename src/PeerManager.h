#pragma once
#include <string_view>
#include <set>

#if _WINDOWS
#pragma warning(push)
#pragma warning(disable:4267)
#endif
#include <simple-websocket-server/client_ws.hpp>
#if _WINDOWS
#pragma warning(pop)
#endif

namespace ash
{

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

class PeerManager
{
    std::set<std::string>   _peers;

public:
    void loadPeers(std::string_view filename);
    void savePeers(std::string_view filename);

    void connectAll();
    void broadcast(std::string_view message);
};

} // namespace ash
