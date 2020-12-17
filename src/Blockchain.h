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

template<typename NumberT>
class CumulativeMovingAverage
{
    NumberT     _total = 0;
    std::size_t _count = 0;
public:
    float value() const
    {
        // TODO: pretty sure only one cast is need, but not 100% sure
        return (static_cast<float>(_total) / static_cast<float>(_count));
    }

    void addValue(NumberT v)
    {
        _total += v;
        _count++;
    }

    void reset()
    {
        _total = 0;
        _count = 0;
    }
};

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

    std::uint64_t getAdjustedDifficulty(std::uint64_t prevCount, std::uint64_t expectedTime)
    {
        CumulativeMovingAverage<std::uint64_t> avg;
        const auto chainsize = size();
        
        if (chainsize == 0)
        {
            return 1;
        }
        else if (chainsize == 1)
        {
            return back().difficulty();
        }
        else if (chainsize == 2)
        {
            avg.addValue(back().time() - front().time());
        }
        else 
        {
            auto startIdx = (chainsize <= prevCount) ? 1 : chainsize - prevCount;
            auto prevTime = at(startIdx-1).time();
            
            for (auto idx = startIdx; idx < chainsize; idx++)
            {
                avg.addValue(at(idx).time() - prevTime);
                prevTime = at(idx).time();
            }
        }

        if (const auto avgTime = avg.value();
            avgTime < (expectedTime * 0.5f))
        {
            return back().difficulty() + 1;
        }
        else if (avgTime > (expectedTime * 2.0f)
            && back().difficulty() > 1)
        {
            return back().difficulty() - 1;
        }

        return back().difficulty();
    }
};

}
