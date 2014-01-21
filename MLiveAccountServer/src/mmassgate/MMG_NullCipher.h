#pragma once

class MMG_NullCipher : public MMG_ICipher
{
public:

private:

public:
	MMG_NullCipher();

	void Encrypt(uint *aData, uint aDataLength);
	void Decrypt(uint *aData, uint aDataLength);
};