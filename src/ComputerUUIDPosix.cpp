#include <boost/asio.hpp>
#include <iostream>
#include "sha256.h"
#include "ComputerUUID.h"

namespace utils
{

// eventually this should get the mac address and do 
// some other cool stuff, but for now we'll just use
// a hash of the computer's hostname and a pointer
// to *this, so that ht
std::string ComputerUUID::getUUID()
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

    return sha256(ss.str());
}

} // namespace utils
