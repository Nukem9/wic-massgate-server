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

	void Encrypt(uint *aData, uint aDataLength);
	void Decrypt(uint *aData, uint aDataLength);

private:
	uint GetDelta();
};