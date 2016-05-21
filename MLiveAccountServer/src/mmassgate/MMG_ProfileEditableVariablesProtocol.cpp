#include "../stdafx.h"

MMG_ProfileEditableVariablesProtocol::MMG_ProfileEditableVariablesProtocol()
{
}

void MMG_ProfileEditableVariablesProtocol::GetRsp::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteString(this->motto);
	aMessage->WriteString(this->homepage);
}

bool MMG_ProfileEditableVariablesProtocol::GetRsp::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadString(this->motto, ARRAYSIZE(this->motto)))
		return false;

	if (!aMessage->ReadString(this->homepage, ARRAYSIZE(this->homepage)))
		return false;

	return true;
}

MMG_ProfileEditableVariablesProtocol::GetRsp::GetRsp()
{
	memset(this->motto, 0, sizeof(this->motto));
	memset(this->homepage, 0, sizeof(this->homepage));
}