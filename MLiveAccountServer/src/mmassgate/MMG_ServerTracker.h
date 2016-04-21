#pragma once

class ServerListFilters
{
public:
	enum
	{
		HAS_DEDICATED_FLAG		= 0x0001,	// dedicated flag
		HAS_RANKED_FLAG			= 0x0002,	// ranked flag
		HAS_RANKBALANCE_FLAG	= 0x0004,	// rankbalanced flag
		HAS_GAMEMODE_FLAG		= 0x0038,	// gamemode flags
		HAS_POPULATION_FLAG		= 0x0100,	// population four-state flag
		HAS_MAPNAME_FLAG		= 0x0400,	// mapname flag
		HAS_SERVERNAME_FLAG		= 0x0800,	// servername flag
		HAS_PASSWORD_FLAG		= 0x1000,	// passworded flag
		HAS_MOD_FLAG			= 0x2000,	// mod flag
	};

	//read search filters from client
	uchar dedicated, ranked, rankbalance;
	uchar gamemode[3], population;
	wchar_t mapname[16], servername[16];
	uchar password, mod;

	ServerListFilters()
	{
		memset(this->gamemode, 0, sizeof(this->gamemode));
		memset(this->mapname, 0, sizeof(this->mapname));
		memset(this->servername, 0, sizeof(this->servername));
		this->dedicated			= 0;
		this->ranked			= 0;
		this->rankbalance		= 0;
		this->population		= 0;
		this->password			= 0;
		this->mod				= 0;
	}

	void PrintFilters(ushort activefilters)
	{
		if (activefilters & HAS_DEDICATED_FLAG) 
		DebugLog(L_INFO, "dedicated flag set\t\t: %s", this->dedicated == 1 ? "Dedicated" : "Not Dedicated");

		if (activefilters & HAS_RANKED_FLAG)
			DebugLog(L_INFO, "ranked flag set\t\t: %s", this->ranked == 1 ? "Ranked" : "Not Ranked" );

		if (activefilters & HAS_RANKBALANCE_FLAG)
			DebugLog(L_INFO, "rankbalance flag set\t: %s", this->rankbalance == 1 ? "Rank Balanced" : "Not Rank Balanced");

		if (activefilters & HAS_GAMEMODE_FLAG)
		{
			if (this->gamemode[0])
				DebugLog(L_INFO, "gamemode flag set\t\t: Domination Only");

			if (this->gamemode[1])
				DebugLog(L_INFO, "gamemode flag set\t\t: Assault Only");

			if (this->gamemode[2])
				DebugLog(L_INFO, "gamemode flag set\t\t: Tug of War Only");
		}

		if (activefilters & HAS_POPULATION_FLAG)
		{
			if(this->population == 0)
				DebugLog(L_INFO, "population flag set\t\t: No Filter");

			if(this->population == 1)
				DebugLog(L_INFO, "population flag set\t\t: Not Full");

			if(this->population == 2)
				DebugLog(L_INFO, "population flag set\t\t: Not Empty");

			if(this->population == 3)
				DebugLog(L_INFO, "population flag set\t\t: Not Empty or Full");
		}

		if (activefilters & HAS_MAPNAME_FLAG)
			DebugLog(L_INFO, "mapname flag set\t\t: %ws", this->mapname);

		if (activefilters & HAS_SERVERNAME_FLAG)
			DebugLog(L_INFO, "servername flag set\t\t: %ws", this->servername);

		if (activefilters & HAS_PASSWORD_FLAG)
			DebugLog(L_INFO, "password flag set\t\t: %s", this->password == 1 ? "Need Password" : "No Password");

		if (activefilters & HAS_MOD_FLAG)
			DebugLog(L_INFO, "mod flag set\t\t: %s", this->mod == 1 ? "No mods" : "Only mods");
	}

	bool ReadFilters(ushort activefilters, MN_ReadMessage *aMessage)
	{
		if (activefilters & HAS_DEDICATED_FLAG) 
		{
			if (!aMessage->ReadUChar(this->dedicated))
				return false;
		}

		if (activefilters & HAS_RANKED_FLAG)
		{
			if (!aMessage->ReadUChar(this->ranked))
				return false;
		}

		if (activefilters & HAS_RANKBALANCE_FLAG)
		{
			if (!aMessage->ReadUChar(this->rankbalance))
				return false;
		}

		if (activefilters & HAS_GAMEMODE_FLAG)
		{
			if (!aMessage->ReadUChar(this->gamemode[0]))  // domination
				return false;

			if (!aMessage->ReadUChar(this->gamemode[1]))  // assault
				return false;

			if (!aMessage->ReadUChar(this->gamemode[2]))  // tugofwar
				return false;
		}

		//always reads population, massgate crashes otherwise
		if (activefilters & HAS_POPULATION_FLAG)
		{
			if (!aMessage->ReadUChar(this->population))
				return false;
		}

		if (activefilters & HAS_MAPNAME_FLAG)
		{
			if (!aMessage->ReadString(this->mapname, ARRAYSIZE(this->mapname)))
				return false;
		}

		if (activefilters & HAS_SERVERNAME_FLAG)
		{
			if (!aMessage->ReadString(this->servername, ARRAYSIZE(this->servername)))
				return false;
		}

		if (activefilters & HAS_PASSWORD_FLAG)
		{
			if (!aMessage->ReadUChar(this->password))
				return false;
		}

		if (activefilters & HAS_MOD_FLAG)
		{
			if (!aMessage->ReadUChar(this->mod))
				return false;
		}

		return true;
	}
};

CLASS_SINGLE(MMG_ServerTracker)
{
public:
	/*enum ServerType : int
	{
		NORMAL_SERVER,
		MATCH_SERVER,
		FPM_SERVER,
		TOURNAMENT_SERVER,
		CLANMATCH_SERVER
	};
	*/

	class Pinger
	{
	public:
		uint64 m_StartTimeStamp;
		uchar m_SequenceNumber;
		uchar m_GetExtendedInfoFlag;
		sockaddr_in m_Target;
	private:
	public:
		Pinger();
	};

private:

public:
	MMG_ServerTracker();

	bool PrivHandlePingers();
	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};