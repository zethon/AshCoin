#pragma once
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace ash
{

constexpr double COINBASE_REWARD = 57.0;

class TxIn;
class TxOut;
class Transaction;

using TxIns = std::vector<TxIn>;
using TxOuts = std::vector<TxOut>;
using Transactions = std::vector<Transaction>;

void to_json(nl::json& j, const Transactions& txs);

class TxIn
{
    std::string     _txtOutId;
    std::uint64_t   _txOutIndex;
    std::string     _signature;

    friend void read_data(std::istream& stream, TxIn& txin); 

public:

    std::string txtOutId() const { return _txtOutId; }
    std::uint64_t txtOutIndex() const noexcept { return _txOutIndex; }
    std::string signature() const { return _signature; }
};

class TxOut
{
    std::string _address;
    double      _amount;

    friend void read_data(std::istream& stream, TxOut& txout);

public:

    std::string address() const { return _address; }
    double amount() const noexcept { return _amount; }
};

class Transaction
{
    std::string _id;
    TxIns       _txIns;
    TxOuts      _txOuts;

public:

    std::string id() const { return _id; }
    const TxIns& txIns() const { return _txIns; }
    TxIns& txIns()
    {
        return const_cast<TxIns&>(
            (static_cast<const Transaction*>(this))->txIns());
    }

    const TxOuts& txOuts() const { return _txOuts; }
    TxOuts& txOuts()
    {
        return const_cast<TxOuts&>(
            (static_cast<const Transaction*>(this))->txOuts());
    }

};

} // namespace ash
