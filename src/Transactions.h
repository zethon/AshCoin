#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <chrono>

#include <boost/functional/hash.hpp>

#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace ash
{

constexpr double COINBASE_REWARD = 57.00;

// this feels wrong being in here but I don't want to 
// define it in multiple places
using BlockTime = std::chrono::time_point<
    std::chrono::system_clock, std::chrono::milliseconds>;

class TxIn;
class TxOut;
class Transaction;
struct TxOutPoint;
using UnspentTxOut = TxOutPoint;

using TxIns = std::vector<TxIn>;
using TxOuts = std::vector<TxOut>;
using Transactions = std::vector<Transaction>;
using UnspentTxOuts = std::vector<UnspentTxOut>;

struct LedgerInfo;
using AddressLedger = std::vector<LedgerInfo>;

void to_json(nl::json& j, const Transactions& txs);
void from_json(const nl::json& j, Transactions& txs);

// void to_json(nl::json& j, const UnspentTxOut& txout);
// void from_json(const nl::json& j, UnspentTxOut& txout);
void to_json(nl::json& j, const UnspentTxOuts& txout);
void from_json(const nl::json& j, UnspentTxOuts& txout);

void to_json(nl::json& j, const LedgerInfo& li);
void to_json(nl::json& j, const AddressLedger& ledger);

Transaction CreateTransaction(std::string_view receiver, double amount, std::string_view privateKey, const UnspentTxOuts& unspentTxOuts);
Transaction CreateCoinbaseTransaction(std::uint64_t blockIdx, std::string_view address);

struct TxOutPoint
{
    std::uint64_t   blockIndex;    // the index of the block
    std::string     txOutId;       // the transaction id inside the block
    std::uint64_t   txOutIndex;    // the index of the TxOut inside the transaction

    std::optional<std::string>  address;
    std::optional<double>       amount;
};

class TxIn final
{
    TxOutPoint      _txOutPt;    
    std::string     _signature;

    friend void read_data(std::istream& stream, TxIn& txin);
    friend void from_json(const nl::json& j, TxIn& txin);

public:
    TxIn() = default;

    TxIn(std::uint64_t blockindex, std::string_view outid, std::uint64_t outidx)
        : TxIn(blockindex, outid, outidx, {})
    {
        // nothing to do
    }

    TxIn(std::uint64_t blockindex, std::string_view outid, std::uint64_t outidx, std::string_view signature)
        : _txOutPt{blockindex, outid.data(), outidx},
          _signature{signature}
    {
        // nothing to do
    }

    TxOutPoint& txOutPt() { return _txOutPt; }
    const TxOutPoint& txOutPt() const noexcept { return _txOutPt; }

    // std::string txOutId() const noexcept { return _txOutPt.txOutId; }
    // std::uint64_t txOutIndex() const noexcept { return _txOutPt.txOutIndex; }

    std::string signature() const { return _signature; }
};

} // ash

namespace std
{
    template<> struct hash<ash::TxIn>
    {
        std::size_t operator()(const ash::TxIn& txin) const noexcept
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, std::hash<std::string>{}(txin.txOutPt().txOutId));
            boost::hash_combine(seed, std::hash<std::uint64_t>{}(txin.txOutPt().txOutIndex));
            return seed;
        }
    };
}

namespace ash
{

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

} // ash

namespace std
{
    template<> struct hash<ash::TxOut>
    {
        std::size_t operator()(const ash::TxOut& txout) const noexcept
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, std::hash<std::string>{}(txout.address()));
            boost::hash_combine(seed, std::hash<double>{}(txout.amount()));
            return seed;
        }
    };
}

namespace ash
{

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
    void calcuateId(std::uint64_t blockid);

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

    bool isCoinebase() const
    {
        return _txOuts.size() == 1
            && _txIns.size() == 1
            && _txIns.front().signature().empty();
    }
};

// struct UnspentTxOut
// {
//     std::uint64_t   blockIndex; // block index
//     std::string     txOutId;    // txid
//     std::string     address;
//     double          amount;
// };


} // namespace ash

namespace std
{
    template<> struct hash<ash::UnspentTxOut>
    {
        std::size_t operator()(const ash::UnspentTxOut& unspent) const noexcept
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, std::hash<std::uint64_t>{}(unspent.blockIndex));
            boost::hash_combine(seed, std::hash<std::string>{}(unspent.txOutId));
            if (unspent.address.has_value())
            {
                boost::hash_combine(seed, std::hash<std::string>{}(*(unspent.address)));
            }

            if (unspent.amount.has_value())
            {
                boost::hash_combine(seed, std::hash<double>{}(*(unspent.amount)));
            }
            
            return seed;
        }
    };
}

namespace ash
{

// TODO: Delete This?
struct LedgerInfo
{
    std::string     txid;
    std::uint64_t   blockIdx;
    BlockTime       time;
    double          amount;
};

} // namespace ash
