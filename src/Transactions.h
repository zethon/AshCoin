#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <chrono>

#include <boost/functional/hash.hpp>

#include <fmt/core.h>

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

void to_json(nl::json& j, const Transaction& tx);
void from_json(const nl::json& j, Transaction& tx);

void to_json(nl::json& j, const Transactions& txs);
void from_json(const nl::json& j, Transactions& txs);

void to_json(nl::json& j, const UnspentTxOuts& txout);
void from_json(const nl::json& j, UnspentTxOuts& txout);

enum class TxResult
{
    SUCCESS = 0,
    INSUFFICIENT_FUNDS,
    TXOUTS_EMPTY
};

class TxResultValue
{
    const TxResult  _value;
public:
    static std::string ToString(TxResult v)
    {
        switch (v)
        {
            default:
                throw std::runtime_error("unknown TxResult");

            case TxResult::SUCCESS:
                return "success";

            case TxResult::INSUFFICIENT_FUNDS:
                return "insufficient_fund";

            case TxResult::TXOUTS_EMPTY:
                return "txouts_empty";
        }

        assert(false);
        return {}; // should never happen
    }

    static TxResult FromString(std::string_view str)
    {
        if (str == "success")
        {
            return TxResult::SUCCESS;
        }
        else if (str == "insufficient_funds")
        {
            return TxResult::INSUFFICIENT_FUNDS;
        }
        else if (str == "txouts_empty")
        {
            return TxResult::TXOUTS_EMPTY;
        }

        throw std::runtime_error(fmt::format("unknown TxResult '{}'", str));
    }

    explicit TxResultValue(TxResult v)
        : _value { v }
    {
    }

    explicit TxResultValue(std::string_view str)
        : _value { FromString(str) }
    {
    }

    operator std::string() const
    {
        return TxResultValue::ToString(_value);
    }

    operator TxResult() const
    {
        return _value;
    }
};

Transaction CreateCoinbaseTransaction(std::uint64_t blockIdx, std::string_view address);

struct TxOutPoint
{
    std::uint64_t   blockIndex;    // the index of the block
    std::uint64_t   txIndex;       // the transaction id inside the block
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

    TxIn(std::uint64_t blockindex, std::uint64_t txIndex, std::uint64_t outidx)
        : TxIn(blockindex, txIndex, outidx, {})
    {
        // nothing to do
    }

    TxIn(std::uint64_t blockindex, std::uint64_t txIndex, std::uint64_t outidx, std::string_view signature)
        : _txOutPt{blockindex, txIndex, outidx},
          _signature{signature}
    {
        // nothing to do
    }

    TxOutPoint& txOutPt() { return _txOutPt; }
    const TxOutPoint& txOutPt() const noexcept { return _txOutPt; }

    std::string signature() const { return _signature; }
};

} // ash

namespace std
{
    template<> struct hash<ash::TxOutPoint>
    {
        std::size_t operator()(const ash::TxOutPoint& txpt) const noexcept
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, std::hash<std::uint64_t>{}(txpt.blockIndex));
            boost::hash_combine(seed, std::hash<std::uint64_t>{}(txpt.txIndex));
            boost::hash_combine(seed, std::hash<std::uint64_t>{}(txpt.txOutIndex));
            return seed;
        }
    };

    template<> struct hash<ash::TxIn>
    {
        std::size_t operator()(const ash::TxIn& txin) const noexcept
        {
            std::size_t seed = 0;
            boost::hash_combine(seed, std::hash<ash::TxOutPoint>{}(txin.txOutPt()));
            boost::hash_combine(seed, std::hash<std::uint64_t>{}(txin.txOutPt().txOutIndex));
            return seed;
        }
    };
}

namespace ash
{

class TxOut final
{

public:

    TxOut() = default;
    TxOut(std::string_view address, double amount)
        : _address{address}, _amount{amount}
    {
        // nothing to do
    }

    std::string address() const { return _address; }
    double amount() const noexcept { return _amount; }

private:
    friend void read_data(std::istream& stream, TxOut& txout);
    friend void from_json(const nl::json& j, TxOut& txout);

    std::string _address;   // public-key/address of receiver
    double      _amount;

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

    bool isCoinbase() const
    {
        assert(_txIns.size() > 0);
        assert(_txOuts.size() > 0);
        const auto& pt = _txIns.at(0).txOutPt();
        return pt.txIndex == 0
            && pt.txOutIndex == 0
            && _txIns.front().signature().empty();
    }
};

} // namespace ash
