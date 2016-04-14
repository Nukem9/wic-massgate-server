#include "../stdafx.h"

MMG_ProfileGuestBookEntry::MMG_ProfileGuestBookEntry()
{
	memset(this->m_GuestBookMessage, 0, sizeof(this->m_GuestBookMessage));
	this->m_TimeStamp = 0;
	this->m_ProfileId = 0;
	this->m_MessageId = 0;
}

void MMG_ProfileGuestBookEntry::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteString(this->m_GuestBookMessage);
	aMessage->WriteUInt(this->m_TimeStamp);
	aMessage->WriteUInt(this->m_ProfileId);
	aMessage->WriteUInt(this->m_MessageId);
}

bool MMG_ProfileGuestBookEntry::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadString(this->m_GuestBookMessage, ARRAYSIZE(this->m_GuestBookMessage)))
		return false;

	if (!aMessage->ReadUInt(this->m_TimeStamp) || !aMessage->ReadUInt(this->m_ProfileId) || !aMessage->ReadUInt(this->m_MessageId))
		return false;

	return true;
}