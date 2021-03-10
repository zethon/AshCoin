#include <set>

#include <boost/range/adaptor/indexed.hpp>

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

void to_json(nl::json& j, const LedgerInfo& li)
{
    j["txid"] = li.txid;
    j["blockid"] = li.blockIdx;
    j["amount"] = li.amount;
    j["time"] =
            static_cast<std::uint64_t>(li.time.time_since_epoch().count());
}

void to_json(nl::json& j, const AddressLedger& ledger)
{
    for (const auto& i : ledger)
    {
        j.push_back(i);
    }
}

UnspentTxOuts GetUnspentTxOuts(const Blockchain& chain, const std::string& address)
{
    auto cmp = 
        [](const UnspentTxOut& a, const UnspentTxOut& b)
        {
            return std::hash<UnspentTxOut>{}(a) < std::hash<UnspentTxOut>{}(b);
        };

    std::set<UnspentTxOut, decltype(cmp)> outs(cmp);

    for (const auto& block : chain)
    {
        for (const auto& txitem : block.transactions() | boost::adaptors::indexed())
        {
            const auto& tx = txitem.value();

            assert(tx.txIns().size() > 0);
            assert(tx.txOuts().size() > 0);

            for (const auto& txoutitem : tx.txOuts() | boost::adaptors::indexed())
            {
                const auto& txout = txoutitem.value();
                if (address.size() > 0 && txout.address() != address)
                {
                    continue;
                }

                outs.insert({
                    block.index(),
                    static_cast<std::uint64_t>(txitem.index()),
                    static_cast<std::uint64_t>(txoutitem.index()),
                    txout.address(), 
                    txout.amount()});
            }

            // if this is a coinbase transaction then we do not need
            // to process the TxIns
            if (tx.isCoinbase())
            {
                continue;
            }

            for (const auto& txin : tx.txIns())
            {
                auto it = std::find_if(outs.begin(), outs.end(),
                   [txin = txin](const UnspentTxOut& utxout)
                   {
                        return (txin.txOutPt().blockIndex == utxout.blockIndex)
                            && (txin.txOutPt().txIndex == utxout.txIndex)
                            && (txin.txOutPt().txOutIndex == utxout.txOutIndex);
                   });

                if (it != outs.end())
                {
                    outs.erase(it);
                }
            }
        }
    }

    UnspentTxOuts retval;
    std::move(outs.begin(), outs.end(), std::back_inserter(retval));

    return retval;
}

AddressLedger GetAddressLedger(const Blockchain& chain, const std::string& address)
{
    ash::AddressLedger ledger;

    for (const auto& block : chain)
    {
        const auto fullblock = ash::GetBlockDetails(chain, block.index());
        for (const auto& tx : fullblock.transactions())
        {
            bool debit = false;
            assert(tx.txOuts().size() > 0);
            
            for (const auto& txout : tx.txOuts())
            {
                if (txout.address() == address)
                {
                    ledger.push_back(
                        LedgerInfo{ block.index(), tx.id(), block.time(), txout.amount() });

                    break;
                }
            }

            // this is a debit for the address so pop the last ledger entry, accumulate
            // the total of all the TxIns and put it back to the ledger
            assert(tx.txIns().size() > 0);
            assert(tx.txIns().at(0).txOutPt().address.has_value());

            if (!tx.isCoinbase()
                && *(tx.txIns().at(0).txOutPt().address) == address)
            {
                double txInTotal = std::accumulate(
                    tx.txIns().begin(), tx.txIns().end(), 0.0,
                    [](auto accum, const ash::TxIn& txin)
                    {
                        return accum + *(txin.txOutPt().amount);
                    });

                auto amount = txInTotal - ledger.back().amount;
                ledger.pop_back();
                ledger.push_back(
                    LedgerInfo{ block.index(), tx.id(), block.time(), amount * -1.0 });
            }

        }
    }

    return ledger;
}

TxResult CreateTransaction(Blockchain& chain, std::string_view senderPK, std::string_view receiver, double amount)
{
    // first get the address of the sender from the privateKey
    const auto senderAddress = ash::crypto::GetAddressFromPrivateKey(senderPK);

    // now get all of the unspent txouts of the sender
    auto senderUnspentList = ash::GetUnspentTxOuts(chain, senderAddress);
    if (senderUnspentList.size() == 0)
    {
        return TxResult::TXOUTS_EMPTY;
    }

    double currentAmount = 0;
    double leftoverAmount = 0;
    ash::UnspentTxOuts includedUnspentOuts;

    for (const auto& unspent : senderUnspentList)
    {
        assert(unspent.amount.has_value());

        includedUnspentOuts.push_back(unspent);
        currentAmount += *(unspent.amount);
        if (currentAmount >= amount)
        {
            leftoverAmount = currentAmount - amount;
            break;
        }
    }

    if (currentAmount < amount)
    {
        return TxResult::INSUFFICIENT_FUNDS;
    }

    ash::Transaction tx;
    auto& txins = tx.txIns();

    for (const auto& uout : includedUnspentOuts)
    {
        // TODO: Signature!!!
        txins.emplace_back(uout.blockIndex,
                           uout.txIndex, uout.txOutIndex, "signature");
    }

    auto& outs = tx.txOuts();
    outs.emplace_back(receiver, amount);
    if (leftoverAmount > 0)
    {
        outs.emplace_back(senderAddress, leftoverAmount);
    }

    chain.queueTransaction(std::move(tx));

    return TxResult::SUCCESS;
}

// TODO: The implementation of this should be improved to be faster
// perhaps with a persisted index or something
std::optional<TxPoint> FindTransaction(const Blockchain& chain, std::string_view txid)
{
    for (const auto& block: chain)
    {
        for (const auto& txitem : block.transactions() | boost::adaptors::indexed())
        {
            const auto& tx = txitem.value();
            if (tx.id() == txid)
            {
                return TxPoint{ block.index(), txitem.index() };
            }
        }
    }

    return {};
}

Block GetBlockDetails(const Blockchain& chain, std::size_t index)
{
    const auto chainsize = chain.size();
    Block retblock = chain.at(index); // block copy!

    // loops through the transactions of the block we're
    // interested in
    for (auto& tx : retblock.transactions())
    {
        // for each TxIn of the block we want to fill in
        // some missing information
        for (auto& txin : tx.txIns())
        {
            const auto& txpt = txin.txOutPt();
            assert(txpt.blockIndex < chainsize);

            const auto& txs = chain.at(txpt.blockIndex).transactions();
            assert(txpt.txIndex < txs.size());
            const auto& tx = txs.at(txpt.txIndex);
            assert(txpt.txOutIndex < tx.txOuts().size());
            const auto& txout = tx.txOuts().at(txpt.txOutIndex);

            txin.txOutPt().address = txout.address();
            txin.txOutPt().amount = txout.amount();
        }
    }

    return retblock;
}

//*** Blockchain
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
                    //retval.emplace_back(UnspentTxOut{block.index(), tx.id(), address.data(), txout.amount()});
                }
            }

            // now scrub out the txins
            for (const auto& txin : tx.txIns())
            {
                //auto it = std::find_if(retval.begin(), retval.end(),
                //    [txin = txin](const unspenttxout& utxout)
                //    {
                //        return (utxout.blockindex == txin.txoutpt().txoutindex
                //            && utxout.txoutid == txin.txoutpt().txoutid);
                //    });

                //if (it != retval.end()) retval.erase(it);
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
        if (tx.isCoinbase()) continue;
        _txQueue.push(tx); // copy!!
        count++;
    }

    return count;
}

} // namespace