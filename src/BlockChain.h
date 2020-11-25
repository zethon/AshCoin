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
    
    std::vector<Block>  _blocks;
    std::uint32_t       _difficulty;

public:
    Blockchain(std::uint32_t difficulty);

    auto begin() const -> decltype(_blocks.begin())
    {
        return _blocks.begin();
    }

    auto end() const -> decltype(_blocks.end())
    {
        return _blocks.end();
    }

    void AddBlock(Block& bNew);

    std::size_t size() const 
    { 
        return _blocks.size(); 
    }

    const Block& getBlockByIndex(std::size_t idx) 
    { 
        return _blocks.at(idx); 
    }

    bool isValidBlockPair(std::size_t idx) const;
    bool isValidChain() const;
};

}
