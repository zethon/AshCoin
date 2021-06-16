#include <iostream>
#include <ifaddrs.h>
#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#include <boost/asio.hpp>

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

std::string getMacString()
{
    struct ifaddrs *ifaddr;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        throw std::runtime_error("call to getifaddrs failed");
    }

    int mib[6];
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;

    std::size_t len;
    int idx = 0;
    char *buf;

    struct if_msghdr    *ifm;
    struct sockaddr_dl  *sdl;
    unsigned char       *ptr;

    std::stringstream   builder;

    while (ifaddr != nullptr)
    {
        if (ifaddr->ifa_addr != nullptr)
        {
            auto family = ifaddr->ifa_addr->sa_family;

            if ((mib[5] = if_nametoindex(ifaddr->ifa_name)) == 0)
            {
                continue;
            }

            if (sysctl(mib, 6, nullptr, &len, nullptr, 0) < 0)
            {
                continue;
            }

            if ((buf = static_cast<char*>(malloc(len))) == nullptr)
            {
                continue;
            }

            if (sysctl(mib, 6, buf, &len, nullptr, 0) < 0)
            {
                continue;
            }

            ifm = (struct if_msghdr *)buf;
            sdl = (struct sockaddr_dl *)(ifm + 1);

            if (sdl->sdl_type == IFT_ETHER)
            {
                ptr = (unsigned char *)LLADDR(sdl);

                builder
                    << std::hex
                    << static_cast<int>(*ptr)
                    << ':'
                    << static_cast<int>(*(ptr + 1))
                    << ':'
                    << static_cast<int>(*(ptr + 2))
                    << ':'
                    << static_cast<int>(*(ptr + 3))
                    << ':'
                    << static_cast<int>(*(ptr + 4))
                    << ':'
                    << static_cast<int>(*(ptr + 5));
            }

            free(buf);
        }

        ++idx;
        ifaddr = ifaddr->ifa_next;
    }

    return builder.str();
}

ComputerID::ComputerID() = default;

std::string ComputerID::getUUID()
{
    namespace ip = boost::asio::ip;
    boost::asio::io_context io_service;

    std::stringstream ss;
    ss << ip::host_name()
        << getMacString();
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
