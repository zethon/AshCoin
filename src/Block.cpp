#include <sstream>

#include "Block.h"
#include "sha256.h"

namespace ash
{

Block::Block(uint32_t nIndexIn, const std::string &sDataIn) 
    : _nIndex(nIndexIn), 
      _sData(sDataIn)
{
    _nNonce = 0;
    _tTime = 0;

    _sHash = calculateHash();
}

void Block::MineBlock(uint32_t nDifficulty)
{
    std::string zeros;
    zeros.assign(nDifficulty, '0');

    do
    {
        _nNonce++;
        _sHash = calculateHash();
        // std::cout << "hash: " << _sHash << std::endl;
    }
    while (_sHash.compare(0, nDifficulty, zeros) != 0);

    std::cout << "Block mined: " << _sHash << std::endl;
}

inline std::string Block::calculateHash() const
{
    std::stringstream ss;
    ss << _nIndex << _sPrevHash << _tTime << _sData << _nNonce;

    return sha256(ss.str());
}

} // namespace