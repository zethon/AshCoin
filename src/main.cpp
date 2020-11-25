#include <iostream>
#include <string_view>

#ifndef _WINDOWS
#include <signal.h>
#endif

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include "core.h"
#include "utils.h"
#include "Blockchain.h"
#include "Settings.h"
#include "MinerApp.h"

#include "AshDb.h"

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

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");
    std::cout << APP_TITLE << '\n';
    std::cout << COPYRIGHT << '\n';

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
    ash::MinerApp app{ std::move(settings) };
    app.run();

    return 0;
}