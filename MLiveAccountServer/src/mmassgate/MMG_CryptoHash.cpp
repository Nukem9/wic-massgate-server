#include "../stdafx.h"

void MMG_CryptoHash::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUChar((uchar)this->m_HashLength);
	aMessage->WriteUChar(this->m_GeneratedFromHashAlgorithm);
	aMessage->WriteRawData(this->m_Hash, this->m_HashLength);
}

bool MMG_CryptoHash::FromStream(MN_ReadMessage *aMessage)
{
	if(!aMessage->ReadUChar((uchar &)this->m_HashLength) || !aMessage->ReadUChar((uchar &)this->m_GeneratedFromHashAlgorithm))
		return false;

	uint dataSize = (this->m_HashLength > sizeof(this->m_Hash)) ? sizeof(this->m_Hash) : this->m_HashLength;

	if(dataSize > 0 && !aMessage->ReadRawData(this->m_Hash, dataSize, NULL))
		return false;

	return true;
}