#include <iostream>

#include <boost/program_options.hpp>

#include <nlohmann/json.hpp>

#include "BlockChain.h"

namespace po = boost::program_options;

// options: mine, validate, sync

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,?", "print help message")
        ("version,v", "print version string")
        ("chain,n", po::value<std::string>(), "the block chain folder file")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") > 0)
    {
        std::cout << desc << '\n';
        return 0;
    }


    ash::Blockchain bChain;

    std::cout << "Mining block 1..." << std::endl;
    bChain.AddBlock(ash::Block(1, "Block 1 Data"));

    std::cout << "Mining block 2..." << std::endl;
    bChain.AddBlock(ash::Block(2, "Block 2 Data"));

    std::cout << "Mining block 3..." << std::endl;
    bChain.AddBlock(ash::Block(3, "Block 3 Data"));

    return 0;
}