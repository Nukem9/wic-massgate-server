#include "../stdafx.h"

MMG_TrackableServer::MMG_TrackableServer()
{
}

bool MMG_TrackableServer::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);

	switch(aDelimiter)
	{
		// Ping/pong handler
		case MMG_ProtocolDelimiters::MESSAGING_DS_PING:
		{
			DebugLog(L_INFO, "MESSAGING_DS_PING:");
			ushort protocolVersion;// MassgateProtocolVersion
			uint publicServerId;   // Server ID sent on SERVERTRACKER_SERVER_STARTED

			if (!aMessage->ReadUShort(protocolVersion))
				return false;

			if (!aMessage->ReadUInt(publicServerId))
				return false;

			// TODO: Verify proto and pub id

			// Pong!
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_PONG);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_DS_GET_BANNED_WORDS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_DS_GET_BANNED_WORDS_REQ:");
		}
		break;

		// Initial message from any server that wants to connect to Massgate.
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:");

			ushort protocolVersion;// MassgateProtocolVersion
			uint keySequenceNumber;// See EncryptionKeySequenceNumber in MMG_AccountProtocol
			
			if (!aMessage->ReadUShort(protocolVersion))
				return false;
			
			if (!aMessage->ReadUInt(keySequenceNumber))
				return false;

			// If a valid sequence was supplied, ask for the "encrypted" quiz answer. This is set to 0
			// if no cdkey is used (unranked).
			if (keySequenceNumber != 0)
			{
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FROM_MASSGATE);
				responseMessage.WriteUInt(12345678);// Quiz seed

				if (!aClient->SendData(&responseMessage))
					return false;

				// quizAnswer = quizSeed
				//
				// MMG_BlockTEA::SetKey(CDKeyEncryptionKey);
				// MMG_BlockTEA::Decrypt(&quizAnswer, 4);
				// quizAnswer = (quizAnswer >> 16) | 65183 * quizAnswer;
				// MMG_BlockTEA::Encrypt(&quizAnswer, 4);
			}
		}
		break;

		// Encrypted quiz answer response from the dedicated server when using a cdkey
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:");

			uint quizAnswer;
			if (!aMessage->ReadUInt(quizAnswer))
				return false;
		}
		break;

		// Broadcasted startup message when a map is loaded
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STARTED:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_STARTED:");

			MMG_ServerStartupVariables startupVars;

			if (!startupVars.FromStream(aMessage))
				return false;

			aClient->SetIsServer(true);
			aClient->SetLoginStatus(true);
			aClient->SetTimeout(WIC_HEARTBEAT_NET_TIMEOUT);

			DebugLog(L_INFO, "test: %ws %s %X %d", startupVars.m_ServerName, startupVars.m_PublicIp, startupVars.m_Ip, startupVars.m_MassgateCommPort);

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

		// Map rotation cycle info from server
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

		// Server heartbeat sent every X seconds
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STATUS:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_STATUS:");

			MMG_TrackableServerHeartbeat heartbeat;

			if (!heartbeat.FromStream(aMessage))
				return false;
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}