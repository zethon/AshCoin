#pragma once
#include <string>

#include <boost/algorithm/string.hpp>

#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/dsa.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

#include <cryptopp/ecp.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>


namespace ash
{

namespace crypto
{

using FieldType = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>;

std::string SHA256(std::string_view data);

inline std::string GetPublicKey(std::string_view privateKeyStr)
{
    FieldType::PrivateKey privateKey;

    CryptoPP::HexDecoder decoder;
    decoder.Put(reinterpret_cast<const CryptoPP::byte*>(privateKeyStr.data()), privateKeyStr.size());
    decoder.MessageEnd();

    CryptoPP::Integer x;
    x.Decode(decoder, decoder.MaxRetrievable());

    privateKey.Initialize(CryptoPP::ASN1::secp256k1(), x);
    
    FieldType::PublicKey publicKey;
    privateKey.MakePublicKey(publicKey);

    const CryptoPP::ECP::Point& q = publicKey.GetPublicElement();

    std::stringstream ssx;
    ssx << std::hex << q.x;
    std::string qx = ssx.str();
    qx.pop_back();

    std::stringstream ssy;
    ssy << std::hex << q.y;
    std::string qy = ssy.str();
    qy.pop_back();

    return boost::algorithm::to_upper_copy("04" + qx + qy);
}







} // namespace ash::crypto

} // namespace ash
