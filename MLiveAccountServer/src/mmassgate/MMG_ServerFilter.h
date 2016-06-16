#pragma once

enum ServerListFilterFlags
{
	DEDICATED_FLAG		= 0x0001,	// dedicated flag
	RANKED_FLAG			= 0x0002,	// ranked flag
	RANKBALANCE_FLAG	= 0x0004,	// rankbalanced flag
	GAMEMODE_FLAG		= 0x0038,	// gamemode flags
	TOTALSLOTS_FLAG		= 0x0040,	// total slots on server
	EMPTYSLOTS_FLAG		= 0x0080,	// empty slots available
	NOTFULLEMPTY_FLAG	= 0x0100,	// population four-state flag
	PLAYNOW_FLAG		= 0x0200,	// play now button has been pressed
	MAPNAME_FLAG		= 0x0400,	// mapname flag
	SERVERNAME_FLAG		= 0x0800,	// servername flag
	PASSWORD_FLAG		= 0x1000,	// passworded flag
	MOD_FLAG			= 0x2000,	// mod flag
};

class MMG_ServerFilter : public MMG_IStreamable
{
public:
	wchar_t partOfServerName[16];
	wchar_t partOfMapName[16];
	bool requiresPassword;
	bool isDedicated;
	bool isRanked;
	bool isRankBalanced;
	bool hasDominationMaps;
	bool hasAssaultMaps;
	bool hasTowMaps;
	bool noMod;
	uchar totalSlots;
	uchar emptySlots;
	uchar notFullEmpty;
	bool fromPlayNow;

private:
	ushort activefilters;

public:
	// TODO MMG_BitReader
	// this class is a bit wrong but oh well

	MMG_ServerFilter();
	MMG_ServerFilter(ushort activefilters);

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);

	bool HasFlag(ServerListFilterFlags flag);
	void PrintFilters();
};