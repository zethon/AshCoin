#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "../src/Block.h"
#include "../src/Blockchain.h"

using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(block)

// BOOST_AUTO_TEST_CASE(basicBlockchain) 
// {
//     ash::Blockchain chain;
//     BOOST_REQUIRE_EQUAL(chain.size(), 1);

//     const auto& genblock = chain.at(0);
//     BOOST_REQUIRE_EQUAL(genblock.data(), "Genesis Block");
// }

// BOOST_AUTO_TEST_CASE(chainValidity)
// {
//     ash::Blockchain chain;
//     BOOST_REQUIRE_EQUAL(chain.isValidChain(), true);
// }

// BOOST_AUTO_TEST_CASE(blockEquality)
// {
//     ash::Blockchain chain;
//     BOOST_REQUIRE_EQUAL(chain.at(0), ash::Block(0, "Genesis Block"));
//     BOOST_REQUIRE_NE(chain.at(0), ash::Block(0, "Dummy Block"));
//     BOOST_REQUIRE_NE(chain.at(0), ash::Block(1, "Genesis Block"));
// }

BOOST_AUTO_TEST_CASE(chainValidity)
{
    ash::Blockchain blockchain;
    BOOST_TEST(blockchain.isValidChain());


}

BOOST_AUTO_TEST_SUITE_END() // block