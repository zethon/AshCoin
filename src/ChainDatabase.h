#pragma once
#include <string_view>
#include <optional>
#include <iterator>
#include <cstddef>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <ashdb/ashdb.h>

#include <leveldb/db.h>
#include <leveldb/comparator.h>

#include "Block.h"
#include "Blockchain.h"
#include "AshLogger.h"

namespace ash
{

inline void write_data(std::ostream& stream, const TxOutPoint& pt)
{
    ashdb::ashdb_write(stream, pt.blockIndex);
    ashdb::ashdb_write(stream, pt.txIndex);
    ashdb::ashdb_write(stream, pt.txOutIndex);
}

inline void write_txin(std::ostream& stream, const TxIn& tx)
{
    ash::write_data(stream, tx.txOutPt());
    ashdb::ashdb_write(stream, tx.signature());
}

inline void write_txout(std::ostream& stream, const TxOut& tx)
{
    ashdb::ashdb_write(stream, tx.address());
    ashdb::ashdb_write(stream, tx.amount());
}

inline void write_tx(std::ostream& stream, const Transaction& tx)
{
    ashdb::ashdb_write(stream, tx.id());

    {
        const auto txins = tx.txIns();
        auto txinsize = static_cast<std::uint64_t>(txins.size());
        ashdb::ashdb_write<std::uint64_t>(stream, txinsize);
        for (const auto& txin : txins)
        {
            write_txin(stream, txin);
        }
    }

    {
        const auto txouts = tx.txOuts();
        auto txoutsize = static_cast<std::uint64_t>(txouts.size());
        ashdb::ashdb_write<std::uint64_t>(stream, txoutsize);
        for (const auto& txout : txouts)
        {
            write_txout(stream, txout);
        }
    }
}

inline void ashdb_write(std::ostream& stream, const Block& block)
{
    ashdb::ashdb_write<std::uint64_t>(stream, block.index());
    ashdb::ashdb_write<std::uint64_t>(stream, block.nonce());
    ashdb::ashdb_write<std::uint64_t>(stream, block.difficulty());
    ashdb::ashdb_write(stream, block.data());

    std::uint64_t dtime =
            static_cast<std::uint64_t>(block.time().time_since_epoch().count());
    ashdb::ashdb_write<std::uint64_t>(stream, dtime);

    ashdb::ashdb_write(stream, block.hash());
    ashdb::ashdb_write(stream, block.previousHash());
    ashdb::ashdb_write(stream, block.miner());

    const auto& txs = block.transactions();
    const auto txsize = static_cast<std::uint64_t>(txs.size());
    ashdb::ashdb_write<std::uint64_t>(stream, txsize);

    for (const auto& tx : txs)
    {
        write_tx(stream, tx);
    }
}

inline void read_data(std::istream& stream, TxOutPoint& pt)
{
    ashdb::ashdb_read(stream, pt.blockIndex);
    ashdb::ashdb_read(stream, pt.txIndex);
    ashdb::ashdb_read(stream, pt.txOutIndex);
}

inline void read_txin(std::istream& stream, TxIn& txin)
{
    ash::read_data(stream, txin.txOutPt());
    ashdb::ashdb_read(stream, txin._signature);
}

inline void read_txout(std::istream& stream, TxOut& txout)
{
    ashdb::ashdb_read(stream, txout._address);
    ashdb::ashdb_read(stream, txout._amount);
}

inline void read_tx(std::istream& stream, Transaction& tx)
{
    ashdb::ashdb_read(stream, tx._id);

    {
        std::uint64_t txincount;
        ashdb::ashdb_read(stream, txincount);
        auto& txins = tx.txIns();
        for (std::uint64_t x = 0; x < txincount; x++)
        {
            TxIn txin;
            read_txin(stream, txin);
            txins.push_back(txin);
        }
    }

    {
        std::uint64_t txoutcount;
        ashdb::ashdb_read(stream, txoutcount);
        auto& txouts = tx.txOuts();
        for (std::uint64_t x = 0; x < txoutcount; x++)
        {
            TxOut txout;
            read_txout(stream, txout);
            txouts.push_back(txout);
        }
    }
}

inline void ashdb_read(std::istream& stream, Block& block)
{
    ashdb::ashdb_read(stream, block._hashed._index);
    ashdb::ashdb_read(stream, block._hashed._nonce);
    ashdb::ashdb_read(stream, block._hashed._difficulty);
    ashdb::ashdb_read(stream, block._hashed._data);

    std::uint64_t dtime;
    ashdb::ashdb_read(stream, dtime);
    block._hashed._time =
            BlockTime{std::chrono::milliseconds{dtime}};

    ashdb::ashdb_read(stream, block._hash);
    ashdb::ashdb_read(stream, block._hashed._prev);
    ashdb::ashdb_read(stream, block._miner);

    auto& txs = block.transactions();
    auto txcount = static_cast<std::uint64_t>(txs.size());
    ashdb::ashdb_read(stream, txcount);

    for (std::uint64_t x = 0; x < txcount; x++)
    {
        Transaction tx;
        read_tx(stream, tx);
        block.add_transaction(std::move(tx));
    }
}

class IChainDatabase;
using IChainDatabasePtr = std::unique_ptr<IChainDatabase>;

class AshChainDatabase;
using ChainDatabasePtr = std::unique_ptr<AshChainDatabase>;

class IChainDatabase
{

public:
    IChainDatabase() = default;
    virtual  ~IChainDatabase() = default;

    // TODO: this should not be in the base class
    using GenesisCallback = std::function<Block(void)>;
    virtual void initialize(Blockchain& chain) = 0;

    virtual bool opened() const = 0;

    virtual std::optional<Block> read(std::size_t index) const = 0;
    virtual std::size_t size() = 0;

    virtual void write(const Block& block) = 0;
    virtual void writeChain(const Blockchain& chain) = 0;
};

class AshChainDatabase final : public IChainDatabase
{

public:
    using DBType = ashdb::AshDB<Block>;

    AshChainDatabase(std::string_view folder);
    ~AshChainDatabase() override = default;

    void write(const Block& block) override;
    void writeChain(const Blockchain& chain) override;

    void initialize(Blockchain& chain) override;

    std::optional<Block> read(std::size_t index) const override;

    std::size_t size() override
    {
        return static_cast<std::size_t>(_db->size());
    }

    bool opened() const override
    {
        return _db->opened();
    }

private:
    std::uint64_t _size = 0;

    std::unique_ptr<DBType> _db;

    // TODO: fair game to be refactored
    boost::filesystem::path     _path;
    SpdLogPtr                   _logger;
};

} // namespace
