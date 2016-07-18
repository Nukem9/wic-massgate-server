#include "../stdafx.h"

MMG_Clan::MMG_Clan()
{
	memset(this->m_ClanName, 0, sizeof(this->m_ClanName));
	memset(this->m_FullClanTag, 0, sizeof(this->m_FullClanTag));
	this->m_ClanId				= 0;
	this->m_MemberCount			= 0;
	this->m_PlayerOfTheWeekId	= 0;
	memset(this->m_MemberIds, 0, sizeof(this->m_MemberIds));

	memset(this->m_ClanMotto, 0, sizeof(this->m_ClanMotto));
	memset(this->m_ClanMessageoftheday, 0, sizeof(this->m_ClanMessageoftheday));
	memset(this->m_ClanHomepage, 0, sizeof(this->m_ClanHomepage));

	this->m_TagPosition = 0; // 0: not set, 1: in front, 2: behind
	this->m_isdeleted = 0;
}

void MMG_Clan::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteString(this->m_ClanName);
	aMessage->WriteString(this->m_FullClanTag);
	
	aMessage->WriteString(this->m_ClanMotto);
	aMessage->WriteString(this->m_ClanMessageoftheday);
	aMessage->WriteString(this->m_ClanHomepage);
	
	aMessage->WriteUInt(this->m_MemberCount);
	for (int i = 0; i < this->m_MemberCount; i++)
		aMessage->WriteUInt(this->m_MemberIds[i]);
	
	aMessage->WriteUInt(this->m_ClanId);
	aMessage->WriteUInt(this->m_PlayerOfTheWeekId);
}

bool MMG_Clan::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadString(this->m_ClanName, ARRAYSIZE(this->m_ClanName)) || !aMessage->ReadString(this->m_FullClanTag, ARRAYSIZE(this->m_FullClanTag)))
		return false;
	
	if (!aMessage->ReadString(this->m_ClanMotto, ARRAYSIZE(this->m_ClanMotto)) || !aMessage->ReadString(this->m_ClanMessageoftheday, ARRAYSIZE(this->m_ClanMessageoftheday)) || !aMessage->ReadString(this->m_ClanHomepage, ARRAYSIZE(this->m_ClanHomepage)))
		return false;

	if (!aMessage->ReadUInt(this->m_MemberCount))
		return false;

	for (int i = 0; i < this->m_MemberCount; i++)
	{
		if (aMessage->ReadUInt(this->m_MemberIds[i]))
			return false;
	}

	if (!aMessage->ReadUInt(this->m_ClanId) || !aMessage->ReadUInt(this->m_PlayerOfTheWeekId))
		return false;
	
	return true;
}