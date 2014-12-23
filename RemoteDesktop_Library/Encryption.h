#ifndef ENCRYPTION123_H
#define ENCRYPTION123_H
#include <memory>

namespace RemoteDesktop{
	class Encryption_Impl;
	class Encryption{
		
		void clear();
		void clear_keyexchange();
		std::unique_ptr<Encryption_Impl> _Encryption_Impl;

	public:

		Encryption();
		~Encryption();
	
		void Init(bool client);
		bool Agree(const char *staticOtherPublicKey, const char *ephemeralOtherPublicKey, bool usepreaes);
		bool Decrypt(char* in_data, char* out_data, int insize, char* iv);
		int Ecrypt(char* in_data, char* out_data, int insize, char* iv);//size will be rounded up to nearest 16 byte chunk. Encryption is in place!

		int get_StaticPublicKeyLength() const;
		int get_EphemeralPublicKeyLength() const;
		const char* get_Static_PublicKey() const; 
		const char* get_Ephemeral_PublicKey() const;
		void set_AES_Key(const char* k);
	};
};


#endif