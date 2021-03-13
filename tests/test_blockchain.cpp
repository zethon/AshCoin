#include <iterator>
#include <fstream>
#include <streambuf>
#include <memory>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/algorithm/string.hpp>

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

std::ostream& operator<<(std::ostream& out, const ash::LedgerInfo& li)
{
    out << fmt::format("{{{},{},{}}}", li.blockIdx, li.txid, li.amount);
    return out;
}

std::ostream& operator<<(std::ostream& out, const ash::AddressLedger& ledger)
{
    out << '{';
    std::copy(ledger.begin(), ledger.end(),
              std::ostream_iterator<ash::LedgerInfo>(out,","));
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

ash::Blockchain LoadBlockchain(std::string_view chainfile)
{
    const std::string filename = fmt::format("{}/tests/data/{}", ASH_SRC_DIRECTORY, chainfile);
    const std::string rawjson = LoadFile(filename);
    nl::json json = nl::json::parse(rawjson, nullptr, false);
    BOOST_TEST(!json.is_discarded());
    return json["blocks"].get<ash::Blockchain>();
}

BOOST_AUTO_TEST_SUITE(block)

BOOST_AUTO_TEST_CASE(LoadChainFromJson)
{
    const auto chain = LoadBlockchain("blockchain2.json");
    BOOST_TEST(chain.size() == 2);

    BOOST_TEST(chain.at(0).transactions().size() == 1);
    BOOST_TEST(chain.at(1).transactions().size() == 2);
}

BOOST_AUTO_TEST_CASE(GetAllUnspentTxOutsTest)
{
    auto chain = LoadBlockchain("blockchain2.json");
    BOOST_TEST(chain.size() == 2);

    const auto unspent = ash::GetUnspentTxOuts(chain);
    BOOST_TEST(unspent.size() == 3);
}

BOOST_AUTO_TEST_CASE(GetAddressUnspentTxOutsTest)
{
    auto chain = LoadBlockchain("blockchain2.json");
    BOOST_TEST(chain.size() == 2);

    const auto addyUnspent = ash::GetUnspentTxOuts(chain, "1LahaosvBaCG4EbDamyvuRmcrqc5P2iv7t");
    BOOST_TEST(addyUnspent.size() == 2);

    const auto stefanUnspent = ash::GetUnspentTxOuts(chain, "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8");
    BOOST_TEST(stefanUnspent.size() == 1);
}

BOOST_AUTO_TEST_CASE(GetAddressLedgerTest)
{
    auto ledgerSort = [](const ash::LedgerInfo& x, const ash::LedgerInfo& y)
        {
            return x.blockIdx < y.blockIdx
                && x.txid < y.txid
                && x.time < y.time;
        };

    constexpr auto dataString = "[{\"amount\":40,\"blockid\":1,\"time\":1615043710832,\"txid\":\"3f7e575f42ffde39ef9d41f96851b7c6be8e352c079023abf5d0ea671d61a09d\"},{\"amount\":0.003,\"blockid\":2,\"time\":1615341058227,\"txid\":\"da2f4994ba47f596c264fbfa0ea6ef8b576f8c5f43a34e3e756ccfc69adcc789\"},{\"amount\":0.002,\"blockid\":2,\"time\":1615341058227,\"txid\":\"8ddc7d1aa03ab1e7692c32f2fe85293e5343f6fefff8d5a381e93384a134393a\"},{\"amount\":-0.001,\"blockid\":3,\"time\":1615375836922,\"txid\":\"78348ae3273195a3b1d0fb974f608be165d8498cf6b333594a7b761e3e51f86d\"},{\"amount\":-0.04999999999999716,\"blockid\":3,\"time\":1615375836922,\"txid\":\"f30da564d3839b25e2bdad1b056eabb1185276a7736eed0e2e8448d9ff3df562\"}]";
    nl::json json = nl::json::parse(dataString, nullptr, false);
    assert(!json.is_discarded());
    assert(json.is_array() && json.size() > 0);

    auto dataLedger = json.get<ash::AddressLedger>();
    BOOST_TEST(dataLedger.size() == 5); // sanity check
    std::sort(dataLedger.begin(), dataLedger.end(), ledgerSort);

    const auto chain = LoadBlockchain("blockchain4.json");
    BOOST_TEST(chain.size() == 4);

    auto ledger = ash::GetAddressLedger(chain, "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8");
    BOOST_TEST(ledger.size() == 5);
    std::sort(ledger.begin(), ledger.end(), ledgerSort);

    BOOST_TEST(dataLedger == ledger, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(GetAddressBalanceTest)
{
    auto chain = LoadBlockchain("blockchain4.json");
    BOOST_TEST(chain.size() == 4);

    auto addyBalance = ash::GetAddressBalance(chain, "1LahaosvBaCG4EbDamyvuRmcrqc5P2iv7t");
    BOOST_TEST(addyBalance == 185.594, boost::test_tools::tolerance(0.0001));

    auto stefanBalance = ash::GetAddressBalance(chain, "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8");
    BOOST_TEST(stefanBalance == 39.954, boost::test_tools::tolerance(0.0001));

    auto henryBalance = ash::GetAddressBalance(chain, "1KHEXSmHaLtz4v8XrHegLzyVuU6SLg7Atw");
    BOOST_TEST(henryBalance == 2.452, boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_CASE(SingleQueueTransactionTest)
{
    auto chain = LoadBlockchain("blockchain1.json");
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

BOOST_AUTO_TEST_CASE(InsufficientFundsQueueTransactionTest)
{
    auto chain = LoadBlockchain("blockchain1.json");
    BOOST_TEST(chain.size() == 1);

    auto result = ash::QueueTransaction(chain, "1b3f78b45456dcfc3a2421da1d9961abd944b7e8a7c2ccc809a7ea92e200eeb1h",
                                        "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8", 1000000.0);
    BOOST_TEST((result == ash::TxResult::INSUFFICIENT_FUNDS));
    BOOST_TEST(chain.transactionQueueSize() == 0);
}

BOOST_AUTO_TEST_CASE(YxOutsEmptyQueueTransactionTest)
{
    auto chain = LoadBlockchain("blockchain1.json");
    BOOST_TEST(chain.size() == 1);

    auto result = ash::QueueTransaction(chain, "362116d38976078659ae158f6c21bcda40f75d4a8aa7f0a4ffbe56a48cacb93h",
                                        "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8", 1000000.0);
    BOOST_TEST((result == ash::TxResult::TXOUTS_EMPTY));
    BOOST_TEST(chain.transactionQueueSize() == 0);
}

BOOST_AUTO_TEST_CASE(NOOPQueueTransactionTest)
{
    auto chain = LoadBlockchain("blockchain1.json");
    BOOST_TEST(chain.size() == 1);

    auto result = ash::QueueTransaction(chain, "b2dfbfd974bcf876ac64a21aadab1cbb0b0350fb41115a3f61de3bd76c6485eah",
                                        "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8", 1000000.0);
    BOOST_TEST((result == ash::TxResult::NOOP_TRANSACTION));
    BOOST_TEST(chain.transactionQueueSize() == 0);
}

BOOST_AUTO_TEST_CASE(FindTransactionTest)
{
    const auto chain = LoadBlockchain("blockchain4.json");
    BOOST_TEST(chain.size() == 4);

    auto tx1 = ash::FindTransaction(chain, "78348ae3273195a3b1d0fb974f608be165d8498cf6b333594a7b761e3e51f86d");
    BOOST_TEST(tx1.has_value());
    auto [blockIndex, txIndex] = *tx1;
    BOOST_TEST(chain.size() > blockIndex);
    BOOST_TEST(chain.at(blockIndex).transactions().size() > txIndex);
    const auto& tx = chain.at(blockIndex).transactions().at(txIndex);
    const auto& txoutpt = tx.txIns().at(0).txOutPt();
    BOOST_TEST(txoutpt.blockIndex == 2);
    BOOST_TEST(txoutpt.txIndex == 5);
    BOOST_TEST(txoutpt.txOutIndex == 0);
}

BOOST_AUTO_TEST_CASE(NotFindTransactionTest)
{
    const auto chain = LoadBlockchain("blockchain4.json");
    BOOST_TEST(chain.size() == 4);

    auto tx2 = ash::FindTransaction(chain, "THISTRANSACTIONDOESNOTEXIST");
    BOOST_TEST(!tx2.has_value());
}

BOOST_AUTO_TEST_CASE(GetBlockDetailsTest)
{
    constexpr auto TestBlockIndex = 3u;
    constexpr auto TxIndex = 1u;

    const auto chain = LoadBlockchain("blockchain4.json");
    BOOST_TEST(chain.size() == 4);

    const ash::Transaction& tx = chain.txAt(TestBlockIndex, TxIndex);
    BOOST_TEST(tx.txIns().size() == 1);
    BOOST_TEST(tx.txOuts().size() == 2);

    const ash::TxOutPoint& txOutPt = tx.txIns().at(0).txOutPt();
    BOOST_TEST(txOutPt.blockIndex == 2);
    BOOST_TEST(txOutPt.txIndex == 5);
    BOOST_TEST(txOutPt.txOutIndex == 0);
    BOOST_TEST(!txOutPt.address.has_value());
    BOOST_TEST(!txOutPt.amount.has_value());

    auto block = ash::GetBlockDetails(chain, TestBlockIndex);
    const ash::TxOutPoint& txOutPt2 = block.transactions().at(TxIndex).txIns().at(0).txOutPt();
    BOOST_TEST(txOutPt2.blockIndex == 2);
    BOOST_TEST(txOutPt2.txIndex == 5);
    BOOST_TEST(txOutPt2.txOutIndex == 0);
    BOOST_TEST(txOutPt2.address.has_value());
    BOOST_TEST(boost::iequals(*(txOutPt2.address), "1Cus7TLessdAvkzN2BhK3WD3Ymru48X3z8"));
    BOOST_TEST(txOutPt2.amount.has_value());
    BOOST_TEST(*(txOutPt2.amount) == 0.003, boost::test_tools::tolerance(0.0001));
}

BOOST_AUTO_TEST_SUITE_END() // block