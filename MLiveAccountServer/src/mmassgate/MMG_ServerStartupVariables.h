#pragma once

enum ServerType : uchar
{
	NORMAL_SERVER		= 0x0,
	MATCH_SERVER		= 0x1,
	FPM_SERVER			= 0x2,
	TOURNAMENT_SERVER	= 0x3,
	CLANMATCH_SERVER	= 0x4,
};

class MMG_ServerStartupVariables : public MMG_IStreamable
{
public:
	wchar_t		m_ServerName[64];
	uint64		m_CurrentMapHash;
	char		m_PublicIp[256];
	uint		m_Ip;
	uint		m_ModId;
	ushort		m_ServerReliablePort;
	ushort		m_MassgateCommPort;
	uint		m_GameVersion;
	ushort		m_ProtocolVersion;
	union
	{
		__int16	_bf166;
		struct
		{
			uchar bitfield1 : 5; // Maximum number of players
			uchar bitfield2 : 1; // ?
			uchar bitfield3 : 1; // ?
			uchar bitfield4 : 1; // ?
			uchar bitfield5 : 1; // Probably incorrect
		} somebits;
	};
	uchar		m_ServerType;
	uint		m_HostProfileId;
	uint		m_Fingerprint;
	uint64		m_ServerId;
	uchar		m_ContainsPreorderMap;
	bool		m_IsRankBalanced;
	bool		m_HasDominationMaps;
	bool		m_HasAssaultMaps;
	bool		m_HasTowMaps;

private:

public:
	MMG_ServerStartupVariables();

	void ToStream(MN_WriteMessage *aMessage);
	bool FromStream(MN_ReadMessage *aMessage);
};