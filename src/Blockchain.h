#pragma once

#include <cstdint>
#include <vector>

#include "Settings.h"
#include "Block.h"

namespace ash
{

class Blockchain;
using BlockChainPtr = std::unique_ptr<Blockchain>;

void to_json(nl::json& j, const Blockchain& b);
void from_json(const nl::json& j, Blockchain& b);

class Blockchain 
{

friend class ChainDatabase;
    
    std::vector<Block>  _blocks;
    std::uint32_t       _difficulty;

    friend void to_json(nl::json& j, const Blockchain& b);
    friend void from_json(const nl::json& j, Blockchain& b);

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

    auto back() const -> decltype(_blocks.back())
    {
        return _blocks.back();
    }

    void AddBlock(Block& bNew);

    std::size_t size() const 
    { 
        return _blocks.size(); 
    }

    void clear()
    {
        _blocks.clear();
    }

    const Block& getBlockByIndex(std::size_t idx) 
    { 
        return _blocks.at(idx); 
    }

    bool isValidBlockPair(std::size_t idx) const;
    bool isValidChain() const;

    std::uint32_t difficulty() const { return _difficulty; }
    void setDifficulty(std::uint32_t v) { _difficulty = v; }
};

}
