#include <range/v3/all.hpp>

#include "Blockchain.h"

namespace ash
{

void to_json(nl::json& j, const Blockchain& b)
{
    for (const auto& block : b._blocks)
    {
        j.push_back(block);
    }
}

void from_json(const nl::json& j, Blockchain& b)
{
    b.clear();

    for (const auto& jblock : j.items())
    {
        b._blocks.push_back(jblock.value().get<Block>());
    }
}

Blockchain::Blockchain()
    : _logger(ash::initializeLogger("Blockchain"))
{
    // nothing to do
}

bool Blockchain::addNewBlock(const Block& block)
{
    return addNewBlock(block, true);
}

bool Blockchain::addNewBlock(const Block& block, bool checkPreviousBlock)
{
    if (block.hash() != CalculateBlockHash(block))
    {
        return false;
    }
    else if (checkPreviousBlock
        && block.previousHash() != _blocks.back().hash())
    {
        return false;
    }

    _blocks.push_back(block);
    updateUnspentTxOuts();

    return true;
}

bool Blockchain::isValidBlockPair(std::size_t idx) const
{
    if (idx > _blocks.size() || idx < 1)
    {
        return false;
    }

    const auto& current = _blocks.at(idx);
    const auto& prev = _blocks.at(idx - 1);

    return (current.index() == prev.index() + 1)
        && (current.previousHash() == prev.hash())
        && (CalculateBlockHash(current) == current.hash());
}

bool Blockchain::isValidChain() const
{
    if (_blocks.size() == 0)
    {
        return false;
    }

    for (auto idx = 1u; idx < _blocks.size(); idx++)
    {
        if (!isValidBlockPair(idx))
        {
            return false;
        }
    }

    return true;
}

std::uint64_t Blockchain::cumDifficulty(std::size_t idx) const
{
    std::uint64_t total = 0;
    auto lastBlockIt = std::next(_blocks.begin(), idx);

    for (auto current = _blocks.begin(); current < lastBlockIt; current++)
    {
        total += static_cast<std::uint64_t>
            (std::pow(2u, current->difficulty()));
    }

    return total;
}

UnspentTxOuts Blockchain::getUnspentTxOuts(const Block& block)
{
    UnspentTxOuts retval;

    for (const auto& tx : block.transactions())
    {
        for (const auto& txout : tx.txOuts())
        {
            retval.emplace_back(
                UnspentTxOut{tx.id(), block.index(), txout.address(), txout.amount()});
        }
    }

    return retval;
}

void Blockchain::updateUnspentTxOuts()
{
    _logger->debug("updated unspent transactions");
    
    _unspentTxOuts.clear();
    
    for (const auto& block : _blocks)
    {
        const auto temp = getUnspentTxOuts(block);
        std::move(temp.begin(), temp.end(), std::back_inserter(_unspentTxOuts));
    }

    _logger->trace("blockchain contains {} unspent transactions", _unspentTxOuts.size());
}

} // namespace