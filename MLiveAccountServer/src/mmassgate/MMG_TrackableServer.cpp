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
		// MMG_TrackableServer::PrivHandleStartUp
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:");

			ushort protocolVersion;
			uint KeySequenceNumber;
			aMessage->ReadUShort(protocolVersion);	// Massgate protocol version
			if (!aMessage->ReadUInt(KeySequenceNumber))	// Similar to SequenceNumber in MMG_AccountProtocol
				return false;

			DebugLog(L_INFO, "Sequence: %X", KeySequenceNumber);

			// If a valid sequence was supplied, ask for the "encrypted" quiz answer
			if (KeySequenceNumber != 0)
			{
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FROM_MASSGATE);
				responseMessage.WriteUInt(10); // quiz seed

				if (!aClient->SendData(&responseMessage))
					return false;
			}
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:");

			uint quizAnswer;
			aMessage->ReadUInt(quizAnswer);

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_PUBLIC_ID);
			responseMessage.WriteUInt(4323);//serverId

			if (!aClient->SendData(&responseMessage))
				return false;

			char someData[16];
			memset(someData, 55, 16);

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_INTERNAL_AUTHTOKEN);
			responseMessage.WriteRawData(someData, 16);//myCookie
			responseMessage.WriteUInt(10000);//myConnectCookieBase

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STARTED:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_STARTED:");

			MMG_ServerStartupVariables startupVars;

			if (!startupVars.FromStream(aMessage))
				return false;

			DebugLog(L_INFO, "test: %ws %s %d %d", startupVars.m_ServerName, startupVars.m_PublicIp, (int)startupVars.somebits.bitfield1, (int)startupVars.somebits.bitfield2);
		}
		break;

		// MMG_TrackableServer::Update
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST:
		{
			ushort mapCount;
			aMessage->ReadUShort(mapCount);

			DebugLog(L_INFO, "SERVERTRACKER_SERVER_MAP_LIST:");

			for (int i = 0; i < mapCount; i++)
			{
				uint64 mapHash;
				wchar_t mapName[128];

				aMessage->ReadUInt64(mapHash);
				aMessage->ReadString(mapName, ARRAYSIZE(mapName));

				DebugLog(L_INFO, "%ws", mapName);
			}
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STATUS:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_STATUS:");
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}