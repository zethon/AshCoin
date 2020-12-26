#include <iterator>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/range/adaptor/indexed.hpp>

#include <range/v3/all.hpp>

#include <nlohmann/json.hpp>

#include "../src/Block.h"
#include "../src/Blockchain.h"
#include "../src/Miner.h"

namespace nl = nlohmann;
namespace data = boost::unit_test::data;

using namespace std::string_literals;

namespace std
{
    std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& strings)
    {
        out << '{';
        std::copy(strings.begin(), strings.end(), 
            std::ostream_iterator<std::string>(out,","));
        out << '}';
        return out;
    }
}

BOOST_AUTO_TEST_SUITE(block)

using StringList = std::vector<std::string>;
using UnspentTestData = std::tuple<std::string, StringList>;
const UnspentTestData unspentData[]
{
    UnspentTestData
    {
        R"({"data":"HenryCoin Genesis 2020-12-25 21:45:22 Eastern Standard Time.","difficulty":0,"hash":"18496b380c9de32f4538c0b47ec7f5eeb1b387d7cff2b346f9150d22e0afeb12","index":57,"miner":"","nonce":0,"prev":"","time":1608950722344,"transactions":[{"id":"transaction0","inputs":[{"txOutId":"","txOutIndex":0,"signature":""}],"outputs":[{"address":"Stefan","amount":10}]},{"id":"transaction1","inputs":[{"txOutId":"transaction0","txOutIndex":57,"signature":""}],"outputs":[{"address":"Henry","amount":4},{"address":"Addy","amount":3},{"address":"Stefan","amount":1}]}]})"s,
        StringList{ "Henry"s, "Addy"s, "Stefan"s }
    },
};

BOOST_DATA_TEST_CASE(getUnspentTxOutsTest, data::make(unspentData), blockjson, expectedIds)
{
    nl::json json = nl::json::parse(blockjson, nullptr, false);
    BOOST_TEST(!json.is_discarded());

    auto block = json.get<ash::Block>();
    BOOST_TEST(block.transactions().size() > 0);

    const auto& tx = block.transactions().at(0);
    BOOST_TEST(tx.txIns().size() > 0);
    BOOST_TEST(tx.txOuts().size() > 0);
    
    auto unspent = ash::GetUnspentTxOuts(block);
    BOOST_TEST(unspent.size() > 0);
    BOOST_TEST(unspent.size() == expectedIds.size());

    auto expectedCopy = expectedIds; // container copy!
    ranges::sort(expectedCopy);
    
    auto actualUnspent = unspent 
        | ranges::views::transform([](const ash::UnspentTxOut& txout) { return txout.address; })
        | ranges::to<std::vector>()
        | ranges::actions::sort;

    BOOST_TEST(expectedCopy == actualUnspent, boost::test_tools::per_element());
}

constexpr std::string_view blockjson = 
    R"x({"data":"HenryCoin Genesis 2020-12-24 13:03:12 EST.","difficulty":1,"hash":"be38cd7b2569fad57eccb732f220f4487c4ad60c52e44c0942fc21b684b2c90d","index":0,"miner":"","nonce":0,"prev":"","time":1608832992516,"transactions":[{"id":"c989408a2b064a29845c60499410e39cb0005427cfb973c38017d1b2d74b477f","inputs":[{"txOutId":"","txOutIndex":0,"signature":""}],"outputs":[{"address":"ASH_TEST_PUBLIC_KEY","amount":57.2718281828}]}]})x";

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