#pragma once

class MMG_TrackablePlayerVariables : public MMG_IStreamable
{
public:
	uint m_ProfileId;
	uint m_Score;

private:

public:
	MMG_TrackablePlayerVariables()
	{
		this->m_ProfileId	= 0;
		this->m_Score		= 0;
	}

	void ToStream	(MN_WriteMessage *aMessage)
	{
		aMessage->WriteUInt(this->m_ProfileId);
		aMessage->WriteUInt(this->m_Score);
	}

	bool FromStream	(MN_ReadMessage *aMessage)
	{
		if (!aMessage->ReadUInt(this->m_ProfileId) || !aMessage->ReadUInt(this->m_Score))
			return false;

		return true;
	}
};

class MMG_TrackableServerFullInfo : public MMG_IStreamable
{
public:
	uint m_GameVersion;
	ushort m_ProtocolVersion;
	uint64 m_CurrentMapHash;
	uint64 m_CycleHash;
	wchar_t m_ServerName[64];
	ushort m_ServerReliablePort;

	union
	{
		uchar gapc4[2];
		struct
		{
			uchar bitfield1 : 5;	//maxPlayers
			uchar bitfield2 : 5;	//playerCount
			uchar bitfield3 : 5;	//spectatorCount
		} somebits;
	};

	union
	{
		__int16	_bf198;
		struct
		{
			uchar bitfield1			: 1;	//serverType
			uchar bitfield2			: 1;	//rankedFlag
			uchar RankBalanceTeams	: 1;
			uchar HasDomMaps		: 1;
			uchar HasAssaultMaps	: 1;
			uchar HasTugOfWarMaps	: 1;
		} somebits2;
	};

	uchar m_ServerType;
	uint m_IP;
	uint m_ModId;
	ushort m_MassgateCommPort;
	float m_GameTime;
	uint m_ServerId;
	uint m_CurrentLeader;
	uint m_HostProfileId;
	uint m_WinnerTeam;
	MMG_TrackablePlayerVariables m_Players[64];

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
		
		memset(this->gapc4, 0, sizeof(this->gapc4));
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
	}

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};