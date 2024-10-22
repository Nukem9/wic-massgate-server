#include "../stdafx.h"

MMG_Profile::MMG_Profile()
{
	memset(this->m_Name, 0, sizeof(this->m_Name));
	this->m_ProfileId		= 0;
	this->m_ClanId			= 0;
	this->m_OnlineStatus	= 0;
	this->m_Rank			= 0;
	this->m_RankInClan		= 0;
}

void MMG_Profile::setClanId(uint clanid, uchar rank)
{
	this->m_ClanId = clanid;
	this->m_RankInClan = clanid > 0 ? rank : 0;
	this->notifyObservers(StateType::ClanId);
}

void MMG_Profile::setOnlineStatus(uint status)
{
	this->m_OnlineStatus = status;
	this->notifyObservers(StateType::OnlineStatus);
}

void MMG_Profile::setRank(uchar rank)
{
	this->m_Rank = rank;
	this->notifyObservers(StateType::Rank);
}

void MMG_Profile::setRankInClan(uchar rank)
{
	this->m_RankInClan = rank;
	this->notifyObservers(StateType::RankInClan);
}

void MMG_Profile::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteString(this->m_Name);
	aMessage->WriteUInt(this->m_ProfileId);
	aMessage->WriteUInt(this->m_ClanId);
	aMessage->WriteUInt(this->m_OnlineStatus);
	aMessage->WriteUChar(this->m_Rank);
	aMessage->WriteUChar(this->m_RankInClan);
}

bool MMG_Profile::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadString(this->m_Name, ARRAYSIZE(this->m_Name)))
		return false;

	if (!aMessage->ReadUInt(this->m_ProfileId) || !aMessage->ReadUInt(this->m_ClanId) || !aMessage->ReadUInt(this->m_OnlineStatus))
		return false;

	if (!aMessage->ReadUChar(this->m_Rank) || !aMessage->ReadUChar(this->m_RankInClan))
		return false;

	return true;
}