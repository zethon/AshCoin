#pragma once
#include <string_view>
#include <set>

#include <boost/signals2.hpp>

#if _WINDOWS
#pragma warning(push)
#pragma warning(disable:4267)
#endif
#include <simple-websocket-server/server_ws.hpp>
#include <simple-websocket-server/client_ws.hpp>
#if _WINDOWS
#pragma warning(pop)
#endif

#include "AshLogger.h"

namespace ash
{

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsServerConnPtr = std::shared_ptr<WsServer::Connection>;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using WsClientPtr = std::shared_ptr<WsClient>;
using WsClientConnPtr = std::shared_ptr<WsClient::Connection>;

using PeerMap = std::map<std::string, WsClientPtr>;
using ConnectionMap = std::map<std::string, WsClientConnPtr>;

class PeerManager
{
    std::set<std::string>               _peers;
    PeerMap                             _peerMap;
    ConnectionMap                       _connections;
    std::map<std::string, std::thread>  _threadPool;
    SpdLogPtr                           _logger;

    WsServer                            _wsServer;
    std::thread                         _wsThread;

public:
    PeerManager();
    ~PeerManager();

    void loadPeers(std::string_view filename);
    void savePeers(std::string_view filename);

    void connectAll(std::function<void(WsClientConnPtr)> cb);
    void broadcast(std::string_view message);

    void initWebSocketServer(std::uint32_t port);

    boost::signals2::signal<void(WsServerConnPtr, const std::string&)> onChainRequest;
    boost::signals2::signal<void(WsClientConnPtr, const std::string&)> onChainResponse;
};

} // namespace ash
