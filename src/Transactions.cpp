#include <nlohmann/json.hpp>

#include "Transactions.h"

#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

namespace nl = nlohmann;

namespace ash
{

void to_json(nl::json& j, const TxOutPoint& pt)
{
    j["blockIndex"] = pt.blockIndex;
    j["txIndex"] = pt.txIndex;
    j["txOutIndex"] = pt.txOutIndex;

    if (pt.address.has_value())
    {
        j["address"] = *(pt.address);
    }

    if (pt.amount.has_value())
    {
        j["amount"] = *(pt.amount);
    }
}

void to_json(nl::json& j, const TxIn& tx)
{
    j["txOutPt"] = tx.txOutPt();
    j["signature"] = tx.signature();
}

void to_json(nl::json& j, const TxIns& txins)
{
    for (const auto& txin : txins)
    {
        j.push_back(txin);
    }
}

void to_json(nl::json& j, const TxOut& txout)
{
    j["address"] = txout.address();
    j["amount"] = txout.amount();
}

void to_json(nl::json& j, const TxOuts& txouts)
{
    for (const auto& txout : txouts)
    {
        j.push_back(txout);
    }
}

void to_json(nl::json& j, const Transaction& tx)
{
    j["id"] = tx.id();
    j["inputs"] = tx.txIns();
    j["outputs"] = tx.txOuts();
}

void to_json(nl::json& j, const Transactions& txs)
{
    for (const auto& tx : txs)
    {
        j.push_back(tx);
    }
}

void from_json(const nl::json& j, TxOutPoint& pt)
{
    j["blockIndex"].get_to(pt.blockIndex);
    j["txIndex"].get_to(pt.txIndex);
    j["txOutIndex"].get_to(pt.txOutIndex);

    if (j.contains("address"))
    {
        pt.address = j["address"].get<std::string>();
    }

    if (j.contains("amount"))
    {
        pt.amount = j["amount"].get<double>();
    }
}

void from_json(const nl::json& j, TxIn& txin)
{
    txin._txOutPt = j["txOutPt"].get<TxOutPoint>();
    j["signature"].get_to(txin._signature);
}

void from_json(const nl::json& j, TxIns& txins)
{
    txins.clear();

    for (const auto& jtx : j.items())
    {
        txins.push_back(jtx.value().get<TxIn>());
    }
}

void from_json(const nl::json& j, TxOut& txout)
{
    j["address"].get_to(txout._address);
    j["amount"].get_to(txout._amount);
}

void from_json(const nl::json& j, TxOuts& txouts)
{
    txouts.clear();

    for (const auto& jtx : j.items())
    {
        txouts.push_back(jtx.value().get<TxOut>());
    }
}

void from_json(const nl::json& j, Transaction& tx)
{
    j["id"].get_to(tx._id);
    j["inputs"].get_to(tx._txIns);
    j["outputs"].get_to(tx._txOuts);
}

void from_json(const nl::json& j, Transactions& txs)
{
    txs.clear();

    for (const auto& jtx : j.items())
    {
        txs.push_back(jtx.value().get<Transaction>());
    }
}

void to_json(nl::json& j, const UnspentTxOuts& outs)
{
    for (const auto& tx : outs)
    {
        j.push_back(tx);
    }
}

void from_json(const nl::json& j, UnspentTxOuts& outs)
{
    outs.clear();

    for (const auto& jtx : j.items())
    {
        outs.push_back(jtx.value().get<UnspentTxOut>());
    }
}

std::string GetTransactionId(const Transaction& tx, std::uint64_t blockid)
{
    std::stringstream ss;
    for (const auto& txin : tx.txIns())
    {
        ss << txin.txOutPt().blockIndex
            << txin.txOutPt().txIndex
            << txin.txOutPt().txOutIndex;
    }

    for (const auto& txout : tx.txOuts())
    {
        ss << txout.address() << txout.amount();
    }

    ss << blockid;

    std::string digest;
    CryptoPP::SHA256 hash;

    CryptoPP::StringSource src(ss.str(), true,
        new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(digest), false)));

    return digest;
}

Transaction CreateCoinbaseTransaction(std::uint64_t blockIdx, std::string_view address)
{
    Transaction tx;
    tx.txIns().emplace_back(blockIdx, 0, 0);
    tx.txOuts().emplace_back(address, COINBASE_REWARD);
    tx._id = GetTransactionId(tx, blockIdx);
    return tx;
}

void Transaction::calcuateId(std::uint64_t blockid)
{
    _id = ash::GetTransactionId(*this, blockid);
}

} // namespace ash
