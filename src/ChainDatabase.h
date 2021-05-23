#pragma once
#include <string_view>
#include <optional>
#include <iterator>
#include <cstddef>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <leveldb/db.h>
#include <leveldb/comparator.h>

#include "Block.h"
#include "Blockchain.h"
#include "AshLogger.h"

namespace ash
{

namespace db
{

using LevelDBPtr = std::unique_ptr<leveldb::DB>;

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

inline void write_data(std::ostream& stream, double val)
{
    stream.write(reinterpret_cast<PointerType>(&val), sizeof(double));
}

inline void write_data(std::ostream& stream, std::string_view data)
{
    auto size = static_cast<StrLenType>(data.size());
    write_data<StrLenType>(stream, size);
    stream.write(data.data(), size);
}

template<typename T,
    typename = typename std::enable_if<(std::is_integral<T>::value)>::type>
inline void read_data(std::istream& stream, T& value)
{
    stream.read(reinterpret_cast<PointerType>(&value), sizeof(value));
}

inline void read_data(std::istream& stream, double& val)
{
    stream.read(reinterpret_cast<PointerType>(&val), sizeof(double));
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

class IChainDatabase;
using IChainDatabasePtr = std::unique_ptr<IChainDatabase>;

class chaindb_error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class IChainDatabase
{
    std::string _datafolder;

public:
    IChainDatabase(const std::string& datafolder)
        : _datafolder(datafolder)
    {
        // nothing to do
    }

    virtual  ~IChainDatabase() = default;

    std::string datafolder() const { return _datafolder; }

    // TODO: this should not be in the base class
    using GenesisCallback = std::function<Block(void)>;
    virtual void initialize(Blockchain& chain, GenesisCallback gcb) = 0;

    virtual std::optional<Block> read(std::size_t index) const = 0;
    virtual std::size_t size() = 0;

    virtual void write(const Block& block) = 0;
    virtual void writeChain(const Blockchain& chain) = 0;
    
    virtual void reset() = 0;
};

class ChainDBComparator : public leveldb::Comparator
{
public:
    ~ChainDBComparator() override = default;

    int Compare(const leveldb::Slice &a, const leveldb::Slice &b) const override;
    const char *Name() const override;
    void FindShortestSeparator(std::string *start, const leveldb::Slice &limit) const override;
    void FindShortSuccessor(std::string *key) const override;
};

class ChainLevelDB final : public IChainDatabase
{
    std::unique_ptr<leveldb::DB>        _db;
    std::unique_ptr<ChainDBComparator>  _comparer;
    std::size_t                         _size = 0;

public:
    ChainLevelDB(const std::string& foldername);
    ~ChainLevelDB() = default;

    void initialize(Blockchain& chain, GenesisCallback gcb) override;

    std::optional<Block> read(std::size_t index) const override;
    size_t size() override;

    void write(const Block &block) override;
    void writeChain(const Blockchain &chain) override;

    void reset() override;

    std::unique_ptr<leveldb::Iterator> const_iteraor() const
    {
        std::unique_ptr<leveldb::Iterator> itptr;
        itptr.reset(_db->NewIterator(leveldb::ReadOptions()));
        return itptr;
    }

    const std::unique_ptr<leveldb::DB>& leveldb() const
    {
        return _db;
    }

    bool validate() const;

private:
    void finalize(leveldb::WriteBatch& batch);

};

class ChainDatabase final : public IChainDatabase
{

public:
    ChainDatabase(std::string_view folder);
    ~ChainDatabase();

    void write(const Block& block) override;
    void writeChain(const Blockchain& chain) override;

    void initialize(Blockchain& chain, GenesisCallback gcb) override;
    void reset() override;

    std::optional<Block> read(std::size_t index) const override
    {
        return {};
    }

    std::size_t size() override
    {
        return 0;
    }

private:
    boost::filesystem::path     _path;
    boost::filesystem::path     _dbfile;
    // ash::db::LevelDBPtr         _txInIndex;
    leveldb::DB*                _txIndex = nullptr;
    
    SpdLogPtr                   _logger;
};

} // namespace
