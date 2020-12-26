#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <nlohmann/json.hpp>

#include "../src/Block.h"
#include "../src/Blockchain.h"
#include "../src/Miner.h"

namespace nl = nlohmann;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(block)

constexpr std::string_view tx1 = 
    R"({"data":"HenryCoin Genesis 2020-12-25 21:45:22 Eastern Standard Time.","difficulty":1,"hash":"18496b380c9de32f4538c0b47ec7f5eeb1b387d7cff2b346f9150d22e0afeb12","index":0,"miner":"","nonce":0,"prev":"","time":1608950722344,"transactions":[{"id":"transaction0","inputs":[{"txOutId":"","txOutIndex":0,"signature":""}],"outputs":[{"address":"Stefan","amount":10}]},{"id":"transaction1","inputs":[{"txOutId":"transaction0","txOutIndex":57,"signature":""}],"outputs":[{"address":"Henry","amount":4},{"address":"Addy","amount":3},{"address":"Stefan","amount":1}]}]})";

BOOST_AUTO_TEST_CASE(getUnspentTxOutsTest)
{
    nl::json json = nl::json::parse(tx1, nullptr, false);
    BOOST_TEST(!json.is_discarded());

    auto block = json.get<ash::Block>();
    auto unspent = ash::GetUnspentTxOuts(block);
    BOOST_TEST(unspent.size() > 0);
}


// 1 block w/1 transaction: https://pastebin.com/vvCp1iHT
constexpr std::string_view blockjson = 
    R"x({"data":"HenryCoin Genesis 2020-12-24 13:03:12 EST.","difficulty":1,"hash":"be38cd7b2569fad57eccb732f220f4487c4ad60c52e44c0942fc21b684b2c90d","index":0,"miner":"","nonce":0,"prev":"","time":1608832992516,"transactions":[{"id":"c989408a2b064a29845c60499410e39cb0005427cfb973c38017d1b2d74b477f","inputs":[{"outid":"","outindex":0,"signature":""}],"outputs":[{"address":"ASH_TEST_PUBLIC_KEY","amount":57.2718281828}]}]})x";

BOOST_AUTO_TEST_CASE(jsonLoading)
{
    nl::json json = nl::json::parse(blockjson, nullptr, false);
    BOOST_TEST(!json.is_discarded());
    auto block = json.get<ash::Block>();
    BOOST_TEST(block.data() == "HenryCoin Genesis 2020-12-24 13:03:12 EST.");
}

BOOST_AUTO_TEST_CASE(chainValidity)
{
    ash::Blockchain blockchain;
    BOOST_TEST(blockchain.isValidChain());


}

BOOST_AUTO_TEST_SUITE_END() // block