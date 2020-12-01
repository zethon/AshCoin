#include <iostream>
#include <string_view>

#ifndef _WINDOWS
#include <signal.h>
#endif

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/common.h>

#include "AshLogger.h"
#include "core.h"
#include "utils.h"
#include "Blockchain.h"
#include "Settings.h"
#include "MinerApp.h"

namespace po = boost::program_options;

ash::SettingsPtr registerSettings()
{
    auto retval = std::make_unique<ash::Settings>();

    retval->registerUInt("chain.difficulty", 5,
        std::make_shared<ash::RangeValidator<std::uint16_t>>(1u, 256u));

    const std::string dbfolder = utils::getDefaultDatabaseFolder();
    retval->registerString("database.folder", dbfolder, 
        std::make_shared<ash::NotEmptyValidator>());

    constexpr auto filesizeMin = 1024u;
    constexpr auto filesizeMax = 1024u * 1024u * 1024u;
    constexpr auto filesizeDefault = 1024u * 1024u * 5u;
    retval->registerUInt("database.filesize.max", filesizeDefault,
        std::make_shared<ash::RangeValidator<std::uint64_t>>(filesizeMin, filesizeMax));

    retval->registerBool("mining.autostart", false);

    constexpr auto portMin = 1024u;
    constexpr auto portMax = 65535u;
    constexpr auto portDefault = ash::HTTPServerPortDefault;
    retval->registerUInt("rest.port", portDefault,
        std::make_shared<ash::RangeValidator<std::uint64_t>>(filesizeMin, filesizeMax));

    constexpr auto wsPortDefault = ash::WebSocketServerPorDefault;
    retval->registerUInt("websocket.port", wsPortDefault,
        std::make_shared<ash::RangeValidator<std::uint64_t>>(filesizeMin, filesizeMax));

    retval->registerBool("rest.autoload", false);

    retval->registerString("peers.file", utils::getDefaultPeersFile(),
        std::make_shared<ash::NotEmptyValidator>());

    // log settings
    retval->registerEnum("logs.level", "debug", 
        { "off", "critical", "err", "warn", "info", "debug", "trace" });

    retval->registerBool("logs.file.enabled", true);
    retval->registerString("logs.file.folder", dbfolder,
        std::make_shared<ash::NotEmptyValidator>());

    return std::move(retval);
}

ash::SettingsPtr initSettings(std::string_view filename)
{
    auto settings = registerSettings();
    boost::filesystem::path configFile{ filename.data() };
    if (boost::filesystem::exists(configFile))
    {
        settings->load(filename);
    }
    else
    {
        settings->save(filename);
    }

    return settings;
}

void initializeLogs(ash::SettingsPtr settings)
{
    const std::string levelstr = settings->value("logs.level", "info");
    const auto configLevel = spdlog::level::from_str(levelstr);

    auto logger = ash::rootLogger();
    logger->set_level(configLevel);

    std::string loggerfilename;

    if (settings->value("logs.file.enabled", false))
    {
        const auto logfolder = settings->value("logs.file.folder", "");
        if (logfolder.size() > 0)
        {
            if (!boost::filesystem::exists(logfolder))
            {
                boost::filesystem::create_directories(logfolder);
            }

            boost::filesystem::path folder { logfolder };
            loggerfilename = boost::filesystem::path(folder / "ash.log").generic_string();
            auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>
                (loggerfilename, 1024 * 1024 * 5, 3);

            logger->sinks().push_back(rotating);
        }
    }

    logger->info("starting {} by {}", APP_TITLE, COPYRIGHT);
    logger->info("built on {}", BUILDTIMESTAMP);
    if (loggerfilename.size() > 0)
    {
        logger->info("logfile written to {}" , loggerfilename);
    }
    logger->info("log level set to: {}", spdlog::level::to_string_view(configLevel));
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,?", "print help message")
        ("version,v", "print version string")
        ("config,c",po::value<std::string>(), "config file")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") > 0)
    {
        std::cout << desc << '\n';
        return 0;
    }

    std::string configFile = utils::getDefaultConfigFile();
    if (vm.count("config") > 0)
    {
        configFile = vm["config"].as<std::string>();
    }

    auto settings = initSettings(configFile);
    initializeLogs(settings);
    ash::rootLogger()->info("using setting file {}", configFile);

    ash::MinerApp app{ std::move(settings) };
    app.run();

    return 0;
}