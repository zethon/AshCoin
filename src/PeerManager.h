#pragma once
#include <string_view>
#include <set>
#include <functional>

#include <boost/signals2.hpp>
#include <boost/asio.hpp>

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

class PeerManager;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsServerConnPtr = std::shared_ptr<WsServer::Connection>;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using WsClientPtr = std::shared_ptr<WsClient>;
using WsClientConnPtr = std::shared_ptr<WsClient::Connection>;

struct PeerData
{
    WsClientPtr     client;
    WsClientConnPtr connection;
    std::thread     worker;
};

using PeerMap = std::map<std::string, PeerData>;

class ReconnectWorker
{

    boost::asio::io_service         _statIoService;
    boost::asio::deadline_timer     _statTimer { _statIoService };
    std::atomic_bool                _shutdown = false;
    
    std::chrono::milliseconds       _timeout;
    
    using Callback = std::function<void()>;
    Callback                        _callback;

public:
    ReconnectWorker(std::size_t timeout, Callback f)
        : _timeout { std::chrono::milliseconds(timeout) },
          _callback { f }
    {
        // nothing to do
    }

    void shutdown() { _shutdown = true; }

    void run()
    {
        _statIoService.reset();

        _statTimer.expires_from_now(boost::posix_time::milliseconds(_timeout.count()));
        _statTimer.async_wait(
            [this](const boost::system::error_code& ec)
            {
                this->privateRun(ec);
            });

        _statIoService.run();
    }

private:
    void privateRun(const boost::system::error_code& ec)
    {
        if (!ec && !_shutdown)
        {
            auto start = std::chrono::steady_clock::now();
            _callback();
            auto stop = std::chrono::steady_clock::now();

            auto elapsed = stop - start;
            std::chrono::milliseconds timeToWait 
                = std::chrono::duration_cast<std::chrono::milliseconds>(_timeout - elapsed);

            _statTimer.expires_from_now(boost::posix_time::milliseconds(timeToWait.count()));
            _statTimer.async_wait(
                [this](const boost::system::error_code& ec)
                {
                    this->privateRun(ec);
                });
        }
    }
};

class PeerManager
    : std::enable_shared_from_this<PeerManager>
{
public:
    using ConnectCallback = std::function<void(WsClientConnPtr)>;

    PeerManager();
    ~PeerManager();

    void loadPeers(std::string_view filename);
    void savePeers(std::string_view filename);

    void connectAll(std::function<void(WsClientConnPtr)> cb);
    void broadcast(std::string_view message);

    void initWebSocketServer(std::uint32_t port);

    boost::signals2::signal<void(WsServerConnPtr, const std::string&)> onChainRequest;
    boost::signals2::signal<void(WsClientConnPtr, const std::string&)> onChainResponse;

private:
    void createClient(const std::string& endpoint);

    PeerMap                             _peers;      
    std::mutex                          _peerMutex;

    std::unique_ptr<ReconnectWorker>    _reconnectWorker;
    std::thread                         _reconnectThread;
    ConnectCallback                     _connectCallback;

    SpdLogPtr                           _logger;

    WsServer                            _wsServer;
    std::thread                         _wsThread;
};

} // namespace ash
