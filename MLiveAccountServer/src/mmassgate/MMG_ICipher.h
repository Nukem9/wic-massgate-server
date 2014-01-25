#pragma once

enum CipherIdentifier : uchar
{
	CIPHER_UNKNOWN,
	CIPHER_BLOCKTEA,
	CIPHER_NULLCIPHER,
	CIPHER_ILLEGAL_CIPHER,
	NUM_CIPHERS,
};

enum HashAlgorithmIdentifier : uchar
{
	HASH_ALGORITHM_UNKNOWN,
	HASH_ALGORITHM_TIGER,
	NUM_HASH_ALGORITHM,
};

class MMG_ICipher
{
public:
	CipherIdentifier m_Indentifier;

private:

public:
	MMG_ICipher();

	virtual void Encrypt(char *aData, sizeptr_t aDataLength) = 0;
	virtual void Decrypt(char *aData, sizeptr_t aDataLength) = 0;

	virtual CipherIdentifier GetType();

	static bool DecryptWith(CipherIdentifier aCipher, ulong *aCipherKeys, uint *aData, sizeptr_t aDataLength);
	static bool EncryptWith(CipherIdentifier aCipher, ulong *aCipherKeys, uint *aData, sizeptr_t aDataLength);
};