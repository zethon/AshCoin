#include <iterator>
#include <fstream>
#include <streambuf>
#include <memory>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/range/adaptor/indexed.hpp>

#include <range/v3/all.hpp>

#include <nlohmann/json.hpp>

#include <test-config.h>

#include "../src/Block.h"
#include "../src/Blockchain.h"
#include "../src/Miner.h"
#include "../src/CryptoUtils.h"
#include "../src/Transactions.h"
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

    std::ostream& operator<<(std::ostream& out, const ash::TxOutPoint& v)
    {
        const std::string address =
                (v.address.has_value() ? *(v.address) : "null"s);

        const std::string amount =
                (v.amount.has_value() ? std::to_string(*(v.amount)) : "null"s);

        out << '{';
        out << v.blockIndex
            << ',' << v.txIndex
            << ',' << v.txOutIndex
            << ',' << address
            << ',' << amount;
        out << '}';
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const ash::UnspentTxOuts& utxouts)
    {
        out << '{';
        std::copy(utxouts.begin(), utxouts.end(), 
            std::ostream_iterator<ash::UnspentTxOut>(out,","));
        out << '}';
        return out;
    }
}

std::string LoadFile(std::string_view filename)
{
    std::ifstream t(filename.data());
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    return str;
}

BOOST_AUTO_TEST_SUITE(block)

using StringList = std::vector<std::string>;
using UnspentTestData = std::tuple<std::string, StringList>;
const UnspentTestData unspentDataBlockTest[]
{
    UnspentTestData
    {
        R"({"data":"HenryCoin Genesis 2020-12-25 21:45:22 Eastern Standard Time.","difficulty":0,"hash":"18496b380c9de32f4538c0b47ec7f5eeb1b387d7cff2b346f9150d22e0afeb12","index":57,"miner":"","nonce":0,"prev":"","time":1608950722344,"transactions":[{"id":"transaction0","inputs":[{"txOutId":"","txOutIndex":0,"signature":""}],"outputs":[{"address":"Stefan","amount":10}]},{"id":"transaction1","inputs":[{"txOutId":"transaction0","txOutIndex":57,"signature":""}],"outputs":[{"address":"Henry","amount":4},{"address":"Addy","amount":3},{"address":"Stefan","amount":1}]}]})"s,
        StringList{ "Henry"s, "Addy"s, "Stefan"s }
    },
};

//BOOST_DATA_TEST_CASE(getBlockUnspentTxOuts, data::make(unspentDataBlockTest), blockjson, expectedIds)
//{
//     nl::json json = nl::json::parse(blockjson, nullptr, false);
//     BOOST_TEST(!json.is_discarded());
//
//     auto block = json.get<ash::Block>();
//     BOOST_TEST(block.transactions().size() > 0);
//
//     const auto& tx = block.transactions().at(0);
//     BOOST_TEST(tx.txIns().size() > 0);
//     BOOST_TEST(tx.txOuts().size() > 0);
//
//     auto unspent = ash::GetUnspentTxOuts(block);
//     BOOST_TEST(unspent.size() > 0);
//     BOOST_TEST(unspent.size() == expectedIds.size());
//
//     auto expectedCopy = expectedIds; // container copy!
//     ranges::sort(expectedCopy);
//
//     auto actualUnspent = unspent
//         | ranges::views::transform([](const ash::UnspentTxOut& txout) { return txout.address; })
//         | ranges::to<std::vector>()
//         | ranges::actions::sort;
//
//     BOOST_TEST(expectedCopy == actualUnspent, boost::test_tools::per_element());
//}

//using UnspentTestChainData = std::tuple<std::string, std::string, ash::UnspentTxOuts>;
//const UnspentTestChainData unspentDataChainTest[]
//{
////    UnspentTestChainData
////    {
////        R"json([{"data":"B57","difficulty":0,"hash":"","index":57,"miner":"","nonce":0,"prev":"","time":1608950722344,"transactions":[{"id":"tx0","inputs":[{"txOutId":"","txOutIndex":0,"signature":""}],"outputs":[{"address":"Stefan","amount":10}]}]},{"data":"B58","difficulty":0,"hash":"","index":58,"miner":"","nonce":0,"prev":"","time":1608950722344,"transactions":[{"id":"tx1","inputs":[{"txOutId":"tx0","txOutIndex":57,"signature":""}],"outputs":[{"address":"Henry","amount":5},{"address":"Addy","amount":3},{"address":"Stefan","amount":2}]}]}])json"s,
////        "Stefan",
////        ash::UnspentTxOuts{ ash::UnspentTxOut{"tx1", 1, "Stefan", 1} }
////    },
//};

//// --run_test=block/getChainUnspentTxOuts
//BOOST_DATA_TEST_CASE(getChainUnspentTxOuts, data::make(unspentDataChainTest), chainjson, wallet, expectedouts)
//{
//    // nl::json json = nl::json::parse(chainjson, nullptr, false);
//    // BOOST_TEST(!json.is_discarded());
//
//    // auto chain = json.get<ash::Blockchain>();
//    // BOOST_TEST(chain.size() == 2);
//
//    // auto txouts = chain.getUnspentTxOuts("Stefan");
//    // BOOST_TEST(txouts == expectedouts, boost::test_tools::per_element());
//}

constexpr std::string_view blockjson = 
    R"x({"data":"coinbase block#13","difficulty":2,"hash":"002f0bb4639b8dd30cb37a4436d23ba85cb86afc09dfd0561869a24d8cb5cd0f","index":13,"miner":"4c5ee1d3ceb8692ebe83d7ecac1d2207051f8065d6022f5fb35fda59c51bd98f","nonce":13,"prev":"001b022fb0dc92b574fd7f516d2dcf5f7c5ee9a25c332837a017e04c5a57a06f","time":1608997664493,"transactions":[{"id":"8ab8c30a3e8061a7f4f308a10fe884e3c965b4e0607ec5ef29deea1e72e301de","inputs":[{"signature":"","txOutId":"","txOutIndex":13}],"outputs":[{"address":"TEST_PUBLIC_KEY","amount":57.2718281828}]}]})x";

BOOST_AUTO_TEST_CASE(loadChainFromJson)
{
    const std::string filename = fmt::format("{}/tests/data/blockchain2.json", ASH_SRC_DIRECTORY);
    const std::string rawjson = LoadFile(filename);
    nl::json json = nl::json::parse(rawjson, nullptr, false);
    BOOST_TEST(!json.is_discarded());

    const auto chain = json["blocks"].get<ash::Blockchain>();
    BOOST_TEST(chain.size() == 2);

    BOOST_TEST(chain.at(0).transactions().size() == 1);
    BOOST_TEST(chain.at(1).transactions().size() == 2);
}

BOOST_AUTO_TEST_CASE(singleTransaction)
{
    const std::string filename = fmt::format("{}/tests/data/blockchain1.json", ASH_SRC_DIRECTORY);
    const std::string rawjson = LoadFile(filename);
    nl::json json = nl::json::parse(rawjson, nullptr, false);
    BOOST_TEST(!json.is_discarded());

    auto chain = json["blocks"].get<ash::Blockchain>();
    BOOST_TEST(chain.size() == 1);

    auto addyBalance = ash::GetAddressBalance(chain, "1LahaosvBaCG4EbDamyvuRmcrqc5P2iv7t");
    BOOST_TEST(addyBalance == 57.00, boost::test_tools::tolerance(0.001));

    auto result = ash::QueueTransaction(chain, "1b3f78b45456dcfc3a2421da1d9961abd944b7e8a7c2ccc809a7ea92e200eeb1h",
                                        "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8", 10.0);
    BOOST_TEST((result == ash::TxResult::SUCCESS));
    BOOST_TEST(chain.transactionQueueSize() == 1);

    ash::Miner miner;
    miner.setDifficulty(0);
    BOOST_TEST(miner.difficulty() == 0);

    ash::Transactions txs;
    txs.push_back(ash::CreateCoinbaseTransaction(chain.size(), "1LahaosvBaCG4EbDamyvuRmcrqc5P2iv7t"));

    ash::Block newblock{ chain.size(), chain.back().hash(), std::move(txs) };
    BOOST_TEST(newblock.transactions().size() == 1);

    BOOST_TEST(chain.getTransactionsToBeMined(newblock) == 1);
    BOOST_TEST(chain.transactionQueueSize() == 0);
    BOOST_TEST(newblock.transactions().size() == 2);

    auto mineResult = miner.mineBlock(newblock, [](std::uint64_t) { return true; });
    BOOST_TEST(mineResult == ash::Miner::SUCCESS);

    chain.addNewBlock(newblock);
    BOOST_TEST(chain.size() == 2);

    addyBalance = ash::GetAddressBalance(chain, "1LahaosvBaCG4EbDamyvuRmcrqc5P2iv7t");
    BOOST_TEST(addyBalance == 104.00, boost::test_tools::tolerance(0.001));

    auto stefanBalance = ash::GetAddressBalance(chain, "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8");
    BOOST_TEST(stefanBalance == 10.00, boost::test_tools::tolerance(0.001));
}

BOOST_AUTO_TEST_SUITE_END() // block