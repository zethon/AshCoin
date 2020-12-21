#pragma once
#include <string>
#include <vector>

namespace ash
{

constexpr double COINBASE_REWARD = 57.0;

class TxIn;
class TxOut;
class Transacitons;

using TxIns = std::vector<TxIn>;
using TxOuts = std::vector<TxOut>;
using Transactions = std::vector<Transactions>;

class TxIn
{
    std::string     _txtOutId;
    std::uint64_t   _txOutIndex;
    std::string     _signature;

public:

    std::string txtOutId() const { return _txtOutId; }
    std::uint64_t txtOutIndex() const noexcept { return _txOutIndex; }
    std::string signature() const { return _signature; }
};

class TxOut
{
    std::string _address;
    double      _amount;

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
    const TxOuts& txOuts() const { return _txOuts; }

};

} // namespace ash
