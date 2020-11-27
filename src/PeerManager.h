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
using WsClientPtr = std::shared_ptr<WsClient>;
using WsClientConnPtr = std::shared_ptr<WsClient::Connection>;

using PeerMap = std::map<std::string, WsClientPtr>;
using ConnectionMap = std::map<std::string, WsClientConnPtr>;

class PeerManager
{
    std::set<std::string>   _peers;
    PeerMap                 _peerMap;
    ConnectionMap           _connections;    

public:
    void loadPeers(std::string_view filename);
    void savePeers(std::string_view filename);

    void connectAll();
    void broadcast(std::string_view message);
};

} // namespace ash
