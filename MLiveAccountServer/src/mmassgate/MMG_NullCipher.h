#pragma once

class MMG_NullCipher : public MMG_ICipher
{
public:

private:

public:
	MMG_NullCipher();

	void Encrypt(char *aData, uint aDataLength);
	void Decrypt(char *aData, uint aDataLength);
};