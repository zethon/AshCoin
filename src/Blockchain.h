#pragma once

#include <cstdint>
#include <vector>
#include <queue>

#include "Transactions.h"
#include "Settings.h"
#include "Block.h"
#include "AshLogger.h"

namespace ash
{

#ifdef _RELEASE
constexpr auto TARGET_TIMESPAN = 10u * 60u; // in seconds
constexpr auto TARGET_SPACING = 25u; // in blocks
#else
constexpr auto TARGET_TIMESPAN  = 60u; // in seconds
constexpr auto BLOCK_INTERVAL   = 10u; // in blocks
#endif

class Blockchain;
using BlockChainPtr = std::unique_ptr<Blockchain>;

void to_json(nl::json& j, const Blockchain& b);
void from_json(const nl::json& j, Blockchain& b);

UnspentTxOuts GetUnspentTxOuts(const Block& block);
UnspentTxOuts GetUnspentTxOuts(const Blockchain& chain);

//! This class is not thread safe and assumes that the
//  client handles synchronization
class Blockchain final
{
    std::vector<Block>  _blocks;
    
    std::set<std::size_t>       _txInHashes;
    UnspentTxOuts               _unspentTxOuts;
    std::queue<Transaction>     _txQueue; // transactions waiting to be mined by this miner
    
    SpdLogPtr           _logger;

    friend class ChainDatabase;
    friend void to_json(nl::json& j, const Blockchain& b);
    friend void from_json(const nl::json& j, Blockchain& b);

public:

    Blockchain();
    Blockchain(const Blockchain&)
    {
        throw std::runtime_error("not implemented");
    }
    Blockchain& operator=(const Blockchain&)
    {
        throw std::runtime_error("not implemented");
    }

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

    bool isValidBlockPair(std::size_t idx) const;
    bool isValidChain() const;

    std::uint64_t cumDifficulty() const
    {
        return cumDifficulty(_blocks.size() - 1);
    }

    std::uint64_t cumDifficulty(std::size_t idx) const;

    std::uint64_t getAdjustedDifficulty()
    {
        const auto chainsize = size();
        assert(chainsize > 0);

        if (((back().index() + 1) % BLOCK_INTERVAL) != 0)
        {
            return back().difficulty();
        }

        const auto& firstBlock = at(size() - BLOCK_INTERVAL);
        const auto& lastBlock = back();
        const auto timespan = 
            std::chrono::duration_cast<std::chrono::seconds>
                (lastBlock.time() - firstBlock.time());

        if (timespan.count() < (TARGET_TIMESPAN / 2))
        {
            return lastBlock.difficulty() + 1;
        }
        else if (timespan.count() > (TARGET_TIMESPAN * 2))
        {
            return lastBlock.difficulty() - 1;
        }

        return lastBlock.difficulty();
    }

    // TODO: this is called in at least one place in MinerApp, 
    // but ideally this should be a private function that is 
    // called internally when needed
    void updateUnspentTxOuts();
    
    const UnspentTxOuts& unspentTransactionOuts() const { return _unspentTxOuts; }
    UnspentTxOuts getUnspentTxOuts(std::string_view address);

    bool createTransaction(std::string_view receiver, double amount, std::string_view privateKey);

    void getTransactionsToBeMined(Block& block);
    std::size_t reQueueTransactions(Block& block);
};

}
