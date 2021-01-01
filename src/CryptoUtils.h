#pragma once
#include <string_view>

#include <cryptopp/integer.h>

namespace ash
{

namespace crypto
{

// SHA256 of generic data
std::string SHA256(std::string_view data);

// given a hex string private key this returns the 
// "04" prepended uncompressed public key
std::string GetPublicKey(std::string_view privateKeyStr);

// give a hex string private key this returns the 
// base58 public address
std::string GetAddressFromPrivateKey(std::string_view privateKeyStr);

// generate a new private key
std::string GeneratePrivateKey();

} // namespace ash::crypto

} // namespace ash
