#include "../stdafx.h"

void MMG_TrackableServerFullInfo::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_GameVersion);
	aMessage->WriteUShort(this->m_ProtocolVersion);
	aMessage->WriteUInt64(this->m_CurrentMapHash);
	aMessage->WriteUInt64(this->m_CycleHash);
	aMessage->WriteString(this->m_ServerName);
	aMessage->WriteUShort(this->m_ServerReliablePort);
	aMessage->WriteUChar(this->somebits.bitfield2);
	aMessage->WriteUChar(this->somebits.bitfield1);
	aMessage->WriteUChar(this->somebits.bitfield3);
	aMessage->WriteUChar(this->somebits2.bitfield2);
	aMessage->WriteUChar(this->somebits2.bitfield1);
	aMessage->WriteUChar(this->somebits2.RankBalanceTeams);
	aMessage->WriteUChar(this->somebits2.HasDomMaps);
	aMessage->WriteUChar(this->somebits2.HasAssaultMaps);
	aMessage->WriteUChar(this->somebits2.HasTugOfWarMaps);
	aMessage->WriteUChar(this->m_ServerType);
	aMessage->WriteUInt(this->m_IP);
	aMessage->WriteUInt(this->m_ModId);
	aMessage->WriteUShort(this->m_MassgateCommPort);
	aMessage->WriteFloat(this->m_GameTime);
	aMessage->WriteUInt(this->m_ServerId);
	aMessage->WriteUInt(this->m_CurrentLeader);
	aMessage->WriteUInt(this->m_WinnerTeam);
	aMessage->WriteUInt(this->m_HostProfileId);
	
	//if (this->somebits.bitfield2 > 0 && this->somebits.bitfield2 < this->somebits.bitfield1)
	//{
	//	for(int i = 0; i < this->somebits.bitfield2; i++) 
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

	this->somebits.bitfield2 = temp[0];
	this->somebits.bitfield1 = temp[1];
	this->somebits.bitfield3 = temp[2];

	// Read bitfields (2, 1, 3, 4, 5, 6)?
	uchar temp2[6];

	if (!aMessage->ReadUChar(temp2[0])
		|| !aMessage->ReadUChar(temp2[1])
		|| !aMessage->ReadUChar(temp2[2])
		|| !aMessage->ReadUChar(temp2[3])
		|| !aMessage->ReadUChar(temp2[4])
		|| !aMessage->ReadUChar(temp2[5]))
		return false;

	this->somebits2.bitfield2			= temp2[0];
	this->somebits2.bitfield1			= temp2[1];
	this->somebits2.RankBalanceTeams	= temp2[2];
	this->somebits2.HasDomMaps			= temp2[3];
	this->somebits2.HasAssaultMaps		= temp2[4];
	this->somebits2.HasTugOfWarMaps		= temp2[5];

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
	//if (this->somebits.bitfield2 >= this->somebits.bitfield1)
	//	return false;
	//
	//for (int i = 0; i < this->somebits.bitfield2; i++)
	//{
	//	if (!this->m_Players[i].FromStream(aMessage))
	//		return false;
	//}

	return true;
}