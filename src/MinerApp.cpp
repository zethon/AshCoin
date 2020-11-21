#include "MinerApp.h"

namespace ash
{

MinerApp::MinerApp(SettingsPtr settings)
    : _settings{ std::move(settings) },
      _mineThread{}
{
    initRest();

    const std::string dbfolder = _settings->value("database.folder", "");
    _database = std::make_unique<ChainDatabase>(dbfolder);

    std::uint32_t difficulty = _settings->value("chain.difficulty", 5u);
    _blockchain = std::make_unique<Blockchain>(difficulty);

    _database->initialize(*_blockchain);
}

MinerApp::~MinerApp()
{
    if (_mineThread.joinable())
    {
        _mineThread.join();
    }
}

void MinerApp::initRest()
{
    _httpServer.config.port = _settings->value("rest.port", 27182);
    _httpServer.resource["^/$"]["GET"] = 
        [](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            std::stringstream stream;
            stream << "<h1>Request from " << request->remote_endpoint().address().to_string() << ":" << request->remote_endpoint().port() << "</h1>";

            stream << request->method << " " << request->path << " HTTP/" << request->http_version;

            stream << "<h2>Query Fields</h2>";
            auto query_fields = request->parse_query_string();
            for (auto &field : query_fields)
                stream << field.first << ": " << field.second << "<br>";

            stream << "<h2>Header Fields</h2>";
            for (auto &field : request->header)
                stream << field.first << ": " << field.second << "<br>";

            response->write(stream);
        };

    _httpServer.start();
}

void MinerApp::run()
{
    _mineThread = std::thread(&MinerApp::doMine, this);
    while (!_done)
    {
        std::this_thread::yield();
    }
}

void MinerApp::doMine()
{
    auto index = _blockchain->size();
    while (!_done)
    {
        std::cout << fmt::format("Mining block {}", index) << '\n';
        _blockchain->AddBlock(ash::Block(static_cast<std::uint32_t>(index++), "Block 3 Data"));
        std::this_thread::yield();
    }
}

} // namespace
