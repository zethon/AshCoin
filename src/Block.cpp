#include <sstream>
#include <ostream>
#include <ctime>

#include "Block.h"
#include "sha256.h"

namespace nl = nlohmann;

namespace ash
{

void to_json(nl::json& j, const Block& b)
{
    j["index"] = b.index();
    j["nonce"] = b.nonce();
    j["difficulty"] = b.difficulty();
    j["data"] = b.data();
    j["time"] = b.time();
    j["hash"] = b.hash();
    j["prev"] = b.previousHash();
}

void from_json(const nl::json& j, Block& b)
{
    j["index"].get_to(b._index);
    j["nonce"].get_to(b._nonce);
    j["difficulty"].get_to(b._difficulty);
    j["data"].get_to(b._data);
    j["time"].get_to(b._time);
    j["hash"].get_to(b._hash);
    j["prev"].get_to(b._prev);
}

std::string CalculateBlockHash(
    std::uint64_t index, 
    std::uint32_t nonce, 
    std::uint32_t difficulty,
    time_t time, 
    const std::string& data, 
    const std::string& previous)
{
    std::stringstream ss;
    ss << index
        << nonce
        << difficulty
        << data
        << time
        << previous;

    return sha256(ss.str());
}

std::string CalculateBlockHash(const Block& block)
{
    std::stringstream ss;
    ss << block.index()
        << block.nonce()
        << block.difficulty()
        << block.data()
        << block.time()
        << block.previousHash();

    return sha256(ss.str());
}

Block::Block(uint64_t nIndexIn, std::string_view sDataIn)
    : _index(nIndexIn), 
      _data(sDataIn),
      _logger(ash::initializeLogger("Block"))
{
    _nonce = 0;
    _time = 0;
    _hash = CalculateBlockHash(*this);
}

bool Block::operator==(const Block & other) const
{
    return _index == other._index
        && _nonce == other._nonce
        && _data == other._data
        && _time == other._time;
}

void Block::MineBlock(std::uint32_t nDifficulty)
{
    _difficulty = nDifficulty;

    std::string zeros;
    zeros.assign(_difficulty, '0');

    do
    {
        _nonce++;
        _time = std::time(nullptr); // this is probably bad
        _hash = CalculateBlockHash(_index, _nonce, _difficulty, _time, _data, _prev);
    }
    while (_hash.compare(0, _difficulty, zeros) != 0);

    const nl::json temp = *this;
    _logger->debug("block mined: {}", temp.dump());
}

BlockInfo Block::info() const
{
    return { _index, _nonce, _difficulty, _time, _data, _prev };
}

} // namespace

namespace std
{

std::ostream& operator<<(std::ostream & os, const ash::Block & block)
{
    os << "block { index: " << block.index() << " hash: " << block.hash() << " data: " << block.data() << " }";
    return os;
}

} // namespace std