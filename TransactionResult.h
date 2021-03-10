#pragma once

#include <stdexcept>

namespace ash
{

enum class TxResult
{
    SUCCESS = 0,
    INSUFFICIENT_FUNDS,
    TXOUTS_EMPTY
};

TxResult

class TxResultValue
{
    const TxResult  _value;

public:
    static std::string ToString(TxResult result)
    {
        switch (result)
        {
            default:
                throw std::runtime_error("unknown TxResult value");

            case TxResult::SUCCESS:
                return "success";

            case TxResult::INSUFFICIENT_FUNDS:
                return "insufficient_funds";

            case TxResult::TXOUTS_EMPTY:
                return "txouts_empty";
        }



    }

    static TxResult FromString(const std::string& str)
    {

    }

    explicit TxResultValue(TxResult val)
        : _value { val }
    {
    }

    explicit TxResultValue(const std::string& val)
        : _
};

} // namespace ash