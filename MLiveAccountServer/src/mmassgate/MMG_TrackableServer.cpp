#include "../stdafx.h"

MMG_TrackableServer::MMG_TrackableServer() : m_ServerList()
{
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

			//if (publicServerId != server->m_PublicId)
				// ?????

			// Pong!
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_PONG);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_DS_INFORM_PLAYER_JOINED:
		{
			DebugLog(L_INFO, "MESSAGING_DS_INFORM_PLAYER_JOINED:");

			uint profileId, antiSpoofToken;

			if (!aMessage->ReadUInt(profileId) || !aMessage->ReadUInt(antiSpoofToken))
				return false;

#ifdef USING_MYSQL_DATABASE
			SvClient *player = MMG_AccountProxy::ourInstance->GetClientByProfileId(profileId);

			// if profileId not logged in, kick profileId
			if (!player)
			{
				DebugLog(L_INFO, "MESSAGING_DS_KICK_PLAYER:");
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_KICK_PLAYER);
				responseMessage.WriteUInt(profileId);

				if (!aClient->SendData(&responseMessage))
					return false;
			}
			else
			{
				//if antispooftoken does not match the generated token from ACCOUNT_AUTH_ACCOUNT_REQ, then kick profileId
				MMG_AuthToken *authtoken = player->GetToken();

				// TODO
				if (authtoken->m_TokenId != antiSpoofToken)
				{
					DebugLog(L_INFO, "MESSAGING_DS_KICK_PLAYER:");
					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_DS_KICK_PLAYER);
					responseMessage.WriteUInt(profileId);

					if (!aClient->SendData(&responseMessage))
						return false;
				}
			}
#endif
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

			// wic_ds sends populated m_PublicIp if firewall flag is set
			// use the PrivateIP/PublicIp as the servers' ip address
			if (strlen(startupVars.m_PublicIp) > 0)
				startupVars.m_Ip = inet_addr(startupVars.m_PublicIp);
			else
				startupVars.m_Ip = ntohl(aClient->GetIPAddress());

			// Request to "connect" the active game server
			if (ConnectServer(server, &startupVars))
			{
				// Tell SvClientManager
				aClient->SetIsServer(true);
				aClient->SetLoginStatus(true);
				aClient->SetTimeout(WIC_HEARTBEAT_NET_TIMEOUT);
				
				struct sockaddr_in temp;
				temp.sin_addr.S_un.S_addr = startupVars.m_Ip;

				DebugLog(L_INFO, "Info: %ws %s %s %d %d",
					startupVars.m_ServerName,
					startupVars.m_PublicIp,
					inet_ntoa(temp.sin_addr),
					startupVars.m_MassgateCommPort,
					startupVars.m_ServerReliablePort);

				// Send back the assigned public ID
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_SERVER_PUBLIC_ID);
				responseMessage.WriteUInt(server->m_PublicId);// ServerId

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

	// TODO
	// small issue with m_Index being set to the size() of the map
	// ie 2 indexes can have the value 4 if the size() of the map is 4
	masterEntry.m_Index			= this->m_ServerList.size();
	masterEntry.m_KeySequence	= aKeySequence;

	//this->m_ServerList.push_back(masterEntry);
	if (this->m_ServerList.find(masterEntry.m_PublicId) == this->m_ServerList.end())
		this->m_ServerList.insert(std::pair<uint, Server>(masterEntry.m_PublicId, masterEntry));

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
	if (aStartupVars->somebits.Ranked)
	{
		// If ranked and mod, drop
		if (aStartupVars->m_ModId != 0)
			return false;

		// If ranked and password, drop
		if (aStartupVars->somebits.Passworded)
			return false;

		// If ranked and has no license, drop
		if (!aServer->m_KeyAuthenticated)
			return false;
	}

	// Mark server list entry as valid and copy information over
	aServer->m_Info				= *aStartupVars;
	aServer->m_Valid			= true;
	//aServer->m_Info.m_ServerId	= aServer->m_Index + 1;
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
	//aServer->m_Info.m_CurrentMapHash	= aHeartbeat->m_CurrentMapHash;
	//aServer->m_Info.somebits.bitfield1	= aHeartbeat->m_MaxNumPlayers;
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

	/*for (auto& server : this->m_ServerList)
	{
		if (server.TestIPHash(clientIp, clientPort))
			return &server;
	}
	*/

	std::map<uint, Server>::iterator iter;

	iter = this->m_ServerList.find(clientIp ^ (clientPort + 0x1010101));
	if (iter != this->m_ServerList.end())
		return &iter->second;

	return nullptr;
}

bool MMG_TrackableServer::FilterCheck(MMG_ServerFilter *filters, Server *aServer)
{
	// todo MAPNAME_FLAG, SERVERNAME_FLAG
	// still needs work

	// temp fix for the playnow button only returning ranked servers
	// remove when ranking works
	if (filters->HasFlag(PLAYNOW_FLAG))
		return true;

	if (filters->HasFlag(DEDICATED_FLAG))
	{
		if (filters->isDedicated && !aServer->m_Info.somebits.Dedicated)	// dedicated
			return false;

		if (!filters->isDedicated && aServer->m_Info.somebits.Dedicated)	// not dedicated
			return false;
	}

	if (filters->HasFlag(RANKED_FLAG))
	{
		if (filters->isRanked && !aServer->m_Info.somebits.Ranked)			// ranked
			return false;

		if (!filters->isRanked && aServer->m_Info.somebits.Ranked)			// not ranked
			return false;
	}

	if (filters->HasFlag(RANKBALANCE_FLAG)
		&& (filters->isRankBalanced && !aServer->m_Info.m_IsRankBalanced)	// rank balanced
		|| (!filters->isRankBalanced && aServer->m_Info.m_IsRankBalanced))	// not rank balanced
		return false;

	if (filters->HasFlag(GAMEMODE_DOM_FLAG)										// domination only
		&& (filters->hasDominationMaps && aServer->m_Info.m_HasAssaultMaps)
		|| (filters->hasDominationMaps && aServer->m_Info.m_HasTowMaps))
		return false;

	if (filters->HasFlag(GAMEMODE_AST_FLAG)										// assault only
		&& (filters->hasAssaultMaps && aServer->m_Info.m_HasDominationMaps)
		|| (filters->hasAssaultMaps && aServer->m_Info.m_HasTowMaps))
		return false;

	if (filters->HasFlag(GAMEMODE_TOW_FLAG)										// tow only
		&& (filters->hasTowMaps && aServer->m_Info.m_HasDominationMaps)
		|| (filters->hasTowMaps && aServer->m_Info.m_HasAssaultMaps))
		return false;

	//if (filters->HasFlag(POPULATION_FLAG))
	//{
	//	if (filters->population == 0) { /* no filter, do nothing*/ }
	//}

	if (filters->HasFlag(NOTFULLEMPTY_FLAG)									// not full
		&& (filters->notFullEmpty == 1)
		&& (aServer->m_Heartbeat.m_NumPlayers == aServer->m_Heartbeat.m_MaxNumPlayers))
		return false;

	if (filters->HasFlag(NOTFULLEMPTY_FLAG)									// not empty
		&& (filters->notFullEmpty == 2)
		&& (aServer->m_Heartbeat.m_NumPlayers < 1))
		return false;

	if (filters->HasFlag(NOTFULLEMPTY_FLAG)									// not empty or full
		&& (filters->notFullEmpty == 3)
		&& (aServer->m_Heartbeat.m_NumPlayers < 1 || aServer->m_Heartbeat.m_NumPlayers == aServer->m_Heartbeat.m_MaxNumPlayers))
		return false;

	if (filters->HasFlag(PLAYNOW_FLAG))										// play now button
	{
		// TODO: sort list by player count descending

		if (filters->HasFlag(TOTALSLOTS_FLAG)								// play now max slots
			&& (filters->totalSlots != aServer->m_Heartbeat.m_MaxNumPlayers))
			return false;

		if (filters->HasFlag(EMPTYSLOTS_FLAG)								// play now available slots
			&& (filters->emptySlots > aServer->m_Heartbeat.m_MaxNumPlayers - aServer->m_Heartbeat.m_NumPlayers))
			return false;
	}

	//if (this->HasFlag(MAPNAME_FLAG))
	//if (this->HasFlag(SERVERNAME_FLAG))

	if (filters->HasFlag(PASSWORD_FLAG))
	{
		if (filters->requiresPassword && !aServer->m_Info.somebits.Passworded)	// need password
			return false;

		if (!filters->requiresPassword && aServer->m_Info.somebits.Passworded)	// no password
			return false;
	}

	if (filters->HasFlag(MOD_FLAG))
	{
		if (filters->noMod && (aServer->m_Info.m_ModId == 0))				// no mods
			return false;

		if (!filters->noMod && (aServer->m_Info.m_ModId > 0))				// only mods
			return false;
	}

	return true;
}

bool MMG_TrackableServer::GetServerListInfo(MMG_ServerFilter *filters, std::vector<MMG_TrackableServerFullInfo> *aFullInfo, std::vector<MMG_TrackableServerBriefInfo> *aBriefInfo, uint *aCount)
{
	// Atleast one parameter must be supplied
	if (!aFullInfo && !aBriefInfo && !aCount)
		return false;

	// Reset any initial data
	if (aFullInfo)
		aFullInfo->clear();

	if (aBriefInfo)
		aBriefInfo->clear();

	if (aCount)
		*aCount = this->m_ServerList.size();

	std::map<uint, Server>::iterator iter;

	// Loop through each master list entry and convert the structures
	//for (auto& server : this->m_ServerList)
	for (iter = this->m_ServerList.begin(); iter != this->m_ServerList.end(); ++iter)
	{
		auto server = iter->second;

		MMG_TrackableServerFullInfo fullInfo;
		MMG_TrackableServerBriefInfo briefInfo;

		// Full tracker information
		if (aFullInfo)
		{
			fullInfo.m_GameVersion			= server.m_Info.m_GameVersion;
			fullInfo.m_ProtocolVersion		= PROTO_1012;
			fullInfo.m_CurrentMapHash		= server.m_Heartbeat.m_CurrentMapHash;
			fullInfo.m_CycleHash			= 0;// TODO
			wcscpy_s(fullInfo.m_ServerName, (const wchar_t *)server.m_Info.m_ServerName);
			fullInfo.m_ServerReliablePort	= server.m_Info.m_ServerReliablePort;

			uint playingCount = 0;
			for (int i = 0; i < 64; i++)
			{
				fullInfo.m_Players[i].m_ProfileId = server.m_Heartbeat.m_PlayersInGame[i];	// fullInfo.m_Players[i].m_Score = 0;
				if (fullInfo.m_Players[i].m_ProfileId > 0 && fullInfo.m_Players[i].m_Score > 0)
					playingCount++;
			}

			fullInfo.bf_MaxPlayers			= 0;	//
			fullInfo.bf_PlayerCount			= 0;	// TODO
			fullInfo.bf_SpectatorCount		= 0;	//
			fullInfo.bf_ServerType			= server.m_Info.m_ServerType;
			fullInfo.bf_RankedFlag			= server.m_Info.somebits.Ranked;
			fullInfo.bf_RankBalanceTeams	= server.m_Info.m_IsRankBalanced;
			fullInfo.bf_HasDomMaps			= server.m_Info.m_HasDominationMaps;
			fullInfo.bf_HasAssaultMaps		= server.m_Info.m_HasAssaultMaps;
			fullInfo.bf_HasTugOfWarMaps		= server.m_Info.m_HasTowMaps;

			fullInfo.m_ServerType			= server.m_Info.m_ServerType;
			fullInfo.m_IP					= server.m_Info.m_Ip;
			fullInfo.m_ModId				= server.m_Info.m_ModId;
			fullInfo.m_MassgateCommPort		= server.m_Info.m_MassgateCommPort;
			fullInfo.m_GameTime				= server.m_Heartbeat.m_GameTime;
			fullInfo.m_ServerId				= server.m_PublicId;
			fullInfo.m_CurrentLeader		= server.m_Heartbeat.m_CurrentLeader;
			fullInfo.m_HostProfileId		= server.m_Info.m_HostProfileId;
			fullInfo.m_WinnerTeam			= 0;

			// server filter check (mostly untested)
			if (this->FilterCheck(filters, &server))
				aFullInfo->insert(aFullInfo->end(), fullInfo);
		}

		// Brief tracker information
		if (aBriefInfo)
		{
			wcscpy_s(briefInfo.m_GameName, (const wchar_t *)fullInfo.m_ServerName);
			briefInfo.m_IP					= fullInfo.m_IP;
			briefInfo.m_ModId				= fullInfo.m_ModId;
			briefInfo.m_ServerId			= fullInfo.m_ServerId;
			briefInfo.m_MassgateCommPort	= fullInfo.m_MassgateCommPort;
			briefInfo.m_CycleHash			= fullInfo.m_CycleHash;
			briefInfo.m_ServerType			= fullInfo.m_ServerType;
			briefInfo.m_IsRankBalanced		= server.m_Info.m_IsRankBalanced;

			// server filter check (mostly untested)
			if (this->FilterCheck(filters, &server))
				aBriefInfo->insert(aBriefInfo->end(), briefInfo);
		}
	}

	return true;
}