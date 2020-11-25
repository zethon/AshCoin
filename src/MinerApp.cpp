#include <charconv>

#include <nlohmann/json.hpp>

#include "utils.h"
#include "MinerApp.h"

namespace nl = nlohmann;

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

void MinerApp::printIndex(HttpResponsePtr response)
{
    std::stringstream out;
    out << "<html><body>";
    out << "<h3>mining status: <b>"
        << (_miningDone ? "stopped" : "started")
        << "</b></h3>";

    out << "<h3>blockchain size: <b>"
        << "<a href='/block-idx/" 
            << (_blockchain->size() - 1) 
            << "'>" 
            << (_blockchain->size() - 1) 
            << "</a>"
        << "</b></h3>";

    if (_miningDone)
    {
        out << "<a href='/startMining'>start mining</a><br/>";
    }
    else
    {
        out << "<a href='/stopMining'>stop mining</a><br/>";
    }
    
    out << "<a href='/quit'>quit</a>";
   
    out << "</body></html>";
    response->write(out);
}

void MinerApp::initRest()
{
    _httpServer.config.port = _settings->value("rest.port", 27182);
    _httpServer.resource["^/$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->printIndex(response);
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
                nl::json json = _blockchain->getBlockByIndex(index);
                ss << "<pre>" << json.dump(4) << "</pre>";
                ss << "<br/>";
                if (index > 0) ss << "<a href='/block-idx/" << (index - 1) << "'>prev</a>&nbsp;";
                ss << "current: " << index;
                if (index < _blockchain->size()) ss << "&nbsp;<a href='/block-idx/" << (index + 1) << "'>next</a>&nbsp;";
            }
            
            response->write(ss);
        };

    _httpServer.resource["^/quit$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->signalExit();
            std::stringstream stream;
            stream << "<html><body><h1>Shutdown requested</h1></body></html>";
            response->write(stream);
        };

    _httpServer.resource["^/stopMining$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->stopMining();
            std::stringstream stream;
            stream << "<html><body><h1>Mining shutdown requested</h1></body></html>";
            response->write(stream);

            if (this->_mineThread.joinable())
            {
                this->_mineThread.join();
            }
        };

    _httpServer.resource["^/startMining$"]["GET"] = 
        [this](std::shared_ptr<HttpResponse> response, std::shared_ptr<HttpRequest> request) 
        {
            this->_miningDone = false;
            this->_mineThread = std::thread(&MinerApp::runMineThread, this);
            std::stringstream stream;
            stream << "<html><body><h1>Mining startup requested</h1></body></html>";
            response->write(stream);
        };
}

void MinerApp::run()
{
    _httpThread = std::thread(
        [this]()
        {
            _httpServer.start();
        });

    if (_settings->value("mining.autostart", false))
    {
        _mineThread = std::thread(&MinerApp::runMineThread, this);
    }
    else
    {
        _miningDone = true;
    }
    
    if (_settings->value("rest.autoload", false))
    {
        const auto address = _httpServer.config.address.empty() ? 
            "localhost" : _httpServer.config.address;

        const auto localUrl = fmt::format("http://{}:{}",
            address, _httpServer.config.port);
        utils::openBrowser(localUrl);
    }

    while (!_done)
    {
        std::this_thread::yield();
    }
}

void MinerApp::runMineThread()
{
    auto index = _blockchain->size();
    while (!_miningDone && !_done)
    {
        std::cout << fmt::format("Mining block {}", index) << '\n';
        const std::string data = fmt::format(":coinbase", index);

        auto newblock = 
            ash::Block(static_cast<std::uint32_t>(index++), data);

        _blockchain->AddBlock(newblock); // does mining
        _database->write(newblock);
    }
}

} // namespace
