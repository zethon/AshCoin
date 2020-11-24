#include <charconv>

#include "MinerApp.h"

namespace ash
{

MinerApp::MinerApp(SettingsPtr settings)
    : _settings{ std::move(settings) },
      _httpThread{},
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

    if (_httpThread.joinable())
    {
        _httpServer.stop();
        _httpThread.join();
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

    _httpServer.resource["^/block-idx/([0-9]+)$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request)
        {
            const auto indexStr = request->path_match[1].str();
            int index = 0;
            auto result = 
                std::from_chars(indexStr.data(), indexStr.data() + indexStr.size(), index);

            std::stringstream ss;
            if (result.ec != std::errc() || index >= _blockchain->size())
            {
                ss << R"xx(<html><body><h2 stye="color:red">Invalid Block</h2></body></html>)xx";
            }
            else
            {
                ss << "<pre>" << _blockchain->getBlockByIndex(index) << "</pre>";
            }
            
            response->write(ss);
        };
}

void MinerApp::run()
{
    _httpThread = std::thread(
        [this]()
        {
            _httpServer.start();
        });

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
        const std::string data = fmt::format("Block {} Data", index);

        auto newblock = 
            ash::Block(static_cast<std::uint32_t>(index++), data);

        _blockchain->AddBlock(newblock);
        _database->write(newblock);

        std::this_thread::yield();
    }
}

} // namespace
