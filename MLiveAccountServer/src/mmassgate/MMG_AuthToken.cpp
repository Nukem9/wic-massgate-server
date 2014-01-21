#include "../stdafx.h"

void MMG_AuthToken::ToStream(MN_WriteMessage *aMessage)
{
	this->m_Hash.ToStream(aMessage);
	aMessage->WriteUInt(this->m_AccountId);
	aMessage->WriteUInt(this->m_ProfileId);
	aMessage->WriteUInt(this->m_TokenId);
	aMessage->WriteUInt(this->m_CDkeyId);
	aMessage->WriteUInt(this->m_GroupMemberships.m_Code);
}

bool MMG_AuthToken::FromStream(MN_ReadMessage *aMessage)
{
	if(!this->m_Hash.FromStream(aMessage))
		return false;

	if(!aMessage->ReadUInt(this->m_AccountId) || !aMessage->ReadUInt(this->m_ProfileId))
		return false;

	if(!aMessage->ReadUInt(this->m_TokenId) || !aMessage->ReadUInt(this->m_CDkeyId))
		return false;

	if(!aMessage->ReadUInt(this->m_GroupMemberships.m_Code))
		return false;

	return true;
}