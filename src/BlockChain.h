#pragma once

#include <cstdint>
#include <vector>

#include "Settings.h"
#include "Block.h"

namespace ash
{

class Blockchain;
using BlockChainPtr = std::unique_ptr<Blockchain>;

class Blockchain 
{

friend class ChainDatabase;

std::uint32_t       _difficulty;
std::vector<Block>  _vChain;

public:
    Blockchain(std::uint32_t difficulty);

    auto begin() const -> decltype(_vChain.begin())
    {
        return _vChain.begin();
    }

    auto end() const -> decltype(_vChain.end())
    {
        return _vChain.end();
    }

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
};

}