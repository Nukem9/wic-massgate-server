#include "../stdafx.h"

MMG_NullCipher::MMG_NullCipher()
{
	this->m_Indentifier = CIPHER_NULLCIPHER;
}

void MMG_NullCipher::Encrypt(char *aData, sizeptr_t aDataLength)
{
}

void MMG_NullCipher::Decrypt(char *aData, sizeptr_t aDataLength)
{
}