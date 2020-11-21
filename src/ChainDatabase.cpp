#include "ChainDatabase.h"

namespace ash
{

constexpr std::string_view AnchorFile = "anchor.bin";
constexpr std::string_view FilePattern = "chain-{}.blockdb";

ChainDatabase::ChainDatabase(std::string_view folder)
    : _folder{ folder },
      _path{ boost::filesystem::path { _folder.data()} }
{
    if (!boost::filesystem::exists(_path))
    {
        boost::filesystem::create_directories(_path);
    }
}

void ChainDatabase::initialize()
{

}

void ChainDatabase::createDatabase()
{

}

void ChainDatabase::writeBlock(const Block & block)
{
}

void ChainDatabase::readBlock(std::uint32_t index)
{
}

} // namespace
