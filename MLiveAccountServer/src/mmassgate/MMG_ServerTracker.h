#pragma once

enum ServerListFilterFlags
{
	DEDICATED_FLAG		= 0x0001,	// dedicated flag
	RANKED_FLAG			= 0x0002,	// ranked flag
	RANKBALANCE_FLAG	= 0x0004,	// rankbalanced flag
	GAMEMODE_FLAG		= 0x0038,	// gamemode flags
	MAXPLAYERS_FLAG		= 0x0040,	// consider maximum players on server flag
	MINPLAYERS_FLAG		= 0x0080,	// consider minimum players on server flag
	POPULATION_FLAG		= 0x0100,	// population four-state flag
	PLAYNOW_FLAG		= 0x0200,	// play now button has been pressed
	MAPNAME_FLAG		= 0x0400,	// mapname flag
	SERVERNAME_FLAG		= 0x0800,	// servername flag
	PASSWORD_FLAG		= 0x1000,	// passworded flag
	MOD_FLAG			= 0x2000,	// mod flag
};

class ServerListFilters
{
public:
	//read search filters from client
	uchar dedicated, ranked, rankbalance;
	uchar gamemode[3], maxplayers, minplayers, population;
	wchar_t mapname[16], servername[16];
	uchar password, mod;
	uchar playnow;

private:
	ushort activefilters;

public:
	ServerListFilters(ushort activefilters)
	{
		memset(this->gamemode, 0, sizeof(this->gamemode));
		memset(this->mapname, 0, sizeof(this->mapname));
		memset(this->servername, 0, sizeof(this->servername));
		this->dedicated			= 0;
		this->ranked			= 0;
		this->rankbalance		= 0;
		this->minplayers		= 0;
		this->maxplayers		= 0;
		this->population		= 0;
		this->playnow		= 0;
		this->password			= 0;
		this->mod				= 0;

		this->activefilters = activefilters;
	}

	void PrintFilters()
	{
		if (this->HasFlag(DEDICATED_FLAG))
			DebugLog(L_INFO, "dedicated flag set\t\t: %s", this->dedicated == 1 ? "Dedicated" : "Not Dedicated");

		if (this->HasFlag(RANKED_FLAG))
			DebugLog(L_INFO, "ranked flag set\t\t: %s", this->ranked == 1 ? "Ranked" : "Not Ranked" );

		if (this->HasFlag(RANKBALANCE_FLAG))
			DebugLog(L_INFO, "rankbalance flag set\t: %s", this->rankbalance == 1 ? "Rank Balanced" : "Not Rank Balanced");

		if (this->HasFlag(GAMEMODE_FLAG) && this->gamemode[0])
			DebugLog(L_INFO, "gamemode flag set\t\t: Domination Only");

		if (this->HasFlag(GAMEMODE_FLAG) && this->gamemode[1])
			DebugLog(L_INFO, "gamemode flag set\t\t: Assault Only");

		if (this->HasFlag(GAMEMODE_FLAG) && this->gamemode[2])
			DebugLog(L_INFO, "gamemode flag set\t\t: Tug of War Only");

		if (this->HasFlag(MAXPLAYERS_FLAG))
			DebugLog(L_INFO, "maxplayers flag set\t\t: %i", this->maxplayers);

		if (this->HasFlag(MINPLAYERS_FLAG))
			DebugLog(L_INFO, "minplayers flag set\t\t: %i", this->minplayers);
		
		if (this->HasFlag(POPULATION_FLAG) && this->population == 0)
			DebugLog(L_INFO, "population flag set\t\t: No Filter");

		if (this->HasFlag(POPULATION_FLAG) && this->population == 1)
			DebugLog(L_INFO, "population flag set\t\t: Not Full");

		if (this->HasFlag(POPULATION_FLAG) && this->population == 2)
			DebugLog(L_INFO, "population flag set\t\t: Not Empty");

		if (this->HasFlag(POPULATION_FLAG) && this->population == 3)
			DebugLog(L_INFO, "population flag set\t\t: Not Empty or Full");

		if (this->HasFlag(PLAYNOW_FLAG))
			DebugLog(L_INFO, "unknown flag set\t\t: %i", this->playnow);

		if (this->HasFlag(MAPNAME_FLAG))
			DebugLog(L_INFO, "mapname flag set\t\t: %ws", this->mapname);

		if (this->HasFlag(SERVERNAME_FLAG))
			DebugLog(L_INFO, "servername flag set\t\t: %ws", this->servername);

		if (this->HasFlag(PASSWORD_FLAG))
			DebugLog(L_INFO, "password flag set\t\t: %s", this->password == 1 ? "Need Password" : "No Password");

		if (this->HasFlag(MOD_FLAG))
			DebugLog(L_INFO, "mod flag set\t\t: %s", this->mod == 1 ? "No mods" : "Only mods");
	}

	bool FromStream(MN_ReadMessage *aMessage)
	{
		if (this->HasFlag(DEDICATED_FLAG) && !aMessage->ReadUChar(this->dedicated))
			return false;

		if (this->HasFlag(RANKED_FLAG) && !aMessage->ReadUChar(this->ranked))
			return false;

		if (this->HasFlag(RANKBALANCE_FLAG) && !aMessage->ReadUChar(this->rankbalance))
			return false;

		if (this->HasFlag(GAMEMODE_FLAG) && !aMessage->ReadUChar(this->gamemode[0]))  // domination
			return false;

		if (this->HasFlag(GAMEMODE_FLAG) && !aMessage->ReadUChar(this->gamemode[1]))  // assault
			return false;

		if (this->HasFlag(GAMEMODE_FLAG) && !aMessage->ReadUChar(this->gamemode[2]))  // tugofwar
			return false;

		if (this->HasFlag(MAXPLAYERS_FLAG) && !aMessage->ReadUChar(this->maxplayers))
			return false;

		if (this->HasFlag(MINPLAYERS_FLAG) && !aMessage->ReadUChar(this->minplayers))
			return false;

		// always reads population, massgate crashes otherwise
		if (this->HasFlag(POPULATION_FLAG) && !aMessage->ReadUChar(this->population))
			return false;

		if (this->HasFlag(PLAYNOW_FLAG) && !aMessage->ReadUChar(this->playnow))
			return false;

		if (this->HasFlag(MAPNAME_FLAG) && !aMessage->ReadString(this->mapname, ARRAYSIZE(this->mapname)))
			return false;

		if (this->HasFlag(SERVERNAME_FLAG) && !aMessage->ReadString(this->servername, ARRAYSIZE(this->servername)))
			return false;

		if (this->HasFlag(PASSWORD_FLAG) && !aMessage->ReadUChar(this->password))
			return false;

		if (this->HasFlag(MOD_FLAG) && !aMessage->ReadUChar(this->mod))
			return false;

		return true;
	}

	bool HasFlag(ServerListFilterFlags flag)
	{
		return (this->activefilters & flag) != 0;
	}
};

CLASS_SINGLE(MMG_ServerTracker)
{
public:

private:

public:
	MMG_ServerTracker();

	bool HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
};