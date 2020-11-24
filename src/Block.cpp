#include <sstream>
#include <ostream>
#include <ctime>

#include "Block.h"
#include "sha256.h"

namespace ash
{

void to_json(nl::json& j, const Block& b)
{
    j["index"] = b.index();
    j["nonce"] = b.nonce();
    j["data"] = b.data();
    j["time"] = b.time();
    j["hash"] = b.hash();
    j["prev"] = b.previousHash();
}

std::string Block::CalculateHash(const Block & block)
{
    std::stringstream ss;
    ss << block.index()
        << block.previousHash()
        << block.time()
        << block.data()
        << block.nonce();

    return sha256(ss.str());
}

Block::Block(uint32_t nIndexIn, const std::string &sDataIn)
    : _nIndex(nIndexIn), 
      _data(sDataIn)
{
    _nNonce = 0;
    _tTime = std::time(nullptr);
    _sHash = Block::CalculateHash(*this);
}

bool Block::operator==(const Block & other) const
{
    return _nIndex == other._nIndex
        && _nNonce == other._nNonce
        && _data == other._data
        && _tTime == other._tTime;
}

void Block::MineBlock(uint32_t nDifficulty)
{
    std::string zeros;
    zeros.assign(nDifficulty, '0');

    do
    {
        _nNonce++;
        _sHash = Block::CalculateHash(*this);
    }
    while (_sHash.compare(0, nDifficulty, zeros) != 0);

    std::cout << "Block mined: " << _sHash << std::endl;
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