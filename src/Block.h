#pragma once

#include <cstdint>
#include <iostream>

namespace ash
{

class Block 
{
    std::string _sHash;
    std::string _sPrevHash;  

public:
    Block(uint32_t nIndexIn, const std::string& sDataIn);

    std::string hash() const { return _sHash; }
    std::string previous() const { return _sPrevHash; }
    void setPrevious(const std::string& val) { _sPrevHash = val; }

    void MineBlock(uint32_t nDifficulty);

private:
    uint32_t    _nIndex;
    uint32_t    _nNonce;
    std::string _sData;
    time_t      _tTime;

    std::string calculateHash() const;
};

} // namespace