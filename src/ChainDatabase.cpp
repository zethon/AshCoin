#include "Blockchain.h"

#include "ChainDatabase.h"

namespace ash
{

constexpr std::string_view AnchorFile = "anchor.bin";
constexpr std::string_view DatabaseFile = "chain.ashdb";

ChainDatabase::ChainDatabase(std::string_view folder)
    : _folder{ folder },
      _path{ boost::filesystem::path { _folder.data()} },
      _dbfile { _path / DatabaseFile.data()}
{
}

void ChainDatabase::initialize(Blockchain& blockchain)
{
    assert(blockchain.size() > 0);
    if (!boost::filesystem::exists(_path))
    {
        boost::filesystem::create_directories(_path);
    }

    if (!boost::filesystem::exists(_dbfile))
    {
        write(Block{ 0, "Genesis Block" });
    }

}
void ChainDatabase::write(const Block & block)
{
}

void ChainDatabase::read(std::uint32_t index)
{
}

} // namespace
