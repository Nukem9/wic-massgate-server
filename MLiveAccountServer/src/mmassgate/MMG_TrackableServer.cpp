#include "../stdafx.h"

MMG_TrackableServer::ThreadedPingHandler::ThreadedPingHandler()
{
}

MMG_TrackableServer::MMG_TrackableServer()
{
}

bool MMG_TrackableServer::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);

	switch(aDelimiter)
	{
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:");
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}