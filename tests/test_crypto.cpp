#include <iterator>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/range/adaptor/indexed.hpp>

#include <range/v3/all.hpp>

#include <nlohmann/json.hpp>

#include "../src/Block.h"
#include "../src/Blockchain.h"
#include "../src/Miner.h"
#include "../src/CryptoUtils.h"

namespace nl = nlohmann;
namespace data = boost::unit_test::data;

using namespace std::string_view_literals;

BOOST_AUTO_TEST_SUITE(crypto)

BOOST_AUTO_TEST_CASE(base58Encode)
{
    // const auto encoded = ash::crypto::Base58Encode("00010966776006953D5567439E5E39F86A0D273BEED61967E3"sv);
    // BOOST_TEST(encoded == "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjv2");
}

BOOST_AUTO_TEST_CASE(cryptoKeyTest)
{
    using namespace boost::algorithm;

    const auto publicKey = ash::crypto::GetPublicKey(R"x(18E14A7B6A307F426A94F8114701E7C8E774E7F9A47E2C2035DB29A206321725)x");
    BOOST_TEST(to_upper_copy(publicKey) == "0450863AD64A87AE8A2FE83C1AF1A8403CB53F53E486D8511DAD8A04887E5B23522CD470243453A299FA9E77237716103ABC11A1DF38855ED6F2EE187E9C582BA6");

    const auto publicKeyHash = ash::crypto::SHA256HexString(publicKey);
    BOOST_TEST(to_upper_copy(publicKeyHash) == "600FFE422B4E00731A59557A5CCA46CC183944191006324A447BDB2D98D4B408");

    const auto ripeHash = ash::crypto::RIPEMD160HexString(publicKeyHash);
    BOOST_TEST(to_upper_copy(ripeHash) == "010966776006953D5567439E5E39F86A0D273BEE");

    const auto step4 = "00"s + ripeHash;
    auto networkBytesHash = ash::crypto::SHA256HexString(step4);
    BOOST_TEST(to_upper_copy(networkBytesHash) == "445C7A8007A93D8733188288BB320A8FE2DEBD2AE1B47F0F50BC10BAE845C094");

    networkBytesHash = ash::crypto::SHA256HexString(networkBytesHash);
    BOOST_TEST(to_upper_copy(networkBytesHash) == "D61967F63C7DD183914A4AE452C9F6AD5D462CE3D277798075B107615C1A8A30");

    const auto step7 = networkBytesHash.substr(0, 8);
    BOOST_TEST(to_upper_copy(step7) == "D61967F6");

    const auto step8 = step4 + step7;
    BOOST_TEST(to_upper_copy(step8) == "00010966776006953D5567439E5E39F86A0D273BEED61967F6");

    const std::string step8h = step8 + "h";
    CryptoPP::Integer step9 { step8h.data() };
    const auto address = "1"s + ash::crypto::Base58Encode(step9);
    BOOST_TEST(address == "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM");
}

BOOST_AUTO_TEST_SUITE_END() // crypto