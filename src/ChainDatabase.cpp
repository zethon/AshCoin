#include "Blockchain.h"
#include "AshDb.h"

#include "ChainDatabase.h"

namespace ash
{

constexpr std::string_view GENESIS_BLOCK = "HenryCoin Genesis";

void write_block(std::ostream& stream, const Block& block)
{
    ashdb::write_data<std::uint32_t>(stream, block.index());
    ashdb::write_data<std::uint32_t>(stream, block.nonce());
    ashdb::write_data<std::uint32_t>(stream, block.difficulty());
    ashdb::write_data(stream, block.data());

    std::uint64_t dtime = static_cast<std::uint64_t>(block.time());
    ashdb::write_data<std::uint64_t>(stream, dtime);

    ashdb::write_data(stream, block.hash());
    ashdb::write_data(stream, block.previousHash());
}

void read_block(std::istream& stream, Block& block)
{
    ashdb::read_data(stream, block._index);
    ashdb::read_data(stream, block._nonce);
    ashdb::read_data(stream, block._difficulty);
    ashdb::read_data(stream, block._data);

    std::uint64_t dtime;
    ashdb::read_data(stream, dtime);
    block._time = static_cast<time_t>(dtime);

    ashdb::read_data(stream, block._hash);
    ashdb::read_data(stream, block._prev);
}

constexpr std::string_view AnchorFile = "anchor.bin";
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
        _logger->warn("creating genesis block, starting new crypto?");
        write(Block{ 0, GENESIS_BLOCK });
    }

    std::ifstream ifs(_dbfile.c_str(), std::ios_base::binary);
    while (ifs.peek() != EOF)
    {
        Block block;
        read_block(ifs, block);
        blockchain._blocks.push_back(block);
    }

    _logger->info("loaded {} blocks from saved chain", blockchain.size());
    if (!blockchain.isValidChain())
    {
        throw std::logic_error("invalid chain");
    }
}
void ChainDatabase::write(const Block& block)
{
    std::ofstream ofs(_dbfile.c_str(), std::ios::app | std::ios::out | std::ios::binary);
    write_block(ofs, block);
}

std::optional<Block> ChainDatabase::read(std::uint32_t index)
{
    //Block retval;
    //std::uint32_t current = 0;

    //std::ifstream ifs(_dbfile.c_str());
    //boost::archive::binary_iarchive ia(ifs);
    //while (!ifs.eof())
    //{
    //    ia >> retval;
    //    if (current == index)
    //    {
    //        return retval;
    //    }

    //    current++;
    //}

    return std::optional<Block>{};
}

} // namespace
