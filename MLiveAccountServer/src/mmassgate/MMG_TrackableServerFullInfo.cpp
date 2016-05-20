#include "../stdafx.h"

void MMG_TrackableServerFullInfo::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_GameVersion);
	aMessage->WriteUShort(this->m_ProtocolVersion);
	aMessage->WriteUInt64(this->m_CurrentMapHash);
	aMessage->WriteUInt64(this->m_CycleHash);
	aMessage->WriteString(this->m_ServerName);
	aMessage->WriteUShort(this->m_ServerReliablePort);
	aMessage->WriteUChar(this->bf_PlayerCount);
	aMessage->WriteUChar(this->bf_MaxPlayers);
	aMessage->WriteUChar(this->bf_SpectatorCount);
	aMessage->WriteUChar(this->bf_RankedFlag);
	aMessage->WriteUChar(this->bf_ServerType);
	aMessage->WriteUChar(this->bf_RankBalanceTeams);
	aMessage->WriteUChar(this->bf_HasDomMaps);
	aMessage->WriteUChar(this->bf_HasAssaultMaps);
	aMessage->WriteUChar(this->bf_HasTugOfWarMaps);
	aMessage->WriteUChar(this->m_ServerType);
	aMessage->WriteUInt(this->m_IP);
	aMessage->WriteUInt(this->m_ModId);
	aMessage->WriteUShort(this->m_MassgateCommPort);
	aMessage->WriteFloat(this->m_GameTime);
	aMessage->WriteUInt(this->m_ServerId);
	aMessage->WriteUInt(this->m_CurrentLeader);
	aMessage->WriteUInt(this->m_WinnerTeam);
	aMessage->WriteUInt(this->m_HostProfileId);
	
	//if (this->bf_PlayerCount > 0 && this->bf_PlayerCount < this->bf_MaxPlayers)
	//{
	//	for(int i = 0; i < this->bf_PlayerCount; i++) 
	//		this->m_Players[i].ToStream(aMessage);
	//}
}

bool MMG_TrackableServerFullInfo::FromStream(MN_ReadMessage *aMessage)
{
	//this is probably never used.

	if (!aMessage->ReadUInt(this->m_GameVersion)
		|| !aMessage->ReadUShort(this->m_ProtocolVersion)
		|| !aMessage->ReadUInt64(this->m_CurrentMapHash)
		|| !aMessage->ReadUInt64(this->m_CycleHash)
		|| !aMessage->ReadString(this->m_ServerName, ARRAYSIZE(this->m_ServerName))
		|| !aMessage->ReadUShort(this->m_ServerReliablePort))
		return false;

	// Read bitfields (2, 1, 3)?
	uchar temp[3];

	if (!aMessage->ReadUChar(temp[0])
		|| !aMessage->ReadUChar(temp[1])
		|| !aMessage->ReadUChar(temp[2]))
		return false;

	this->bf_MaxPlayers			= temp[1];
	this->bf_PlayerCount		= temp[0];
	this->bf_SpectatorCount		= temp[2];

	// Read bitfields (2, 1, 3, 4, 5, 6)?
	uchar temp2[6];

	if (!aMessage->ReadUChar(temp2[0])
		|| !aMessage->ReadUChar(temp2[1])
		|| !aMessage->ReadUChar(temp2[2])
		|| !aMessage->ReadUChar(temp2[3])
		|| !aMessage->ReadUChar(temp2[4])
		|| !aMessage->ReadUChar(temp2[5]))
		return false;

	this->bf_ServerType			= temp2[1];
	this->bf_RankedFlag			= temp2[0];
	this->bf_RankBalanceTeams	= temp2[2];
	this->bf_HasDomMaps			= temp2[3];
	this->bf_HasAssaultMaps		= temp2[4];
	this->bf_HasTugOfWarMaps	= temp2[5];

	//read rest of message
	if (!aMessage->ReadUChar(this->m_ServerType)
		|| !aMessage->ReadUInt(this->m_IP)
		|| !aMessage->ReadUInt(this->m_ModId)
		|| !aMessage->ReadUShort(this->m_MassgateCommPort)
		|| !aMessage->ReadFloat(this->m_GameTime)
		|| !aMessage->ReadUInt(this->m_ServerId)
		|| !aMessage->ReadUInt(this->m_CurrentLeader)
		|| !aMessage->ReadUInt(this->m_WinnerTeam)
		|| !aMessage->ReadUInt(this->m_HostProfileId))
		return false;

	// read players
	//if (this->bf_PlayerCount >= this->bf_MaxPlayers)
	//	return false;
	//
	//for (int i = 0; i < this->bf_PlayerCount; i++)
	//{
	//	if (!this->m_Players[i].FromStream(aMessage))
	//		return false;
	//}

	return true;
}