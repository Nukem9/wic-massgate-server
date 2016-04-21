#include "../stdafx.h"

MMG_TrackableServerBriefInfo::MMG_TrackableServerBriefInfo()
{
	memset(this->m_GameName, 0, sizeof(this->m_GameName));
	this->m_IP					= 0;
	this->m_ModId				= 0;
	this->m_ServerId			= 0;
	this->m_MassgateCommPort	= 0;
	this->m_CycleHash			= 0;
	this->m_ServerType			= 0;
	this->m_IsRankBalanced		= 0;
}

void MMG_TrackableServerBriefInfo::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteString(this->m_GameName);
	aMessage->WriteUInt(this->m_IP);
	aMessage->WriteUInt(this->m_ModId);
	aMessage->WriteUInt(this->m_ServerId);
	aMessage->WriteUShort(this->m_MassgateCommPort);
	aMessage->WriteUInt64(this->m_CycleHash);
	aMessage->WriteUChar(this->m_ServerType);
	aMessage->WriteBool(this->m_IsRankBalanced);
}

bool MMG_TrackableServerBriefInfo::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadString(this->m_GameName, ARRAYSIZE(this->m_GameName)))
		return false;

	if (!aMessage->ReadUInt(this->m_IP) || !aMessage->ReadUInt(this->m_ModId) || !aMessage->ReadUInt(this->m_ServerId))
		return false;

	if (!aMessage->ReadUShort(this->m_MassgateCommPort))
		return false;

	if(!aMessage->ReadUInt64(this->m_CycleHash))
		return false;

	if(!aMessage->ReadUChar(this->m_ServerType))
		return false;

	if(!aMessage->ReadBool(this->m_IsRankBalanced))
		return false;

	return true;
}