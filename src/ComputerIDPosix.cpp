

#include <boost/asio.hpp>
#include <iostream>

#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

#include "ComputerID.h"

namespace utils
{

std::string SHA1(const std::string& data)
{
    std::string digest;
    CryptoPP::SHA1 hash;

    CryptoPP::StringSource foo(data, true,
        new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(digest), false)));

    return digest;
}

ComputerID::ComputerID() = default;

// eventually this should get the mac address and do 
// some other cool stuff, but for now we'll just use
// a hash of the computer's hostname and a pointer
// to *this, so that ht
std::string ComputerID::getUUID()
{
    namespace ip = boost::asio::ip;
    boost::asio::io_service io_service;

    std::stringstream ss;
    ss << ip::host_name();
    if (_uniquePerProcess)
    {
        ss << static_cast<void*>(this);
    }

    if (_customData.size() > 0)
    {
        ss << _customData;
    }

    return SHA1(ss.str());
}

} // namespace utils
