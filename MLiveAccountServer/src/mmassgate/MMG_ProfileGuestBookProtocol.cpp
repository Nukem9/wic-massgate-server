#include "../stdafx.h"

void MMG_ProfileGuestBookProtocol::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_RequestId);
	aMessage->WriteUChar(this->m_IgnoresGettingProfile);
	aMessage->WriteUInt(this->m_Count);
	for (int i = 0; i < 1; i++)
	{
		this->m_GuestBookEntry[i].ToStream(aMessage);
	}

}

bool MMG_ProfileGuestBookProtocol::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUInt(this->m_RequestId))
		return false;
	
	if (!aMessage->ReadUChar(this->m_IgnoresGettingProfile))
		return false;

	if (!aMessage->ReadUInt(this->m_Count))
		return false;

	for (int i = 0; i < 1; i++)
	{
		this->m_GuestBookEntry[i].FromStream(aMessage);
	}

	return true;
}