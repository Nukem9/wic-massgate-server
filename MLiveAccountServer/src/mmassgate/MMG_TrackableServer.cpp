#include "../stdafx.h"

MMG_TrackableServer::MMG_TrackableServer()
{
	m_ServerList.clear();
}

bool MMG_TrackableServer::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);

	//
	// All dedicated sever packets require an authenticated connection, except
	// for the initial auth delimiter
	//
	auto server = FindServer(aClient);

	if (!server &&
		aDelimiter != MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION)
	{
		// Unauthed messages ignored
		return false;
	}

	// Handle all message types and disconnect the server on any errors
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

			//if (publicServerId != server->m_Info.m_ServerId)
				// ?????

			// Pong!
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_PONG);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		// List of banned words in chat
		case MMG_ProtocolDelimiters::MESSAGING_DS_GET_BANNED_WORDS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_DS_GET_BANNED_WORDS_REQ:");

			// See MMG_BannedWordsProtocol::GetRsp::FromStream
		}
		break;

		// Initial message from any server that wants to connect to Massgate
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_AUTH_DS_CONNECTION:");

			ushort protocolVersion;// MassgateProtocolVersion
			uint keySequenceNumber;// See EncryptionKeySequenceNumber in MMG_AccountProtocol
			
			if (!aMessage->ReadUShort(protocolVersion))
				return false;
			
			if (!aMessage->ReadUInt(keySequenceNumber))
				return false;

			// Drop immediately if key not valid
			if (!AuthServer(aClient, keySequenceNumber, protocolVersion))
			{
				DebugLog(L_INFO, "Failed to authenticate server");
				return false;
			}

			// If a valid sequence was supplied, ask for the "encrypted" quiz answer. This is set to 0
			// if no cdkey is used (unranked).
			if (keySequenceNumber != 0)
			{
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_FROM_MASSGATE);
				responseMessage.WriteUInt(12345678);// Quiz seed

				if (!aClient->SendData(&responseMessage))
					return false;
			}
		}
		break;

		// Encrypted quiz answer response from the dedicated server when using a cdkey
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_QUIZ_ANSWERS_TO_MASSGATE:");

			// quizAnswer = quizSeed
			//
			// MMG_BlockTEA::SetKey(CDKeyEncryptionKey);
			// MMG_BlockTEA::Decrypt(&quizAnswer, 4);
			// quizAnswer = (quizAnswer >> 16) | 65183 * quizAnswer;
			// MMG_BlockTEA::Encrypt(&quizAnswer, 4);

			uint quizAnswer;
			if (!aMessage->ReadUInt(quizAnswer))
				return false;
		}
		break;

		// Broadcasted message when the very first map is loaded
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_STARTED:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_STARTED:");

			MMG_ServerStartupVariables startupVars;
			if (!startupVars.FromStream(aMessage))
				return false;

			// Request to "connect" the active game server
			if (ConnectServer(server, &startupVars))
			{
				// Tell SvClientManager
				aClient->SetIsServer(true);
				aClient->SetLoginStatus(true);
				aClient->SetTimeout(WIC_HEARTBEAT_NET_TIMEOUT);

				DebugLog(L_INFO, "Info: %ws %s %X %d %d",
					startupVars.m_ServerName,
					startupVars.m_PublicIp,
					startupVars.m_Ip,
					startupVars.m_MassgateCommPort,
					startupVars.m_ServerReliablePort);

				// Send back the assigned public ID
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_PUBLIC_ID);
				responseMessage.WriteUInt(server->m_Info.m_ServerId);// ServerId

				if (!aClient->SendData(&responseMessage))
					return false;

				// Send back the connection cookie
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_INTERNAL_AUTHTOKEN);
				responseMessage.WriteRawData(&server->m_Cookie, sizeof(server->m_Cookie));// myCookie
				responseMessage.WriteUInt(10000);// myConnectCookieBase

				if (!aClient->SendData(&responseMessage))
					return false;
			}
			else
			{
				DebugLog(L_INFO, "Failed to connect server");
			}
		}
		break;

		// Map rotation cycle info
		case MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_MAP_LIST:
		{
			DebugLog(L_INFO, "SERVERTRACKER_SERVER_MAP_LIST:");

			ushort mapCount;
			aMessage->ReadUShort(mapCount);

			for (int i = 0; i < mapCount; i++)
			{
				uint64 mapHash;
				wchar_t mapName[128];

				aMessage->ReadUInt64(mapHash);
				aMessage->ReadString(mapName, ARRAYSIZE(mapName));

				DebugLog(L_INFO, "%llX - %ws", mapHash, mapName);
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

			if (!UpdateServer(server, &heartbeat))
				return false;
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}

bool MMG_TrackableServer::AuthServer(SvClient *aClient, uint aKeySequence, ushort aProtocolVersion)
{
	//
	// Protocol and key validation (similar to the client)
	//
	//if (aProtocolVersion != MassgateProtocolVersion)
	//	return false;

	if (aKeySequence != 0)
	{
		// TODO: Query database
		// TODO: Generate quiz answer
		// return false;
	}

	//
	// Add entry to master list
	//
	Server masterEntry(aClient->GetIPAddress(), aClient->GetPort());
	masterEntry.m_Index			= this->m_ServerList.size();
	masterEntry.m_KeySequence	= aKeySequence;

	this->m_ServerList.push_back(masterEntry);

	// Sanity check
	assert(FindServer(aClient) != nullptr);
	return true;
}

bool MMG_TrackableServer::ConnectServer(MMG_TrackableServer::Server *aServer, MMG_ServerStartupVariables *aStartupVars)
{
	//
	// Protocol validation
	//
	//if (aStartupVars->m_ProtocolVersion != MassgateProtocolVersion)
	//	return false;

	//if (aStartupVars->m_GameVersion != VER_1011)
	//	return false;

	//
	// License validation
	//
	if (aStartupVars->somebits.bitfield4)
	{
		// If ranked and mod, drop
		if (aStartupVars->m_ModId != 0)
			return false;

		// If ranked and password, drop
		if (aStartupVars->somebits.bitfield2)
			return false;

		// If ranked and has no license, drop
		if (!aServer->m_KeyAuthenticated)
			return false;
	}

	// Mark server list entry as valid and copy information over
	aServer->m_Info				= *aStartupVars;
	aServer->m_Valid			= true;
	aServer->m_Info.m_ServerId	= aServer->m_Index + 1;
	return true;
}

bool MMG_TrackableServer::UpdateServer(MMG_TrackableServer::Server *aServer, MMG_TrackableServerHeartbeat *aHeartbeat)
{
	// Compare connection cookies
	if (aServer->m_Cookie.hash != aHeartbeat->m_Cookie.hash ||
		aServer->m_Cookie.trackid != aHeartbeat->m_Cookie.trackid)
	{
		__debugbreak();
		return false;
	}

	// Copy heartbeat variables over to the startup info structure
	aServer->m_Info.m_CurrentMapHash	= aHeartbeat->m_CurrentMapHash;
	aServer->m_Info.somebits.bitfield1	= aHeartbeat->m_MaxNumPlayers;
	aServer->m_Heartbeat				= *aHeartbeat;
	return true;
}

void MMG_TrackableServer::DisconnectServer(MMG_TrackableServer::Server *aServer)
{
}

MMG_TrackableServer::Server *MMG_TrackableServer::FindServer(SvClient *aClient)
{
	uint clientIp	= aClient->GetIPAddress();
	uint clientPort = aClient->GetPort();

	for (auto& server : this->m_ServerList)
	{
		if (server.TestIPHash(clientIp, clientPort))
			return &server;
	}

	return nullptr;
}