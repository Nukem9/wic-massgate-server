#include "../stdafx.h"

MMG_TrackablePlayerVariables::MMG_TrackablePlayerVariables()
{
	this->m_ProfileId = 0;
	this->m_Score = 0;
}

void MMG_TrackablePlayerVariables::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUInt(this->m_ProfileId);
	aMessage->WriteUInt(this->m_Score);
}

bool MMG_TrackablePlayerVariables::FromStream(MN_ReadMessage *aMessage)
{

	if (!aMessage->ReadUInt(this->m_ProfileId) || !aMessage->ReadUInt(this->m_Score))
		return false;

	return true;
}