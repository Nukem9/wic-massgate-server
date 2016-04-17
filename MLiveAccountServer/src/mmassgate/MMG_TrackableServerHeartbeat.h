#pragma once

class MMG_TrackableServerHeartbeat : public MMG_IStreamable
{
public:
	uint64						m_CurrentMapHash;
	float						m_GameTime;
	uint						m_PlayersInGame[64];
	uint						m_CurrentLeader;
	uchar						m_MaxNumPlayers;
	uchar						m_NumPlayers;
	MMG_TrackableServerCookie	m_Cookie;

private:

public:
	MMG_TrackableServerHeartbeat();

	void ToStream	(MN_WriteMessage *aMessage);
	bool FromStream	(MN_ReadMessage *aMessage);
};