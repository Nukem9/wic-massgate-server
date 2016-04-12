#pragma once

class MMG_TrackableServerFullInfo : public MMG_IStreamable
{
public:
	uint m_GameVersion;
	ushort m_ProtocolVersion;
	uint64 m_CurrentMapHash;
	uint64 m_CycleHash;
	wchar_t m_ServerName[64];
	ushort m_ServerReliablePort;
	uchar gapc4; //bitmask?
	uchar _bf198; //bitmask?
	uchar m_ServerType;	//uint
	uint m_IP;
	uint m_ModId;
	ushort m_MassgateCommPort;
	float m_GameTime;
	uint m_ServerId;
	uint m_CurrentLeader;
	uint m_HostProfileId;
	uint m_WinnerTeam;
	MMG_TrackablePlayerVariables m_Players[64];

	uint m_Ping;

private:

public:
	MMG_TrackableServerFullInfo() : m_Players()
	{
		this->m_GameVersion			= 0;
		this->m_ProtocolVersion		= 0;
		this->m_CurrentMapHash		= 0;
		this->m_CycleHash			= 0;
		memset(m_ServerName, 0, sizeof(m_ServerName));
		this->m_ServerReliablePort	= 0;
		
		this->gapc4					= 0;
		this->_bf198				= 0;
		
		this->m_ServerType			= 0;
		this->m_IP					= 0;
		this->m_ModId				= 0;
		this->m_MassgateCommPort	= 0;
		this->m_GameTime			= 0;
		this->m_ServerId			= 0;
		this->m_CurrentLeader		= 0;
		this->m_HostProfileId		= 0;
		this->m_WinnerTeam			= 0;
		
		this->m_Ping				= 0;
	}

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};