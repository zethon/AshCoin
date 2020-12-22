#include <nlohmann/json.hpp>

#include "Transactions.h"

namespace nl = nlohmann;

namespace ash
{

void to_json(nl::json& j, const TxIn& tx)
{
    j["outid"] = tx.txtOutId();
    j["outindex"] = tx.txtOutIndex();
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
    j["id'"] = tx.id();
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



} // namespace ash
