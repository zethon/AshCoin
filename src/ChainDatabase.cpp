#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/binary_object.hpp>

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

    std::size_t index = 0;
    Block block;
    while (ifs.tellg() < streamEnd)
    {
        try
        {
            //boost::archive::binary_iarchive ia(ifs);
            boost::archive::text_iarchive ia(ifs);
            ia >> block;
        }
        catch (const std::bad_alloc& er)
        {
            std::cout << "position: " << ifs.tellg() << '\n';
            std::cout << "index: " << index++ << '\n';
            std::cout << "error at index: " << index << " error: " << er.what() << '\n';
        }

        std::cout << "loaded: " << block << '\n';
        std::cout << "position: " << ifs.tellg() << '\n';
        std::cout << "index: " << index++ << '\n';
        std::cout << "--------------------------------------------\n";
        blockchain._vChain.push_back(block);
    }
}
void ChainDatabase::write(const Block& block)
{
    using namespace boost::serialization;
    
    std::ofstream ofs(_dbfile.c_str(), std::ios_base::app);

//    auto bo = make_binary_object()
    // boost::archive::binary_oarchive oa(ofs);
    boost::archive::text_oarchive oa(ofs);
    oa << block;

    std::cout << "saved: " << block << '\n';
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
