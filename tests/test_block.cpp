#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

//#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include "../src/Block.h"
#include "../src/Transactions.h"
#include "../src/ChainDatabase.h"

#include "Test.h"

namespace nl = nlohmann;
namespace data = boost::unit_test::data;

using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(block)

BOOST_AUTO_TEST_CASE(empty_blockcopy)
{
    ash::Block block{ 123, "data123" };
    auto block_copy = block;
    BOOST_TEST(block.index() == block_copy.index());
    BOOST_TEST(block.data() == block_copy.data());
}

BOOST_AUTO_TEST_CASE(block_with_transaction_copy)
{
    ash::Transaction tx;
    tx.txIns().emplace_back(1,2,3,"signature");
    tx.txOuts().emplace_back("senderAddress", 12.34);

    ash::Block block{ 123, "data123" };
//    block.transactions()

}

BOOST_AUTO_TEST_SUITE_END() // db