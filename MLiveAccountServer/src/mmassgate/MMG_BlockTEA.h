#pragma once

class MMG_BlockTEA : public MMG_ICipher
{
public:

//private:
	ulong m_Keys[4];

public:
	MMG_BlockTEA();
	MMG_BlockTEA(ulong *keys);
	MMG_BlockTEA(ulong key1, ulong key2, ulong key3, ulong key4);

	~MMG_BlockTEA();

	template <typename T>
	void Encrypter(T *aData, uint aDataLength);

	template<typename T>
	void Decrypter(T *aData, uint aDataLength);

	void Encrypt(char *aData, uint aDataLength);
	void Decrypt(char *aData, uint aDataLength);

private:
};