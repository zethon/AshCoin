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

BOOST_AUTO_TEST_SUITE(db)

std::vector<ash::Block> CreateBlockStore()
{
    std::vector<ash::Block> retval;
    retval.emplace_back(0, "prevhash1");
    retval.back().setData("data1");

    retval.emplace_back(1, "prevhash2");
    retval.back().setData("data2");

    retval.emplace_back(2, "prevhash3");
    retval.back().setData("data3");

    return retval;
}

std::string CreateBlockDB(const std::string& test_name)
{
    auto path = ash::tempFolder(test_name);
    {
        ash::ChainLevelDB db(path.string());
        for (auto blocks = CreateBlockStore(); auto block : blocks)
        {
            db.write(block);
        }
    }

    return path.string();
}

ash::Block createGenesisBlock()
{
    ash::Transactions txs;
    txs.push_back(ash::CreateCoinbaseTransaction(0, "REWARDADDRESS"));
    ash::Block gen{ 0, "", std::move(txs) };

//    std::time_t t = std::time(nullptr);
//    const auto gendata =
//        fmt::format("Gensis Block created {:%Y-%m-%d %H:%M:%S %Z}", *std::localtime(&t));

    gen.setData("GENESISDATA");
    return gen;
}

// add blocks in a random order and make sure that our custom
// comparator sorts the blocks in leveldb in numeric order
BOOST_AUTO_TEST_CASE(test_comparator)
{
    auto path = ash::tempFolder("test_comparator");
    ash::ChainLevelDB db(path.string());

    ash::Block block1 { 0, "", {} };
    db.write(block1);

    ash::Block block2 { 1975, "", {} };
    db.write(block2);

    ash::Block block3 { 57, "", {} };
    db.write(block3);

    ash::Block block4 { 575757, "", {} };
    db.write(block4);

    std::unique_ptr<leveldb::Iterator> itptr;
    itptr.reset(db.leveldb()->NewIterator(leveldb::ReadOptions()));

    itptr->SeekToFirst();
    BOOST_TEST(itptr->Valid());
    auto szptr = reinterpret_cast<const std::size_t*>(itptr->key().data());
    BOOST_TEST(*szptr == 0);

    itptr->Next();
    BOOST_TEST(itptr->Valid());
    szptr = reinterpret_cast<const std::size_t*>(itptr->key().data());
    BOOST_TEST(*szptr == 57);

    itptr->Next();
    BOOST_TEST(itptr->Valid());
    szptr = reinterpret_cast<const std::size_t*>(itptr->key().data());
    BOOST_TEST(*szptr == 1975);

    itptr->Next();
    BOOST_TEST(itptr->Valid());
    szptr = reinterpret_cast<const std::size_t*>(itptr->key().data());
    BOOST_TEST(*szptr == 575757);

    itptr->Next();
    BOOST_TEST(!itptr->Valid());
}

// write a couple blocks to the db and make sure we get them
// back how we want
BOOST_AUTO_TEST_CASE(test_write_read)
{
    auto db_name = CreateBlockDB("test_write_read");
    ash::ChainLevelDB db{ db_name };

    auto itptr = db.const_iteraor();
    std::size_t count = 0;

    // this is probably bad practice since we're iterating the db and already
    // have the value during our iteration, but that's ok since we want to
    // test our read function too and this is a unit test
    for (itptr->SeekToFirst(); itptr->Valid(); itptr->Next(), count++)
    {
        auto index = *(reinterpret_cast<const std::size_t*>(itptr->key().data()));
        auto block = db.read(index);
        BOOST_TEST(block.has_value());
        BOOST_TEST(block->index() == count);
        BOOST_TEST(block->previousHash() == ("prevhash" + std::to_string(count + 1)));
        BOOST_TEST(block->data() == ("data" + std::to_string(count + 1)));
    }
}

BOOST_AUTO_TEST_CASE(test_update_block)
{
    auto db_name = CreateBlockDB("test_update_block");
    ash::ChainLevelDB db{db_name};

//    ash::Block& block1 { db.read(0) };
//    block1.setData("updated1")

}

// write and entire blockchain to the database and make
// sure we can read it how we expect
BOOST_AUTO_TEST_CASE(test_write_read_blockchain)
{

}

BOOST_AUTO_TEST_CASE(test_size)
{
    auto db_name = CreateBlockDB("test_write_read");
    ash::ChainLevelDB db{ db_name };
    BOOST_TEST(db.size() == 3);

    ash::Block newblock1 { db.size(), ""};


}

BOOST_AUTO_TEST_SUITE_END() // db