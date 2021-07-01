#pragma once

#include <cstdint>
#include <iostream>
#include <chrono>

#include <nlohmann/json.hpp>

#include "Transactions.h"
#include "AshLogger.h"

namespace nl = nlohmann;

namespace ash
{

class Block;
using BlockSharedPtr = std::shared_ptr<Block>;
using BlockUniquePtr = std::unique_ptr<Block>;
using BlockList = std::vector<Block>;

void to_json(nl::json& j, const Block& b);
void from_json(const nl::json& j, Block& b);

void to_json(nl::json& j, const BlockList& b);
void from_json(const nl::json& j, BlockList& b);

bool ValidHash(const Block& block);
bool ValidNewBlock(const Block& block, const Block& prevblock);

std::string CalculateBlockHash(const Block& block);
std::string CalculateBlockHash(
    std::uint64_t index, 
    std::uint64_t nonce, 
    std::uint64_t difficulty,
    BlockTime time,
    const std::string& data, 
    const std::string& previous,
    const std::string& extra);

class Block 
{
    friend void ashdb_read(std::istream& stream, Block& block);
    friend void from_json(const nl::json& j, Block& b);
    friend class Miner;

public:
    Block() = default;
    Block(std::uint64_t index, std::string_view prevHash)
        : Block(index, prevHash, {})
    {
        // nothing to do
    }

    Block(std::uint64_t index, std::string_view prevHash, Transactions&& tx);

    bool operator==(const Block& other) const;
    bool operator!=(const Block& other) const
    {
        return !(*this == other);
    }

    std::uint64_t index() const { return _hashed._index; }
    std::uint64_t nonce() const { return _hashed._nonce; }
    std::uint64_t difficulty() const { return _hashed._difficulty; }
    
    std::string data() const { return _hashed._data;  }
    void setData(std::string_view data) { _hashed._data = data; }

    BlockTime time() const { return _hashed._time; }
    std::string previousHash() const { return _hashed._prev; }

    const Transactions& transactions() const { return _hashed._txs; }
    void add_transaction(Transaction&& tx)
    {
        _hashed._txs.push_back(std::move(tx));
    }

    [[maybe_unused]] bool update_transaction(std::size_t index, const Transaction& tx)
    {
        if (index >= _hashed._txs.size())
        {
            return false;
        }

        _hashed._txs.at(index) = tx;
        return true;
    }

    std::string hash() const { return _hash; }

    std::string miner() const { return _miner; }
    void setMiner(std::string_view val) { _miner = val; }

    void setMinedData(std::uint64_t nonce, std::uint64_t diff, BlockTime time, std::string_view hash)
    {
        _hashed._nonce = nonce;
        _hashed._difficulty = diff;
        _hashed._time = time;
        _hash = hash;
    }

private:
    struct HashedData
    {
        std::uint64_t       _index;
        std::uint64_t       _nonce;
        std::uint64_t       _difficulty;
        std::string         _data;
        BlockTime           _time;
        std::string         _prev;
        Transactions        _txs;
    };

    HashedData      _hashed;

    std::string     _hash;
    std::string     _miner;
};

} // namespace ash

namespace std
{

 std::ostream& operator<<(std::ostream& os, const ash::Block& block);

} // namespace std