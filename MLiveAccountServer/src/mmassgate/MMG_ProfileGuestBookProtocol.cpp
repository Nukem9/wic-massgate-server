#include "../stdafx.h"

MMG_ProfileGuestBookProtocol::GetRsp::GuestbookEntry::GuestbookEntry()
{
	memset(this->m_Message, 0, sizeof(this->m_Message));
	this->m_Timestamp = 0;
	this->m_ProfileId = 0;
	this->m_MessageId = 0;
}

MMG_ProfileGuestBookProtocol::GetRsp::GetRsp() : m_Entries()
{
	this->m_ProfileId = 0;
	this->m_RequestId = 0;
}

void MMG_ProfileGuestBookProtocol::GetRsp::ToStream(MN_WriteMessage *aMessage)
{
}

MMG_ProfileGuestBookProtocol::MMG_ProfileGuestBookProtocol()
{
}