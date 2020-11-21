#pragma once

#include "BlockChain.h"
#include "ChainDatabase.h"
#include "Settings.h"

namespace ash
{

class MinerApp
{

public:
    MinerApp(SettingsPtr settings);

    void run();

private:
    ChainDatabasePtr    _database;
    BlockChainPtr       _blockchain;
    SettingsPtr         _settings;
};

} // namespace
