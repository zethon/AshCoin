#pragma once

namespace ash
{

using BlockTime = std::chrono::time_point<
    std::chrono::system_clock, std::chrono::milliseconds>;

struct BlockInfo
{
    std::uint64_t       _index;
    std::uint64_t       _nonce;
    std::uint64_t       _difficulty;
    std::string         _data;
    BlockTime           _time;
    std::string         _prev;
    Transactions        _txs;
};

} // namespace
