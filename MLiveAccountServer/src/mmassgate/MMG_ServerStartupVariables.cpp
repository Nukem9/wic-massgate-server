#include "../stdafx.h"

MMG_ServerStartupVariables::MMG_ServerStartupVariables()
{
	memset(m_ServerName, 0, sizeof(m_ServerName));
	m_CurrentMapHash		= 0;
	memset(m_PublicIp, 0, sizeof(m_PublicIp));
	m_Ip					= 0;
	m_ModId					= 0;
	m_ServerReliablePort	= 0;
	m_MassgateCommPort		= 0;
	m_GameVersion			= 0;
	m_ProtocolVersion		= 0;
	_bf166					= 0;
	m_ServerType			= NORMAL_SERVER;
	m_HostProfileId			= 0;
	m_Fingerprint			= 0;
	m_ContainsPreorderMap	= true;
	m_IsRankBalanced		= false;
	m_HasDominationMaps		= false;
	m_HasAssaultMaps		= false;
	m_HasTowMaps			= true;
}

void MMG_ServerStartupVariables::ToStream(MN_WriteMessage *aMessage)
{
	uint randXorSeed = rand();

	aMessage->WriteUInt(MassgateProtocolVersion);
	aMessage->WriteUInt(this->m_GameVersion);
	aMessage->WriteUShort(this->m_ProtocolVersion);
	aMessage->WriteUInt(randXorSeed);
	aMessage->WriteUInt(randXorSeed ^ this->m_Fingerprint);
	aMessage->WriteUInt(this->m_Ip);
	aMessage->WriteUInt(this->m_ModId);
	aMessage->WriteUShort(this->m_ServerReliablePort);
	aMessage->WriteUShort(this->m_MassgateCommPort);
	aMessage->WriteString(this->m_ServerName);
	aMessage->WriteUChar(this->somebits.MaxPlayers);
	aMessage->WriteUChar(this->somebits.Passworded);
	aMessage->WriteUChar(this->somebits.Dedicated);
	aMessage->WriteUChar(this->somebits.bitfield3);
	aMessage->WriteUChar(this->somebits.Ranked);
	aMessage->WriteUChar(this->m_ServerType);
	aMessage->WriteUInt64(this->m_CurrentMapHash);
	aMessage->WriteString(this->m_PublicIp);
	aMessage->WriteUInt(this->m_HostProfileId);
	aMessage->WriteUChar(this->m_ContainsPreorderMap);
	aMessage->WriteBool(this->m_IsRankBalanced);
	aMessage->WriteBool(this->m_HasDominationMaps);
	aMessage->WriteBool(this->m_HasAssaultMaps);
	aMessage->WriteBool(this->m_HasTowMaps);
}

bool MMG_ServerStartupVariables::FromStream(MN_ReadMessage *aMessage)
{
	// Protocol is checked twice
	uint protocolCheck;

	if (!aMessage->ReadUInt(protocolCheck))
		return false;

	// TODO: Change server protocol
	//if (protocolCheck != MassgateProtocolVersion)
	//	return false;

	// Fingerprint XOR variables
	uint fingerprintRand = 0;
	uint fingerprintXor  = 0;

	if (!aMessage->ReadUInt(this->m_GameVersion)
		|| !aMessage->ReadUShort(this->m_ProtocolVersion)
		|| !aMessage->ReadUInt(fingerprintRand)
		|| !aMessage->ReadUInt(fingerprintXor)
		|| !aMessage->ReadUInt(this->m_Ip)
		|| !aMessage->ReadUInt(this->m_ModId)
		|| !aMessage->ReadUShort(this->m_ServerReliablePort)
		|| !aMessage->ReadUShort(this->m_MassgateCommPort)
		|| !aMessage->ReadString(this->m_ServerName, ARRAYSIZE(this->m_ServerName)))
		return false;

	// Read bitfields (1, 2, 5, 3, 4)
	uchar temp[5];

	if (!aMessage->ReadUChar(temp[0])
		|| !aMessage->ReadUChar(temp[1])
		|| !aMessage->ReadUChar(temp[2])
		|| !aMessage->ReadUChar(temp[3])
		|| !aMessage->ReadUChar(temp[4]))
		return false;

	this->somebits.MaxPlayers = temp[0];
	this->somebits.Passworded = temp[1];
	this->somebits.Dedicated = temp[2];
	this->somebits.bitfield3 = temp[3];
	this->somebits.Ranked = temp[4];

	// Read the final parts of message
	if (!aMessage->ReadUChar(this->m_ServerType)
		|| !aMessage->ReadUInt64(this->m_CurrentMapHash)
		|| !aMessage->ReadString(this->m_PublicIp, ARRAYSIZE(this->m_PublicIp))
		|| !aMessage->ReadUInt(this->m_HostProfileId)
		|| !aMessage->ReadUChar(this->m_ContainsPreorderMap)
		|| !aMessage->ReadBool(this->m_IsRankBalanced)
		|| !aMessage->ReadBool(this->m_HasDominationMaps)
		|| !aMessage->ReadBool(this->m_HasAssaultMaps)
		|| !aMessage->ReadBool(this->m_HasTowMaps))
		return false;

	this->m_Fingerprint = fingerprintRand ^ fingerprintXor;
	return true;
}