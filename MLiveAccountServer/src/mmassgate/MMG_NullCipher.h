#pragma once

class MMG_NullCipher : public MMG_ICipher
{
public:

private:

public:
	MMG_NullCipher();

	void Encrypt(char *aData, sizeptr_t aDataLength);
	void Decrypt(char *aData, sizeptr_t aDataLength);
};