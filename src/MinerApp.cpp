#include "MinerApp.h"

namespace ash
{

MinerApp::MinerApp(SettingsPtr settings)
    : _settings{ std::move(settings) }
{
    std::uint32_t difficulty = _settings->value("chain.difficulty", 5u);
    _blockchain = std::make_unique<Blockchain>(difficulty);

    const std::string dbfolder = _settings->value("database.folder", "");
    _database = std::make_unique<ChainDatabase>(dbfolder);
}

void MinerApp::run()
{
    std::cout << "Mining 1..." << std::endl;
    _blockchain->AddBlock(ash::Block(1, "Block 1 Data"));

    std::cout << "Mining 2..." << std::endl;
    _blockchain->AddBlock(ash::Block(2, "Block 2 Data"));

    std::cout << "Mining 3..." << std::endl;
    _blockchain->AddBlock(ash::Block(3, "Block 3 Data"));
}

} // namespace
