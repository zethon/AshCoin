#pragma once
#include "Block.h"

namespace ash
{

class Miner
{
    std::uint32_t       _difficulty = 0;
    std::uint64_t       _maxTries = 0;
    std::atomic_bool    _keepTrying = true;

public:
    enum ResultType { SUCCESS, TIMEOUT, ABORT };
    using Result = std::tuple<ResultType, Block>;

    Miner() = default;
    Miner(std::uint32_t difficulty)
        : _difficulty{difficulty}
    {
        // nothing to do
    }

    std::uint32_t difficulty() const noexcept { return _difficulty; }
    void setDifficulty(std::uint32_t val) { _difficulty = val; }

    Result mineBlock(const BlockInfo& info)
    {
        assert(input.index > 0);
        assert(input.previous.size() > 0);

        std::string zeros;
        zeros.assign(info.difficulty, '0');

        std::uint32_t nonce = 0;
        time_t time = std::time(nullptr);
        std::string hash = 
            CalculateBlockHash(info.index, nonce, info.difficulty, time, info.data, info.prev);

        while (_keepTrying && hash.compare(0, info.difficulty, zeros) != 0)
        {
            // TODO: might only need to return these three things
            // instead of creating a new block?
            nonce++;
            time = std::time(nullptr); // this is probably bad
            hash = CalculateBlockHash(info.index, nonce, info.difficulty, time, info.data, info.prev);
        }

        if (!_keepTrying)
        {
            return { ResultType::ABORT, {} };
        }

        Block retval { info.index, info.data };
        retval._nonce = nonce;
        retval._difficulty = info.difficulty;
        retval._time = time;
        retval._hash = hash;
        retval._prev = info.prev;

        return { ResultType::SUCCESS, retval };
    }
};

} // namespace ash
