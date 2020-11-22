#pragma once
#include <string_view>

#include <boost/filesystem.hpp>

#include "Block.h"

namespace ash
{

class ChainDatabase;
using ChainDatabasePtr = std::unique_ptr<ChainDatabase>;

class ChainDatabase
{

public:
    ChainDatabase(std::string_view folder);

    void write(const Block& block);
    void read(std::uint32_t index);

    void initialize(Blockchain& chain);

private:
    std::string                 _folder;

    boost::filesystem::path     _path;
    boost::filesystem::path     _dbfile;
    boost::filesystem::path     _anchorFile;
};

} // namespace
