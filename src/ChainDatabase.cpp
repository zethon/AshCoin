#include "Transactions.h"
#include "Blockchain.h"
#include "ChainDatabase.h"

namespace ash
{

void write_data(std::ostream& stream, const TxOutPoint& pt)
{
    ash::db::write_data(stream, pt.blockIndex);
    ash::db::write_data(stream, pt.txIndex);
    ash::db::write_data(stream, pt.txOutIndex);
}

void write_data(std::ostream& stream, const TxIn& tx)
{
    ash::write_data(stream, tx.txOutPt());
    ash::db::write_data(stream, tx.signature());
}

void write_data(std::ostream& stream, const TxOut& tx)
{
    ash::db::write_data(stream, tx.address());
    ash::db::write_data(stream, tx.amount());
}

void write_data(std::ostream& stream, const Transaction& tx)
{
    ash::db::write_data(stream, tx.id());
    
    {
        const auto txins = tx.txIns();
        auto txinsize = static_cast<ash::db::StrLenType>(txins.size());
        ash::db::write_data<ash::db::StrLenType>(stream, txinsize);
        for (const auto& txin : txins)
        {
            write_data(stream, txin);
        }
    }

    {
        const auto txouts = tx.txOuts();
        auto txoutsize = static_cast<ash::db::StrLenType>(txouts.size());
        ash::db::write_data<ash::db::StrLenType>(stream, txoutsize);
        for (const auto& txout : txouts)
        {
            write_data(stream, txout);
        }
    }
}

void write_block(std::ostream& stream, const Block& block)
{
    ash::db::write_data<std::uint64_t>(stream, block.index());
    ash::db::write_data<std::uint64_t>(stream, block.nonce());
    ash::db::write_data<std::uint64_t>(stream, block.difficulty());
    ash::db::write_data(stream, block.data());

    std::uint64_t dtime = 
        static_cast<std::uint64_t>(block.time().time_since_epoch().count());
    ash::db::write_data<std::uint64_t>(stream, dtime);

    ash::db::write_data(stream, block.hash());
    ash::db::write_data(stream, block.previousHash());
    ash::db::write_data(stream, block.miner());

    const auto& txs = block.transactions();
    auto txsize = static_cast<ash::db::StrLenType>(txs.size());
    ash::db::write_data<ash::db::StrLenType>(stream, txsize);

    for (const auto& tx : txs)
    {
        write_data(stream, tx);
    }
}

void read_data(std::istream& stream, TxOutPoint& pt)
{
    ash::db::read_data(stream, pt.blockIndex);
    ash::db::read_data(stream, pt.txIndex);
    ash::db::read_data(stream, pt.txOutIndex);
}

void read_data(std::istream& stream, TxIn& txin)
{
    ash::read_data(stream, txin.txOutPt());
    ash::db::read_data(stream, txin._signature);
}

void read_data(std::istream& stream, TxOut& txout)
{
    ash::db::read_data(stream, txout._address);
    ash::db::read_data(stream, txout._amount);
}

void read_data(std::istream& stream, Transaction& tx)
{
    ash::db::read_data(stream, tx._id);

    {
        ash::db::StrLenType txincount;
        ash::db::read_data(stream, txincount);
        auto& txins = tx.txIns();
        for (ash::db::StrLenType x = 0; x < txincount; x++)
        {
            TxIn txin;
            read_data(stream, txin);
            txins.push_back(txin);
        }
    }

    {
        ash::db::StrLenType txoutcount;
        ash::db::read_data(stream, txoutcount);
        auto& txouts = tx.txOuts();
        for (ash::db::StrLenType x = 0; x < txoutcount; x++)
        {
            TxOut txout;
            read_data(stream, txout);
            txouts.push_back(txout);
        }
    }
}

void read_block(std::istream& stream, Block& block)
{
    ash::db::read_data(stream, block._hashed._index);
    ash::db::read_data(stream, block._hashed._nonce);
    ash::db::read_data(stream, block._hashed._difficulty);
    ash::db::read_data(stream, block._hashed._data);

    std::uint64_t dtime;
    ash::db::read_data(stream, dtime);
    block._hashed._time = 
        BlockTime{std::chrono::milliseconds{dtime}};

    ash::db::read_data(stream, block._hash);
    ash::db::read_data(stream, block._hashed._prev);
    ash::db::read_data(stream, block._miner);

    auto& txs = block.transactions();
    auto txcount = static_cast<ash::db::StrLenType>(txs.size());
    ash::db::read_data(stream, txcount);

    for (ash::db::StrLenType x = 0; x < txcount; x++)
    {
        Transaction tx;
        read_data(stream, tx);
        txs.push_back(tx);
    }
}

constexpr std::string_view DatabaseFile = "chain.ashdb";

ChainDatabase::ChainDatabase(std::string_view folder)
    : _folder{ folder },
      _path{ boost::filesystem::path { _folder.data()} },
      _dbfile { _path / DatabaseFile.data()},
      _logger(ash::initializeLogger("ChainDatabase"))
{
}

ChainDatabase::~ChainDatabase()
{
    delete _txIndex;
}

void ChainDatabase::initialize(Blockchain& blockchain, GenesisCallback gcb)
{
    if (!boost::filesystem::exists(_path))
    {
        _logger->debug("creating chain database folder {}", _path.generic_string());
        boost::filesystem::create_directories(_path);
    }

    if (!boost::filesystem::exists(_dbfile))
    {
        _logger->warn("creating genesis block, starting new chain");
        assert(gcb);
        write(gcb());
    }

    _logger->info("loading blockchain from {}", _dbfile.string());

    std::ifstream ifs(_dbfile.c_str(), std::ios_base::binary);
    while (ifs.peek() != EOF)
    {
        Block block;
        read_block(ifs, block);
        blockchain._blocks.push_back(block);
    }

    if (!blockchain.isValidChain())
    {
        throw std::logic_error("invalid chain");
    }

    boost::filesystem::path txidx { _path / "txinindx" };
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, txidx.string(), &_txIndex);
    if (!status.ok())
    {
        throw std::logic_error(fmt::format("could not open txin index: {}", status.ToString()));
    }

    _logger->info("loaded {} blocks from saved chain", blockchain.size());
}

void ChainDatabase::write(const Block& block)
{
    std::ofstream ofs(_dbfile.c_str(), std::ios::app | std::ios::out | std::ios::binary);
    write_block(ofs, block);
}

void ChainDatabase::writeChain(const Blockchain& chain)
{
    _logger->debug("writing {} blocks to file {}", chain.size(), _dbfile.string());
    std::ofstream ofs(_dbfile.c_str(), std::ios::app | std::ios::out | std::ios::binary);
    for (const auto& block : chain)
    {
        write_block(ofs, block);
    }
}

void ChainDatabase::reset()
{
    _logger->debug("deleting datbase file {}", _dbfile.string());

    if (boost::filesystem::exists(_dbfile))
    {
        boost::filesystem::remove(_dbfile);
    }
}

} // namespace
