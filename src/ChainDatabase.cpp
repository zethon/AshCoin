#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

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
    if (!boost::filesystem::exists(_path))
    {
        boost::filesystem::create_directories(_path);
    }

    if (!boost::filesystem::exists(_dbfile))
    {
        write(Block{ 0, "Genesis Block" });
    }

    std::ifstream ifs(_dbfile.c_str());
    std::streampos streamEnd = ifs.seekg(0, std::ios_base::end).tellg();
    ifs.clear();
    ifs.seekg(0);

    boost::archive::binary_iarchive ia(ifs);
    Block block;
    while (ifs.tellg() < streamEnd)
    {
        ia >> block;
        blockchain._vChain.emplace_back(std::move(block));
    }
}
void ChainDatabase::write(const Block& block)
{
    using namespace boost::serialization;
    
    std::ofstream ofs(_dbfile.c_str(), std::ios_base::app);
    boost::archive::binary_oarchive oa(ofs);
    oa << block;
}

std::optional<Block> ChainDatabase::read(std::uint32_t index)
{
    Block retval;
    std::uint32_t current = 0;

    std::ifstream ifs(_dbfile.c_str());
    boost::archive::binary_iarchive ia(ifs);
    while (!ifs.eof())
    {
        ia >> retval;
        if (current == index)
        {
            return retval;
        }

        current++;
    }

    return std::optional<Block>{};
}

} // namespace
