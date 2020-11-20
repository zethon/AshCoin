#pragma once

#include <cstdint>
#include <vector>
#include "Block.h"

namespace ash
{

constexpr auto DIFFICULTY = 10u;

class Blockchain 
{
public:
    Blockchain();

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
    std::vector<Block> _vChain;
};

}