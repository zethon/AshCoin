#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

#include "CryptoUtils.h"

namespace ash
{

namespace crypto
{

// sha256 of any string data
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

// sha256 of a hex string
std::string SHA256HexString(std::string_view data)
{
    std::string digest;
    CryptoPP::SHA256 hash;

    auto ptr = reinterpret_cast<const CryptoPP::byte*>(data.data());
    CryptoPP::StringSource temp(ptr, data.size(), true,
        new CryptoPP::HexDecoder(
            new CryptoPP::HashFilter(hash,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(digest), false))));

    return digest;
}

std::string RIPEMD160HexString(std::string_view data)
{
    std::string digest;
    CryptoPP::RIPEMD160 hash;

    auto ptr = reinterpret_cast<const CryptoPP::byte*>(data.data());
    CryptoPP::StringSource temp(ptr, data.size(), true,
        new CryptoPP::HexDecoder(
            new CryptoPP::HashFilter(hash,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(digest), false))));

    return digest;
}

constexpr std::string_view base58Alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
// std::string Base58Encode(std::string_view data)
// {
std::string Base58Encode(CryptoPP::Integer num)
  {
    std::string alphabet[58] = {"1","2","3","4","5","6","7","8","9","A","B","C","D","E","F",
    "G","H","J","K","L","M","N","P","Q","R","S","T","U","V","W","X","Y","Z","a","b","c",
    "d","e","f","g","h","i","j","k","m","n","o","p","q","r","s","t","u","v","w","x","y","z"};
    int base_count = 58; 
    std::string encoded; 
    CryptoPP::Integer div; 
    CryptoPP::Integer mod;
    while (num >= base_count)
    {
        div = num / base_count;
        mod = (num - (base_count * div));
        encoded = alphabet[ mod.ConvertToLong() ] + encoded;
        num = div;
    }
    encoded = alphabet[ num.ConvertToLong() ] + encoded;
    return encoded;
}

} // namespace ash::crypto

} // namespace ash