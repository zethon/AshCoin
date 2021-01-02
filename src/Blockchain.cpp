#include <range/v3/all.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>

#include "CryptoUtils.h"
#include "Blockchain.h"

namespace nl = nlohmann;

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

UnspentTxOuts GetUnspentTxOuts(const Block& block)
{
    UnspentTxOuts newTxOuts;

    for (const auto& tx : block.transactions())
    {
        for (const auto& txout : tx.txOuts())
        {
            newTxOuts.emplace_back(
                UnspentTxOut{ tx.id(), block.index(), txout.address(), txout.amount() });
        }
    }

    for (const auto& tx : block.transactions())
    {
        for (const auto& txin : tx.txIns())
        {
            if (txin.txOutIndex() != block.index())
            {
                continue;
            }

            auto tempIt = std::find_if(newTxOuts.begin(), newTxOuts.end(),
                [outid = txin.txOutId()](const UnspentTxOut& unspent)
                {
                    return outid == unspent.txOutId;
                });

            if (tempIt != newTxOuts.end())
            {
                newTxOuts.erase(tempIt);
            }
        }
    }

    return newTxOuts;
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
    if (!ValidHash(block))
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
        return true;
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

void Blockchain::updateUnspentTxOuts()
{
    _logger->trace("updated unspent transactions");
    
    _unspentTxOuts.clear();
    
    for (const auto& block : _blocks)
    {
        const auto temp = GetUnspentTxOuts(block);
        std::move(temp.begin(), temp.end(), std::back_inserter(_unspentTxOuts));
    }

    _logger->debug("blockchain contains {} unspent transactions", _unspentTxOuts.size());
}

bool Blockchain::createTransaction(std::string_view receiver, double amount, std::string_view privateKey)
{
    // first get the address of the sender from the privateKey
    const auto senderAddress = ash::crypto::GetAddressFromPrivateKey(privateKey);

    // now get all of the unspent txouts of the sender
    auto senderUnspentList = _unspentTxOuts
        | ranges::views::filter([senderAddress](const UnspentTxOut& uout) { return uout.address == senderAddress; });

    bool success = false;
    double currentAmount = 0;
    double leftoverAmount = 0;
    ash::UnspentTxOuts txOuts;

    for (const auto& unspent : senderUnspentList)
    {
        txOuts.push_back(unspent);
        currentAmount = currentAmount + unspent.amount;
        if (currentAmount >= amount) 
        {
            leftoverAmount = currentAmount - amount;
            break;
        }
    }

    return success;
}

} // namespace