#pragma once

class MMG_Clan : public MMG_IStreamable
{
public:
	wchar_t m_ClanName[WIC_CLANNAME_MAX_LENGTH];
	wchar_t m_FullClanTag[WIC_CLANTAG_MAX_LENGTH];
	uint	m_ClanId;
	uint	m_MemberCount;
	uint	m_PlayerOfTheWeekId;
	uint	m_MemberIds[512];
	wchar_t m_ClanMotto[WIC_MOTTO_MAX_LENGTH];
	wchar_t m_ClanMessageoftheday[WIC_MOTD_MAX_LENGTH];
	wchar_t m_ClanHomepage[WIC_HOMEPAGE_MAX_LENGTH];
	// not part of original class
	uchar	m_TagPosition;
	uchar	m_isdeleted;
	
private:

public:
	MMG_Clan();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};
