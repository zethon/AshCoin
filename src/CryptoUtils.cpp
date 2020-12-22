#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

#include "CryptoUtils.h"

namespace ash
{

namespace crypto
{

std::string SHA256(std::string_view data)
{
    std::string digest;
    CryptoPP::SHA256 hash;

    auto ptr = reinterpret_cast<const CryptoPP::byte*>(data.data());
    CryptoPP::StringSource temp(ptr, data.size(), true,
        new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(digest), false)));

    return digest;
}

} // namespace ash::crypto

} // namespace ash