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
    static std::string CalculateHash(const Block& block);

public:
    Block(uint32_t nIndexIn, const std::string& sDataIn);

    bool operator==(const Block& other) const;

    std::uint32_t index() const { return _nIndex; }
    std::string data() const { return _data;  }
    std::string hash() const { return _sHash; }
    std::string previousHash() const { return _sPrevHash; }
    std::uint32_t nonce() const { return _nNonce; }
    time_t time() const { return _tTime; }

    void setPrevious(const std::string& val) { _sPrevHash = val; }
    void MineBlock(uint32_t nDifficulty);

private:
    uint32_t    _nIndex;
    uint32_t    _nNonce;
    std::string _data;
    time_t      _tTime;
};

} // namespace ash

namespace std
{

 std::ostream& operator<<(std::ostream& os, const ash::Block& block);

}