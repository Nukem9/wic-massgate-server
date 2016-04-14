#include "../stdafx.h"

MMG_ProfileEditableVariablesProtocol::MMG_ProfileEditableVariablesProtocol()
{
	memset(this->m_Motto, 0, sizeof(this->m_Motto));
	memset(this->m_Homepage, 0, sizeof(this->m_Homepage));
	
}

void MMG_ProfileEditableVariablesProtocol::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteString(this->m_Motto);
	aMessage->WriteString(this->m_Homepage);
	
}

bool MMG_ProfileEditableVariablesProtocol::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadString(this->m_Motto, ARRAYSIZE(this->m_Motto)))
		return false;

	if (!aMessage->ReadString(this->m_Homepage, ARRAYSIZE(this->m_Homepage)))
		return false;

	return true;
}