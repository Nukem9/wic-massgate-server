#pragma once

class MMG_CryptoHash : public MMG_IStreamable
{
public:
	ulong					m_Hash[16];
	ulong					m_HashLength;
	HashAlgorithmIdentifier	m_GeneratedFromHashAlgorithm;

private:

public:
	MMG_CryptoHash()
	{
		memset(this->m_Hash, 0, sizeof(this->m_Hash));
		this->m_HashLength					= 0;
		this->m_GeneratedFromHashAlgorithm	= HASH_ALGORITHM_UNKNOWN;
	}

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};