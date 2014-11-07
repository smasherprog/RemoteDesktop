#include "stdafx.h"
#include "Encryption.h"

#include "cryptopp/integer.h"
using CryptoPP::Integer;
#include "cryptopp/osrng.h"
using CryptoPP::AutoSeededRandomPool;
#include "cryptopp/aes.h"
using CryptoPP::AES;
#include "cryptopp/sha.h"
using CryptoPP::SHA256;
#include "cryptopp/gcm.h"
using CryptoPP::GCM;
#include "cryptopp/asn.h"
#include "cryptopp/oids.h"
using CryptoPP::OID;
using namespace CryptoPP::ASN1;

RemoteDesktop::Encryption::Encryption() : AESKey(SHA256::DIGESTSIZE) {}
RemoteDesktop::Encryption::~Encryption() {

}
void RemoteDesktop::Encryption::clear(){// clear everything
	clear_keyexchange();
	memset(AESKey.BytePtr(), 0, AESKey.SizeInBytes());
	fhmqv.reset();
}
void RemoteDesktop::Encryption::clear_keyexchange(){// clear only the keys after the initial key  exchange is finished, keep the AES key intact, delete the ECC domain params
	memset(ephemeralprivatekey.BytePtr(), 0, ephemeralprivatekey.SizeInBytes());
	memset(staticprivatekey.BytePtr(), 0, staticprivatekey.SizeInBytes());
	memset(staticpublickey.BytePtr(), 0, staticpublickey.SizeInBytes());
	memset(ephemeralpublickey.BytePtr(), 0, ephemeralpublickey.SizeInBytes());
	fhmqv.reset();
}
void RemoteDesktop::Encryption::Init(bool client){
	clear();// just in case
	const OID CURVE = secp256r1();
	fhmqv =std::make_shared<FHMQV<ECP>::Domain>(CURVE, client);// allocate here
	staticprivatekey.resize(fhmqv->StaticPrivateKeyLength());
	staticpublickey.resize(fhmqv->StaticPublicKeyLength());
	ephemeralprivatekey.resize(fhmqv->EphemeralPrivateKeyLength());
	ephemeralpublickey.resize(fhmqv->EphemeralPublicKeyLength());
	AutoSeededRandomPool rnd;
	fhmqv->GenerateStaticKeyPair(rnd, staticprivatekey, staticpublickey);
	fhmqv->GenerateEphemeralKeyPair(rnd, ephemeralprivatekey, ephemeralpublickey);
}