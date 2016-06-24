#pragma once

enum ServerListFilterFlags
{
	DEDICATED_FLAG		= 0x0001,	// dedicated flag
	RANKED_FLAG			= 0x0002,	// ranked flag
	RANKBALANCE_FLAG	= 0x0004,	// rankbalanced flag
	GAMEMODE_DOM_FLAG	= 0x0008,	// gamemode domination flag
	GAMEMODE_AST_FLAG	= 0x0010,	// gamemode assault flag
	GAMEMODE_TOW_FLAG	= 0x0020,	// gamemode tug of war flag
	TOTALSLOTS_FLAG		= 0x0040,	// total slots on server
	EMPTYSLOTS_FLAG		= 0x0080,	// empty slots available
	NOTFULLEMPTY_FLAG	= 0x0100,	// population four-state flag
	PLAYNOW_FLAG		= 0x0200,	// play now button has been pressed
	MAPNAME_FLAG		= 0x0400,	// partOfMapName flag
	SERVERNAME_FLAG		= 0x0800,	// partOfServerName flag
	PASSWORD_FLAG		= 0x1000,	// passworded flag
	MOD_FLAG			= 0x2000,	// no mod flag
};

class MMG_ServerFilter : public MMG_IStreamable
{
public:
	wchar_t partOfServerName[16];
	wchar_t partOfMapName[16];
	uchar requiresPassword;
	uchar isDedicated;
	uchar isRanked;
	uchar isRankBalanced;
	uchar hasDominationMaps;
	uchar hasAssaultMaps;
	uchar hasTowMaps;
	uchar noMod;
	char totalSlots;
	char emptySlots;
	char notFullEmpty;
	char fromPlayNow;

private:
	ushort activefilters;

public:
	MMG_ServerFilter();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);

	bool HasFlag(ServerListFilterFlags flag);
	void PrintFilters();
};