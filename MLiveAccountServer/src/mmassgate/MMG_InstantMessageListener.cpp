#include "../stdafx.h"

MMG_InstantMessageListener::InstantMessage::InstantMessage() : m_SenderProfile()
{
	this->m_MessageId			= 0;
	this->m_RecipientProfile	= 0;
	memset(this->m_Message, 0, sizeof(this->m_Message));
	this->m_WrittenAt			= 0;
}

void MMG_InstantMessageListener::InstantMessage::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_MessageId);
	this->m_SenderProfile.ToStream(aMessage);
	aMessage->WriteUInt(this->m_RecipientProfile);
	aMessage->WriteString(this->m_Message);
	aMessage->WriteUInt(this->m_WrittenAt);
}

bool MMG_InstantMessageListener::InstantMessage::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUInt(this->m_MessageId))
		return false;

	if (!this->m_SenderProfile.FromStream(aMessage))
		return false;

	if (!aMessage->ReadUInt(this->m_RecipientProfile))
		return false;

	if (!aMessage->ReadString(this->m_Message, ARRAYSIZE(this->m_Message)))
		return false;

	if (!aMessage->ReadUInt(this->m_WrittenAt))
		return false;
	
	return true;
}
	
MMG_InstantMessageListener::MMG_InstantMessageListener()
{
}