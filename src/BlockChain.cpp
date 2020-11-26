#include "Blockchain.h"

namespace ash
{

void to_json(nl::json& j, const Blockchain& b)
{
    for (const auto& block : b._blocks)
    {
        j["blocks"].push_back(block);
    }
}

void from_json(const nl::json& j, Blockchain& b)
{
    b.clear();

    if (!j.contains("blocks")) return;
    if (!j["blocks"].is_array()) return;

    for (const auto& jblock : j["blocks"].items())
    {
        b._blocks.push_back(jblock.value().get<Block>());
    }
}

Blockchain::Blockchain(std::uint32_t difficulty)
    : _difficulty{ difficulty }
{
}

void Blockchain::AddBlock(Block& newblock)
{
    newblock.setPrevious(_blocks.back().hash());
    newblock.MineBlock(_difficulty);
    _blocks.push_back(newblock);
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
        return false;
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

} // namespace