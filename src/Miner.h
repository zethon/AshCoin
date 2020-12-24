#pragma once
#include "Block.h"
#include "AshLogger.h"
#include "CryptoUtils.h"

namespace ash
{

class Miner
{
    std::uint32_t       _difficulty = 0;
    std::uint64_t       _maxTries = 0;
    std::atomic_bool    _keepTrying = true;
    std::uint32_t       _timeout; // seconds
    SpdLogPtr           _logger;

public:
    enum ResultType { SUCCESS, TIMEOUT, ABORT };
    using Result = std::tuple<ResultType, Block>;

    Miner()
        : Miner(0)
    {
        // nothing to do
    }

    Miner(std::uint32_t difficulty)
        : _difficulty{difficulty},
          _logger(ash::initializeLogger("Miner"))
    {
        // nothing to do
    }

    std::uint32_t difficulty() const noexcept { return _difficulty; }
    void setDifficulty(std::uint32_t val) { _difficulty = val; }

    void abort() 
    { 
        _keepTrying.store(false, std::memory_order_release);
    }

    ResultType mineBlock(Block& block,
        std::function<bool(std::uint64_t)> keepGoingFunc = nullptr)
    {
        assert(block.index() > 0);
        assert(block.previousHash().size() > 0 || (block.index() - 1 == 0));

        std::string zeros;
        zeros.assign(_difficulty, '0');

        std::uint32_t nonce = 0;
        auto time = 
            std::chrono::time_point_cast<std::chrono::milliseconds>
                (std::chrono::system_clock::now());

        const auto extra = 
            ash::crypto::SHA256(nl::json(block.transactions()).dump());

        std::string hash = 
            CalculateBlockHash(
                block.index(), nonce, _difficulty, time, block.data(), block.previousHash(), extra);

        _keepTrying = true;

        while (_keepTrying.load(std::memory_order_acquire) 
            && hash.compare(0, _difficulty, zeros) != 0)
        {
            // do some extra stuff every few seconds
            if ((nonce & 0x3ffff) == 0)
            {
                if (keepGoingFunc && !keepGoingFunc(block.index()))
                {
                    // our callback has told us to bail
                    return ResultType::ABORT;
                }

                time = 
                    std::chrono::time_point_cast<std::chrono::milliseconds>
                        (std::chrono::system_clock::now());
            }

            nonce++;
            hash = CalculateBlockHash(
                block.index(), nonce, _difficulty, time, block.data(), block.previousHash(), extra);
        }

        if (!_keepTrying)
        {
            return ResultType::ABORT;
        }

        block.setMinedData(nonce, _difficulty, time, hash);

        _logger->info("successfully mined bock {}", block.index());
        return ResultType::SUCCESS;
    }
};

} // namespace ash
