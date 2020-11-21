#pragma once
#include <string_view>

#include <boost/filesystem.hpp>

#include "Block.h"

namespace ash
{

class ChainDatabase;
using ChainDatabasePtr = std::unique_ptr<ChainDatabase>;

struct ChainIndicie
{
    std::uint64_t   fileid;
    std::uint64_t   idx1;
    std::uint64_t   idx2;
};

struct ChainIndex
{
    void open(std::string_view filename)
    {
    }

    void close()
    {
    }

    ChainIndicie read(std::uint64_t index)
    {
        return ChainIndicie{};
    }

    std::uint64_t append(const ChainIndicie& indicie)
    {
        return std::uint64_t{};
    }

    void update(std::uint64_t index, const ChainIndicie& indicie)
    {
    }
};

class ChainDatabase
{

public:
    ChainDatabase(std::string_view folder);

    void writeBlock(const Block& block);
    void readBlock(std::uint32_t index);

    void initialize(Blockchain& chain);

private:
    std::string                 _folder;

    boost::filesystem::path     _path;
    boost::filesystem::path     _anchorFile;
};

} // namespace
