#include "BlockChain.h"

namespace ash
{

Blockchain::Blockchain()
{
    _vChain.emplace_back(Block(0, "Genesis Block"));
}

void Blockchain::AddBlock(Block bNew)
{
    bNew.setPrevious(_vChain.back().hash());
    bNew.MineBlock(DIFFICULTY);
    _vChain.push_back(bNew);
}

bool Blockchain::isValidBlockPair(std::size_t idx) const
{
    if (idx > _vChain.size() || idx < 1) return false;

    const auto& current = _vChain.at(idx);
    const auto& prev = _vChain.at(idx - 1);

    return (current.index() == prev.index() + 1)
        && (current.previousHash() == prev.hash())
        && (Block::CalculateHash(current) == current.hash());
}

bool Blockchain::isValidChain() const
{
    // TODO: test that the genises block is [0]

    for (auto idx = 1; idx < _vChain.size(); idx++)
    {
        if (!isValidBlockPair(idx)) return false;
    }

    return true;
}

} // namespace