#pragma once
#include <string_view>
#include <optional>

#include <boost/filesystem.hpp>

#include "Block.h"
#include "AshLogger.h"

namespace ash
{

namespace db
{

using ValueType = unsigned char;
using Container = std::vector<ValueType>;
using PointerType = char*;
using StrLenType = std::uint32_t;

template <typename T,
    typename = typename std::enable_if<(std::is_integral<T>::value)>::type>
inline void write_data(std::ostream& stream, T value)
{
    stream.write(reinterpret_cast<PointerType>(&value), sizeof(value));
}

inline void write_data(std::ostream& stream, std::string_view data)
{
    auto size = static_cast<StrLenType>(data.size());
    write_data<StrLenType>(stream, size);
    stream.write(data.data(), size);
}

template <typename T,
    typename = typename std::enable_if<(std::is_integral<T>::value)>::type>
inline void read_data(std::istream& stream, T& value)
{
    stream.read(reinterpret_cast<PointerType>(&value), sizeof(value));
}

inline void read_data(std::istream& stream, std::string& data)
{
    StrLenType len;
    read_data(stream, len);

    data.resize(len);
    stream.read(reinterpret_cast<PointerType>(data.data()), len);
}

} // namespace ash::db

class ChainDatabase;
using ChainDatabasePtr = std::unique_ptr<ChainDatabase>;

class ChainDatabase
{

public:
    ChainDatabase(std::string_view folder);

    void write(const Block& block);
    void writeChain(const Blockchain& chain);

    void initialize(Blockchain& chain);
    void reset();

private:
    std::string                 _folder;

    boost::filesystem::path     _path;
    boost::filesystem::path     _dbfile;
    
    SpdLogPtr                   _logger;
};

} // namespace
