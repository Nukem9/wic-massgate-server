#pragma once

class MMG_BlockTEA : public MMG_ICipher
{
public:

private:
	ulong m_Keys[4];

public:
	MMG_BlockTEA();
	MMG_BlockTEA(ulong *keys);
	MMG_BlockTEA(ulong key1, ulong key2, ulong key3, ulong key4);

	~MMG_BlockTEA();

	template <typename T>
	void Encrypter(T *aData, sizeptr_t aDataLength);

	template<typename T>
	void Decrypter(T *aData, sizeptr_t aDataLength);

	void Encrypt(char *aData, sizeptr_t aDataLength);
	void Decrypt(char *aData, sizeptr_t aDataLength);

private:
};