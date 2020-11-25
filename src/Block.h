#pragma once

#include <cstdint>
#include <iostream>

#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace ash
{
class Block;

std::string CalculateBlockHash(const Block& block);
void to_json(nl::json& j, const Block& b);

class Block 
{
    friend void read_block(std::istream& stream, Block& block);
    friend void write_block(std::ostream& stream, const Block& block);

public:
    Block() = default;
    Block(uint32_t nIndexIn, const std::string& sDataIn);

    bool operator==(const Block& other) const;

    std::uint32_t index() const { return _index; }
    std::uint32_t nonce() const { return _nonce; }
    std::uint32_t difficulty() const { return _difficulty; }

    std::string data() const { return _data;  }
    time_t time() const { return _time; }
    std::string hash() const { return _hash; }
    std::string previousHash() const { return _prev; }
   

    void setPrevious(const std::string& val) { _prev = val; }
    void MineBlock(uint32_t nDifficulty);

private:
    std::uint32_t        _index;
    std::uint32_t        _nonce;
    std::uint32_t        _difficulty;

    std::string     _data;
    time_t          _time; // std::uint64_t 
    std::string     _hash;
    std::string     _prev;
};

} // namespace ash

namespace std
{

 std::ostream& operator<<(std::ostream& os, const ash::Block& block);

} // namespace std