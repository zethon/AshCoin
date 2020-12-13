#pragma once
#include "Block.h"

namespace ash
{

class Miner
{
    std::uint32_t       _difficulty = 0;
    std::uint64_t       _maxTries = 0;
    std::atomic_bool    _keepTrying = true;
    std::uint32_t       _timeout; // seconds

public:
    enum ResultType { SUCCESS, TIMEOUT, ABORT };
    using Result = std::tuple<ResultType, Block>;

    Miner()
        : Miner(0)
    {
        // nothing to do
    }

    Miner(std::uint32_t difficulty)
        : _difficulty{difficulty}
    {
        // nothing to do
    }

    std::uint32_t difficulty() const noexcept { return _difficulty; }
    void setDifficulty(std::uint32_t val) { _difficulty = val; }

    void abort() { _keepTrying = false; }

    Result mineBlock(std::uint64_t index, 
        const std::string& data, const std::string& prev)
    {
        assert(index > 0);
        assert(prev.size() > 0 || (index - 1 == 0));

        std::string zeros;
        zeros.assign(_difficulty, '0');

        std::uint32_t nonce = 0;
        time_t time = std::time(nullptr);
        std::string hash = 
            CalculateBlockHash(index, nonce, _difficulty, time, data, prev);

        _keepTrying = true;

        while (_keepTrying 
            && hash.compare(0, _difficulty, zeros) != 0)
        {
            // TODO: might only need to return these three things
            // instead of creating a new block?
            nonce++;
            time = std::time(nullptr); // this is probably bad
            hash = CalculateBlockHash(index, nonce, _difficulty, time, data, prev);
        }

        if (!_keepTrying)
        {
            return { ResultType::ABORT, {} };
        }

        Block retval { index, data };
        retval._hashed._nonce = nonce;
        retval._hashed._difficulty = _difficulty;
        retval._hashed._time = time;
        retval._hashed._prev = prev;
        retval._hash = hash;

        return { ResultType::SUCCESS, retval };
    }
};

} // namespace ash
