#include "../stdafx.h"

MMG_ServerFilter::MMG_ServerFilter()
{
	memset(this->partOfServerName, 0, sizeof(this->partOfServerName));
	memset(this->partOfMapName, 0, sizeof(this->partOfMapName));
	this->requiresPassword		= false;
	this->isDedicated			= false;
	this->isRanked				= false;
	this->isRankBalanced		= false;
	this->hasDominationMaps		= false;
	this->hasAssaultMaps		= false;
	this->hasTowMaps			= false;
	this->noMod					= false;
	this->totalSlots			= 0;
	this->emptySlots			= 0;
	this->notFullEmpty			= 0;
	this->fromPlayNow			= false;

	this->activefilters			= 0;
}

void MMG_ServerFilter::ToStream(MN_WriteMessage *aMessage)
{
}

bool MMG_ServerFilter::FromStream(MN_ReadMessage *aMessage)
{
	uint gameVersion;
	if (!aMessage->ReadUInt(gameVersion))
		return false;

	uint protocolVersion;
	if (!aMessage->ReadUInt(protocolVersion))
		return false;

	if (!aMessage->ReadUShort(this->activefilters))
		return false;

	if (this->HasFlag(DEDICATED_FLAG) && !aMessage->ReadUChar(this->isDedicated))
		return false;

	if (this->HasFlag(RANKED_FLAG) && !aMessage->ReadUChar(this->isRanked))
		return false;

	if (this->HasFlag(RANKBALANCE_FLAG) && !aMessage->ReadUChar(this->isRankBalanced))
		return false;

	if (this->HasFlag(GAMEMODE_DOM_FLAG) && !aMessage->ReadUChar(this->hasDominationMaps))
		return false;

	if (this->HasFlag(GAMEMODE_AST_FLAG) && !aMessage->ReadUChar(this->hasAssaultMaps))
		return false;

	if (this->HasFlag(GAMEMODE_TOW_FLAG) && !aMessage->ReadUChar(this->hasTowMaps))
		return false;

	if (this->HasFlag(TOTALSLOTS_FLAG) && !aMessage->ReadChar(this->totalSlots))
		return false;

	if (this->HasFlag(EMPTYSLOTS_FLAG) && !aMessage->ReadChar(this->emptySlots))
		return false;

	// always reads population, massgate crashes otherwise
	if (this->HasFlag(NOTFULLEMPTY_FLAG) && !aMessage->ReadChar(this->notFullEmpty))
		return false;

	if (this->HasFlag(PLAYNOW_FLAG) && !aMessage->ReadChar(this->fromPlayNow))
		return false;

	if (this->HasFlag(MAPNAME_FLAG) && !aMessage->ReadString(this->partOfMapName, ARRAYSIZE(this->partOfMapName)))
		return false;

	if (this->HasFlag(SERVERNAME_FLAG) && !aMessage->ReadString(this->partOfServerName, ARRAYSIZE(this->partOfServerName)))
		return false;

	if (this->HasFlag(PASSWORD_FLAG) && !aMessage->ReadUChar(this->requiresPassword))
		return false;

	if (this->HasFlag(MOD_FLAG) && !aMessage->ReadUChar(this->noMod))
		return false;

	return true;
}

bool MMG_ServerFilter::HasFlag(ServerListFilterFlags flag)
{
	return (this->activefilters & flag) != 0;
}

void MMG_ServerFilter::PrintFilters()
{
	if (this->HasFlag(DEDICATED_FLAG))
		DebugLog(L_INFO, "dedicated flag set\t\t: %s", this->isDedicated ? "Dedicated" : "Not Dedicated");

	if (this->HasFlag(RANKED_FLAG))
		DebugLog(L_INFO, "ranked flag set\t\t: %s", this->isRanked ? "Ranked" : "Not Ranked" );

	if (this->HasFlag(RANKBALANCE_FLAG))
		DebugLog(L_INFO, "rankbalance flag set\t: %s", this->isRankBalanced ? "Rank Balanced" : "Not Rank Balanced");

	if (this->HasFlag(GAMEMODE_DOM_FLAG) && this->hasDominationMaps)
		DebugLog(L_INFO, "gamemode flag set\t\t: Domination Only");

	if (this->HasFlag(GAMEMODE_AST_FLAG) && this->hasAssaultMaps)
		DebugLog(L_INFO, "gamemode flag set\t\t: Assault Only");

	if (this->HasFlag(GAMEMODE_TOW_FLAG) && this->hasTowMaps)
		DebugLog(L_INFO, "gamemode flag set\t\t: Tug of War Only");

	if (this->HasFlag(TOTALSLOTS_FLAG))
		DebugLog(L_INFO, "totalslots flag set\t\t: %i", this->totalSlots);

	if (this->HasFlag(EMPTYSLOTS_FLAG))
		DebugLog(L_INFO, "emptyslots flag set\t\t: %i", this->emptySlots);

	if (this->HasFlag(NOTFULLEMPTY_FLAG) && this->notFullEmpty == 0)
		DebugLog(L_INFO, "population flag set\t\t: No Filter");

	if (this->HasFlag(NOTFULLEMPTY_FLAG) && this->notFullEmpty == 1)
		DebugLog(L_INFO, "population flag set\t\t: Not Full");

	if (this->HasFlag(NOTFULLEMPTY_FLAG) && this->notFullEmpty == 2)
		DebugLog(L_INFO, "population flag set\t\t: Not Empty");

	if (this->HasFlag(NOTFULLEMPTY_FLAG) && this->notFullEmpty == 3)
		DebugLog(L_INFO, "population flag set\t\t: Not Empty or Full");

	if (this->HasFlag(PLAYNOW_FLAG))
		DebugLog(L_INFO, "playnow flag set\t\t: %i", this->fromPlayNow);

	if (this->HasFlag(MAPNAME_FLAG))
		DebugLog(L_INFO, "mapname flag set\t\t: %ws", this->partOfMapName);

	if (this->HasFlag(SERVERNAME_FLAG))
		DebugLog(L_INFO, "servername flag set\t\t: %ws", this->partOfServerName);

	if (this->HasFlag(PASSWORD_FLAG))
		DebugLog(L_INFO, "password flag set\t\t: %s", this->requiresPassword ? "Need Password" : "No Password");

	if (this->HasFlag(MOD_FLAG))
		DebugLog(L_INFO, "mod flag set\t\t: %s", this->noMod ? "No mods" : "Only mods");
}