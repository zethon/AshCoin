#include <memory>

#include <fmt/core.h>

#include <leveldb/write_batch.h>
#include <ashdb/ashdb.h>

#include "Transactions.h"
#include "Blockchain.h"
#include "ChainDatabase.h"

namespace ash
{

AshChainDatabase::AshChainDatabase(std::string_view folder)
    : _path{ boost::filesystem::path { folder.data()} },
      _logger(ash::initializeLogger("AshChainDatabase"))
{
}

void AshChainDatabase::initialize(Blockchain& blockchain)
{
    if (!boost::filesystem::exists(_path))
    {
        _logger->debug("creating chain database folder {}", _path.generic_string());
        boost::filesystem::create_directories(_path);
    }

    _logger->debug("opening blockchain database");
    ashdb::Options options;
    options.filesize_max = 1024 * 1024 * 5; // five megs
    options.prefix = "blocks";
    options.extension = "dat";
    _db = std::make_unique<DBType>(_path.string(), options);

    if (_db->open() != ashdb::OpenStatus::OK)
    {
        throw std::logic_error("could not open blockchain database");
    }

    _logger->info("loading blockchain from {}", _path.string());


    if (!blockchain.isValidChain())
    {
        throw std::logic_error("invalid chain");
    }

    _logger->info("loaded {} blocks from saved chain", blockchain.size());
}

void AshChainDatabase::write(const Block& block)
{
    _db->write(block);
}

void AshChainDatabase::writeChain(const Blockchain& chain)
{
//    _logger->debug("writing {} blocks to file {}", chain.size(), _dbfile.string());
//    std::ofstream ofs(_dbfile.c_str(), std::ios::app | std::ios::out | std::ios::binary);
//    for (const auto& block : chain)
//    {
//        write_block(ofs, block);
//    }
}

std::optional<Block> AshChainDatabase::read(std::size_t index) const
{
    return _db->read(index);
}

} // namespace
