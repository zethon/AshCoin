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
    _tTime = time(nullptr);

    _sHash = calculateHash();
}

void Block::MineBlock(uint32_t nDifficulty)
{
    char cstr[nDifficulty + 1];
    for (uint32_t i = 0; i < nDifficulty; ++i)
    {
        cstr[i] = '0';
    }
    cstr[nDifficulty] = '\0';

    std::string str(cstr);

    do
    {
        _nNonce++;
        _sHash = calculateHash();
        // std::cout << "hash: " << _sHash << std::endl;
    }
    while (_sHash.substr(0, nDifficulty) != str);

    std::cout << "Block mined: " << _sHash << std::endl;
}

inline std::string Block::calculateHash() const
{
    std::stringstream ss;
    ss << _nIndex << _sPrevHash << _tTime << _sData << _nNonce;

    return sha256(ss.str());
}

} // namespace