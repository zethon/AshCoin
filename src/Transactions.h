#pragma once
#include <string>
#include <vector>
#include <sstream>

#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace ash
{

constexpr double COINBASE_REWARD = 57.2718281828;

class TxIn;
class TxOut;
class Transaction;
struct UnspentTxOut;

using TxIns = std::vector<TxIn>;
using TxOuts = std::vector<TxOut>;
using Transactions = std::vector<Transaction>;
using UnspentTxOuts = std::vector<UnspentTxOut>;

void to_json(nl::json& j, const Transactions& txs);
void from_json(const nl::json& j, Transactions& txs);

void to_json(nl::json& j, const UnspentTxOut& txout);
void from_json(const nl::json& j, UnspentTxOut& txout);
void to_json(nl::json& j, const UnspentTxOuts& txout);
void from_json(const nl::json& j, UnspentTxOuts& txout);

Transaction CreateTransaction(std::string_view receiver, double amount, std::string_view privateKey, const UnspentTxOuts& unspentTxOuts);
Transaction CreateCoinbaseTransaction(std::uint64_t blockIdx, std::string_view address);

class TxIn final
{
    std::string     _txOutId;       
    std::uint64_t   _txOutIndex = 0;
    std::string     _signature;

    friend void read_data(std::istream& stream, TxIn& txin);
    friend void from_json(const nl::json& j, TxIn& txin);

public:
    TxIn() = default;
    TxIn(std::string_view outid, std::uint64_t outidx, std::string_view signature)
        : _txOutId{outid}, _txOutIndex{outidx}, _signature{signature}
    {
        // nothing to do
    }

    std::string txOutId() const { return _txOutId; }
    std::uint64_t txOutIndex() const noexcept { return _txOutIndex; }
    std::string signature() const { return _signature; }
};

class TxOut final
{
    std::string _address;   // public-key/address of receiver
    double      _amount;

    friend void read_data(std::istream& stream, TxOut& txout);
    friend void from_json(const nl::json& j, TxOut& txout);

public:
    TxOut() = default;
    TxOut(std::string_view address, double amount)
        : _address{address}, _amount{amount}
    {
        // nothing to do
    }

    std::string address() const { return _address; }
    double amount() const noexcept { return _amount; }
};

class Transaction final
{
    std::string _id;
    TxIns       _txIns;
    TxOuts      _txOuts;

    friend Transaction CreateCoinbaseTransaction(std::uint64_t blockIdx, std::string_view address);
    friend void from_json(const nl::json& j, Transaction& tx);
    friend void read_data(std::istream& stream, Transaction& tx);

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

struct UnspentTxOut
{
    std::string     txOutId;    // txid
    std::uint64_t   txOutIndex; // block index
    std::string     address;
    double          amount;
};

} // namespace ash
