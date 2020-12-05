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

    friend void to_json(nl::json& j, const Blockchain& b);
    friend void from_json(const nl::json& j, Blockchain& b);

public:
    Blockchain() = default;

    auto begin() const -> decltype(_blocks.begin())
    {
        return _blocks.begin();
    }

    auto end() const -> decltype(_blocks.end())
    {
        return _blocks.end();
    }

    auto front() const -> decltype(_blocks.front())
    {
        return _blocks.front();
    }

    auto back() const -> decltype(_blocks.back())
    {
        return _blocks.back();
    }

    std::size_t size() const 
    { 
        return _blocks.size(); 
    }

    void clear()
    {
        _blocks.clear();
    }

    void resize(std::size_t size)
    {
        _blocks.resize(size);
    }

    auto at(std::size_t index) -> decltype(_blocks.at(index))
    {
        return _blocks.at(index);
    }

    bool addNewBlock(const Block& block);
    bool addNewBlock(const Block& block, bool checkPreviousBlock);

    const Block& getBlockByIndex(std::size_t idx) 
    { 
        return _blocks.at(idx); 
    }

    bool isValidBlockPair(std::size_t idx) const;
    bool isValidChain() const;

    std::uint64_t cumDifficulty() const
    {
        return cumDifficulty(_blocks.size() - 1);
    }

    std::uint64_t cumDifficulty(std::size_t idx) const;
};

}
