#pragma once

class MMG_Profile : public MMG_IStreamable
{
public:
	wchar_t m_Name[WIC_NAME_MAX_LENGTH];
	uint	m_ProfileId;
	uint	m_ClanId;
	uint	m_OnlineStatus;
	uchar	m_Rank;
	uchar	m_RankInClan;

private:

public:
	MMG_Profile();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};