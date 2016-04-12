#include "../stdafx.h"

void MMG_TrackableServerFullInfo::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_GameVersion);
	aMessage->WriteUShort(this->m_ProtocolVersion);
	aMessage->WriteUInt64(this->m_CurrentMapHash);
	aMessage->WriteUInt64(this->m_CycleHash);
	aMessage->WriteString(this->m_ServerName);
	aMessage->WriteUShort(this->m_ServerReliablePort);
	
	//server still probably wont show in list until 
	//we figure out what the bitmask is used for
	aMessage->WriteUChar(this->gapc4);
	aMessage->WriteUChar(this->gapc4);
	aMessage->WriteUChar(this->gapc4);
	aMessage->WriteUChar(this->_bf198);
	aMessage->WriteUChar(this->_bf198);
	aMessage->WriteUChar(this->_bf198);
	aMessage->WriteUChar(this->_bf198);
	aMessage->WriteUChar(this->_bf198);
	aMessage->WriteUChar(this->_bf198);
	
	aMessage->WriteUChar(this->m_ServerType);
	aMessage->WriteUInt(this->m_IP);
	aMessage->WriteUInt(this->m_ModId);
	aMessage->WriteUShort(this->m_MassgateCommPort);
	aMessage->WriteFloat(this->m_GameTime);
	aMessage->WriteUInt(this->m_ServerId);
	aMessage->WriteUInt(this->m_CurrentLeader);
	aMessage->WriteUInt(this->m_WinnerTeam);
	aMessage->WriteUInt(this->m_HostProfileId);
	
	//gapc4 has something to do with how many players get written to the stream?
	/*
	for(int i=0;i<64;i++) 
		this->m_Players[i].ToStream(aMessage);
	*/
}

bool MMG_TrackableServerFullInfo::FromStream(MN_ReadMessage *aMessage)
{
	return true;
}