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

    void writeBlock(const Block& block);
    void readBlock(std::uint32_t index);

private:
    void initialize();
    void createDatabase();

private:
    std::string                 _folder;
    boost::filesystem::path     _path;
};

} // namespace
