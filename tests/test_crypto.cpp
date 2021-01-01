#include <iterator>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/algorithm/string.hpp>

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

std::tuple<std::string,std::string> addressGenData[] =
{
    { 
        "18E14A7B6A307F426A94F8114701E7C8E774E7F9A47E2C2035DB29A206321725",
        "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM"
    },
    { 
        "B90EA0BD18CD692D7D17B70C94F23C0D2EDFDFF444B81C46BEC6319B2E628BD2",
        "14VmSNdxunr6XWgiqzC2pxg3hsifNFfbM7"
    },
    { 
        "D37ABA89831946C9F3F4B79D227DD3E4EB2252F5F12E220962DE4FB33DBA7DB0",
        "1N7fqm3nF2KQQYzGxb47CinzRBxzh5QqXs"
    },
    { 
        "593EAE3AA66236E3F9D6A91072D3715893D7DB0797D28E4E1485D040AEB75749",
        "1GVbsBtswsoiLHzhBvTp2jyPSg9pJQ5m95"
    },
    { 
        "3A0787242C12790C625C7DAF272027B691FA5CACF60BFC6DE5898AEAB74CFD29",
        "1DyxybGMtbtNXm7tbZiPxgBa4uj7qQFY9H"
    }
};

BOOST_DATA_TEST_CASE(addressGenTest, data::make(addressGenData), pk, expected)
{
    const auto address = ash::crypto::GetAddressFromPrivateKey(pk);
    BOOST_TEST(address == expected);
}

BOOST_AUTO_TEST_SUITE_END() // crypto