#include <iostream>

#include "BlockChain.h"

// options: mine, validate, sync

int main()
{
    ash::Blockchain bChain;

    std::cout << "Mining block 1..." << std::endl;
    bChain.AddBlock(ash::Block(1, "Block 1 Data"));

    std::cout << "Mining block 2..." << std::endl;
    bChain.AddBlock(ash::Block(2, "Block 2 Data"));

    std::cout << "Mining block 3..." << std::endl;
    bChain.AddBlock(ash::Block(3, "Block 3 Data"));

    return 0;
}