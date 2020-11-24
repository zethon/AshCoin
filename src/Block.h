#pragma once

#include <cstdint>
#include <iostream>

#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace ash
{

class Block;
void to_json(nl::json& j, const Block& b);

class Block 
{
    friend void read_block(std::istream& stream, Block& block);
    friend void write_block(std::ostream& stream, const Block& block);

public:
    static std::string CalculateHash(const Block& block);

public:
    Block() = default;
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
    uint32_t        _nIndex;
    uint32_t        _nNonce;
    std::string     _data;
    time_t          _tTime; // std::uint64_t 
    std::string     _sHash;
    std::string     _sPrevHash;
};

} // namespace ash

namespace std
{

 std::ostream& operator<<(std::ostream& os, const ash::Block& block);

} // namespace std