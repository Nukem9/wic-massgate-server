#pragma once

class MMG_AuthToken : public MMG_IStreamable
{
public:
	uint					m_AccountId;
	uint					m_ProfileId;
	uint					m_TokenId;
	uint					m_CDkeyId;
	MMG_GroupMemberships	m_GroupMemberships;
	MMG_CryptoHash			m_Hash;
	//MMG_Tiger				myHasher;

private:

public:
	MMG_AuthToken() : m_Hash()
	{
		this->m_AccountId				= 0;
		this->m_ProfileId				= 0;
		this->m_TokenId					= 0;
		this->m_CDkeyId					= 0;
		this->m_GroupMemberships.m_Code = 0;
	}

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};