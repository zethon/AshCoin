#include "BlockChain.h"

namespace ash
{

Blockchain::Blockchain()
{
    _vChain.emplace_back(Block(0, "Genesis Block"));
    _nDifficulty = 2;
}

void Blockchain::AddBlock(Block bNew)
{
    bNew.setPrevious(_GetLastBlock().hash());
    bNew.MineBlock(_nDifficulty);
    _vChain.push_back(bNew);
}

Block Blockchain::_GetLastBlock() const
{
    return _vChain.back();
}

} // namespace