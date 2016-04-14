#include "../stdafx.h"

MMG_Chat::MMG_Chat()
{
}

bool MMG_Chat::HandleDelayedItems()
{
	return true;
}

bool MMG_Chat::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);

	switch(aDelimiter)
	{
		case MMG_ProtocolDelimiters::CHAT_REAL_JOIN_REQ:
		{
			DebugLog(L_INFO, "CHAT_REAL_JOIN_REQ:");
		}
		break;

		case MMG_ProtocolDelimiters::CHAT_LIST_ROOMS_REQ:
		{
			DebugLog(L_INFO, "CHAT_LIST_ROOMS_REQ:");
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}