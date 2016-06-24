#include "../stdafx.h"

MMG_TrackableServerHeartbeat::MMG_TrackableServerHeartbeat()
{
	m_CurrentMapHash		= 0;
	m_GameTime				= 0.0f;
	memset(m_PlayersInGame, 0, sizeof(m_PlayersInGame));
	m_CurrentLeader			= 0;
	m_MaxNumPlayers			= 0;
	m_NumPlayers			= 0;
	memset(&m_Cookie, 0, sizeof(m_Cookie));
}

void MMG_TrackableServerHeartbeat::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt64(this->m_CurrentMapHash);
	aMessage->WriteUInt(this->m_CurrentLeader);
	aMessage->WriteUChar(this->m_MaxNumPlayers);
	aMessage->WriteUChar(this->m_NumPlayers);
	aMessage->WriteFloat(this->m_GameTime);
	aMessage->WriteRawData(&this->m_Cookie, sizeof(this->m_Cookie));

	// Write array of player profile IDs
	for (int i = 0; i < this->m_NumPlayers; i++)
		aMessage->WriteUInt(this->m_PlayersInGame[i]);
}

bool MMG_TrackableServerHeartbeat::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUInt64(this->m_CurrentMapHash)
		|| !aMessage->ReadUInt(this->m_CurrentLeader)
		|| !aMessage->ReadUChar(this->m_MaxNumPlayers)
		|| !aMessage->ReadUChar(this->m_NumPlayers)
		|| !aMessage->ReadFloat(this->m_GameTime)
		|| !aMessage->ReadRawData(&this->m_Cookie, sizeof(this->m_Cookie), nullptr))
		return false;

	// Read player array
	if (this->m_NumPlayers >= ARRAYSIZE(this->m_PlayersInGame))
		return false;

	for (int i = 0; i < this->m_NumPlayers; i++)
	{
		if (!aMessage->ReadUInt(this->m_PlayersInGame[i]))
			return false;
	}

	return true;
}