#pragma once

class MMG_CryptoHash : public MMG_IStreamable
{
public:
	ulong					m_Hash[16];
	ulong					m_HashLength;
	HashAlgorithmIdentifier	m_GeneratedFromHashAlgorithm;

private:

public:
	MMG_CryptoHash	();
	MMG_CryptoHash	(voidptr_t theHash, uint theByteLength, HashAlgorithmIdentifier theSourceAlgorithm);

	void SetHash	(voidptr_t theHash, uint theByteLength, HashAlgorithmIdentifier theSourceAlgorithm);

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};