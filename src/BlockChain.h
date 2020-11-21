#pragma once

#include <cstdint>
#include <vector>

#include "Settings.h"
#include "Block.h"

namespace ash
{

class Blockchain 
{
public:
    Blockchain(ash::SettingsPtr settings);

    void AddBlock(Block bNew);

    std::size_t size() const 
    { 
        return _vChain.size(); 
    }

    const Block& getBlockByIndex(std::size_t idx) 
    { 
        return _vChain.at(idx); 
    }

    bool isValidBlockPair(std::size_t idx) const;
    bool isValidChain() const;

private:
    ash::SettingsPtr    _settings;
    std::uint32_t       _difficulty;
    std::vector<Block>  _vChain;
};

}