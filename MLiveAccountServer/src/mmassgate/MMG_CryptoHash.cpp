#include "../stdafx.h"

MMG_CryptoHash::MMG_CryptoHash()
{
	memset(this->m_Hash, 0, sizeof(this->m_Hash));
	this->m_HashLength = 0;
	this->m_GeneratedFromHashAlgorithm = HASH_ALGORITHM_UNKNOWN;
}

MMG_CryptoHash::MMG_CryptoHash(voidptr_t theHash, uint theByteLength, HashAlgorithmIdentifier theSourceAlgorithm)
{
	this->SetHash(theHash, theByteLength, theSourceAlgorithm);
}

void MMG_CryptoHash::SetHash(voidptr_t theHash, uint theByteLength, HashAlgorithmIdentifier theSourceAlgorithm)
{
	assert(theByteLength < sizeof(m_Hash));

	this->m_HashLength					= theByteLength;
	this->m_GeneratedFromHashAlgorithm	= theSourceAlgorithm;

	// Zero all bits and then copy over the incoming data
	memset(this->m_Hash, 0, sizeof(this->m_Hash));
	memcpy(this->m_Hash, theHash, theByteLength);
}

void MMG_CryptoHash::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUChar((uchar)this->m_HashLength);
	aMessage->WriteUChar(this->m_GeneratedFromHashAlgorithm);
	aMessage->WriteRawData(this->m_Hash, this->m_HashLength);
}

bool MMG_CryptoHash::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUChar((uchar &)this->m_HashLength) || !aMessage->ReadUChar((uchar &)this->m_GeneratedFromHashAlgorithm))
		return false;

	if (!aMessage->ReadRawData(this->m_Hash, this->m_HashLength, nullptr))
		return false;

	return true;
}