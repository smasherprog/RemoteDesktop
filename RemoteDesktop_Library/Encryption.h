#ifndef ENCRYPTION123_H
#define ENCRYPTION123_H
#include <memory>

#include "cryptopp/fhmqv.h"
#include "cryptopp/eccrypto.h"
using CryptoPP::ECP;
using CryptoPP::FHMQV;

#include "cryptopp/secblock.h"
using CryptoPP::SecByteBlock;

namespace RemoteDesktop{
	class Encryption{
	public:

		Encryption();
		~Encryption();
		void clear();
		void clear_keyexchange();
		void Init(bool client);
		std::unique_ptr<FHMQV<ECP>::Domain> fhmqv;
		SecByteBlock staticprivatekey, staticpublickey, ephemeralprivatekey, ephemeralpublickey, AESKey;
	};
}


#endif