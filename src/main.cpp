#include <iostream>
#include <string_view>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include "core.h"
#include "utils.h"
#include "BlockChain.h"
#include "Settings.h"

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
    retval->registerUInt("database.filesize.max", 1024u * 5u,
        std::make_shared<ash::RangeValidator<std::uint64_t>>(filesizeMin, filesizeMax));

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
        ("chain,n", po::value<std::string>(), "the block chain folder file")
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

    ash::Blockchain bChain{ std::move(settings) };

    std::cout << "Mining block 1..." << std::endl;
    bChain.AddBlock(ash::Block(1, "Block 1 Data"));

    std::cout << "Mining block 2..." << std::endl;
    bChain.AddBlock(ash::Block(2, "Block 2 Data"));

    std::cout << "Mining block 3..." << std::endl;
    bChain.AddBlock(ash::Block(3, "Block 3 Data"));

    return 0;
}