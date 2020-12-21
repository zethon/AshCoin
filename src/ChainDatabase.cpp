#include <fmt/chrono.h>

#include "Transactions.h"
#include "Blockchain.h"
#include "ChainDatabase.h"

namespace ash
{

constexpr std::string_view GENESIS_BLOCK = "HenryCoin Genesis";

void write_block(std::ostream& stream, const TxIn& tx)
{

}

void write_block(std::ostream& stream, const TxOut& tx)
{
    
}


void write_block(std::ostream& stream, const Transaction& tx)
{

}


void write_block(std::ostream& stream, const Block& block)
{
    ash::db::write_data<std::uint64_t>(stream, block.index());
    ash::db::write_data<std::uint32_t>(stream, block.nonce());
    ash::db::write_data<std::uint32_t>(stream, block.difficulty());
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
        write_block(stream, tx);
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
}

constexpr std::string_view DatabaseFile = "chain.ashdb";

ChainDatabase::ChainDatabase(std::string_view folder)
    : _folder{ folder },
      _path{ boost::filesystem::path { _folder.data()} },
      _dbfile { _path / DatabaseFile.data()},
      _logger(ash::initializeLogger("ChainDatabase"))
{
}

void ChainDatabase::initialize(Blockchain& blockchain)
{
    if (!boost::filesystem::exists(_path))
    {
        _logger->debug("creating chain database folder {}", _path.generic_string());
        boost::filesystem::create_directories(_path);
    }

    if (!boost::filesystem::exists(_dbfile))
    {
        _logger->warn("creating genesis block, starting new chain");

        std::time_t t = std::time(nullptr);
        const auto gendata = fmt::format("{} {:%Y-%m-%d %H:%M:%S %Z}.",GENESIS_BLOCK, *std::localtime(&t));
        _logger->trace("generating genesis block with data '{}'", gendata);
        write(Block{ 0, gendata });
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
