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

// UnspentTxOuts GetUnspentTxOuts(const Block& block)
// {
//     UnspentTxOuts newTxOuts;

//     for (auto txit = block.begin(); txit != block.end(); ++txit)
//     {

//     }


//     for (const auto& tx : block.transactions())
//     {
//         for (const auto& txout : tx.txOuts())
//         {
//             newTxOuts.emplace_back(
//                 UnspentTxOut{ tx.id(), block.index(), txout.address(), txout.amount() });
//         }
//     }

//     while (!newTxOuts.empty())
//     {

//     }

//     for (const auto& tx : block.transactions())
//     {
//         for (const auto& txin : tx.txIns())
//         {
//             if (txin.txOutPt().txOutIndex != block.index())
//             {
//                 continue;
//             }

//             auto tempIt = std::find_if(newTxOuts.begin(), newTxOuts.end(),
//                 [outid = txin.txOutPt().txOutId](const UnspentTxOut& unspent)
//                 {
//                     return outid == unspent.txOutId;
//                 });

//             if (tempIt != newTxOuts.end())
//             {
//                 newTxOuts.erase(tempIt);
//             }
//         }
//     }

//     return newTxOuts;
// }

UnspentTxOuts GetUnspentTxOuts(const Blockchain& chain)
{
    // for every TxOut we have to check if there is a TxIn
    // that references it
    UnspentTxOuts txOuts;

    for (const auto& block : chain)
    {
        for (const auto& tx : block.transactions())
        {
            for (const auto& txin : tx.txIns())
            {
                // txOuts.push_back()
            }
        }
    }

    return txOuts;
}

Blockchain::Blockchain()
    : _logger(ash::initializeLogger("Blockchain"))
{
    // nothing to do
}

Block Blockchain::txDetails(std::size_t index) const
{ 
    const auto chainsize = _blocks.size();
    Block retblock = _blocks.at(index);
    
    // loops through the transactions of the block we're
    // interested in
    for (auto& tx : retblock.transactions())
    {
        // for each TxIn of the block we want to fill in
        // some missing information
        for (auto& txin : tx.txIns())
        {
            assert(txin.txOutPt().blockIndex < chainsize);

            // we know the corresponding TxOut's block, txid 
            // and txout index
            const auto& outblock = _blocks.at(txin.txOutPt().blockIndex);
            const auto& txs = outblock.transactions();

            // find the transaction containing the TxOut from 
            // which this TxIn was create
            auto txoutit = std::find_if(txs.begin(), txs.end(),
                [txid = txin.txOutPt().txOutId](const ash::Transaction& temptx)
                {
                    return txid == temptx.id();
                });

            if (txoutit == txs.end()) continue;
            const auto& txout = *txoutit;

            assert(txin.txOutPt().txOutIndex < txout.txIns().size());
            txin.txOutPt().address = txout.txOuts().at(txin.txOutPt().txOutIndex).address();
            txin.txOutPt().amount = txout.txOuts().at(txin.txOutPt().txOutIndex).amount();
        }
    }

    return retblock;
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

UnspentTxOuts Blockchain::getUnspentTxOuts(std::string_view address)
{
    UnspentTxOuts retval;

    for (const auto& block : _blocks)
    {
        for (const auto& tx : block.transactions())
        {
            // collect the txouts first
            for (const auto& txout : tx.txOuts())
            {
                if (txout.address() == address)
                {
                    retval.emplace_back(UnspentTxOut{block.index(), tx.id(), address.data(), txout.amount()});
                }
            }

            // now scrub out the txins
            for (const auto& txin : tx.txIns())
            {
                auto it = std::find_if(retval.begin(), retval.end(),
                    [txin = txin](const UnspentTxOut& utxout)
                    {
                        return (utxout.txOutIndex == txin.txOutPt().txOutIndex
                            && utxout.txOutId == txin.txOutPt().txOutId);
                    });

                if (it != retval.end()) retval.erase(it);
            }
        }
    }

    return retval;
}

// TODO: THIS IS PROBABLY TRASH
void Blockchain::updateUnspentTxOuts()
{
    _logger->trace("updated unspent transactions");
    
    _unspentTxOuts.clear();
    
    // // gather up all the outs
    // for (const auto& block : _blocks)
    // {
    //     const auto temp = GetUnspentTxOuts(block);
    //     std::move(temp.begin(), temp.end(), std::back_inserter(_unspentTxOuts));
    // }

    // // now go through the txins and remove the textouts
    // for (const auto& block : _blocks)
    // {
    //     for (const auto& tx : block.transactions())
    //     {
    //         for (const auto& txin : tx.txIns())
    //         {
    //             // auto it = std::find_if(_unspentTxOuts.begin(), _unspentTxOuts.end()
    //             // [](){ });
    //         }
    //     }
    // }

    // _txInHashes.clear();
    // for (const auto& block : _blocks)
    // {
    //     updateUnspentTxOuts(block);
    // }
    
    _logger->debug("blockchain contains {} unspent transactions", _unspentTxOuts.size());
}

bool Blockchain::createTransaction(std::string_view receiver, double amount, std::string_view privateKey)
{
    // // first get the address of the sender from the privateKey
    // const auto senderAddress = ash::crypto::GetAddressFromPrivateKey(privateKey);

    // // now get all of the unspent txouts of the sender
    // auto senderUnspentList = this->getUnspentTxOuts(senderAddress);

    bool success = false;
    // double currentAmount = 0;
    // double leftoverAmount = 0;
    // ash::UnspentTxOuts includedUnspentOuts;

    // for (const auto& unspent : senderUnspentList)
    // {
    //     includedUnspentOuts.push_back(unspent);
    //     currentAmount += unspent.amount;
    //     if (currentAmount >= amount) 
    //     {
    //         leftoverAmount = currentAmount - amount;
    //         break;
    //     }
    // }

    // ash::Transaction tx;

    // auto& txins = tx.txIns();
    // for (const auto& uout : includedUnspentOuts)
    // {
    //     txins.emplace_back(uout.txOutId, uout.txOutIndex);
    // }

    // auto& outs = tx.txOuts();
    // outs.emplace_back(receiver, amount);
    // if (leftoverAmount > 0)
    // {
    //     outs.emplace_back(senderAddress, leftoverAmount);
    // }

    // _txQueue.push(std::move(tx));

    return success;
}

void Blockchain::getTransactionsToBeMined(Block& block)
{
    auto& txs = block.transactions();

    // we assume someone else is making sure we're thread safe!
    while (!_txQueue.empty())
    {
        auto& tx = _txQueue.front();
        tx.calcuateId(block.index());
        txs.push_back(std::move(tx));
        _txQueue.pop();
    }
}

std::size_t Blockchain::reQueueTransactions(Block& block)
{
    std::size_t count = 0;

    for (const auto& tx : block.transactions())
    {
        if (tx.isCoinebase()) continue;
        _txQueue.push(tx); // copy!!
        count++;
    }

    return count;
}

} // namespace