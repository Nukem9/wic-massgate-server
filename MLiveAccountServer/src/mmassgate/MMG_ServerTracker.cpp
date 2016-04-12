#include "../stdafx.h"

/*
171 SERVERTRACKER_USER_NUM_TOTAL_SERVERS

172 SERVERTRACKER_USER_SHORT_SERVER_INFO

170 SERVERTRACKER_USER_NO_MORE_SERVER_INFO

173 SERVERTRACKER_USER_COMPLETE_SERVER_INFO

176 SERVERTRACKER_USER_LIST_SERVERS

184 SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP

186 SERVERTRACKER_USER_FRIENDS_LADDER_GET_RSP

188 SERVERTRACKER_USER_CLAN_LADDER_RESULTS_START

189 SERVERTRACKER_USER_CLAN_LADDER_SINGLE_ITEM

190 SERVERTRACKER_USER_CLAN_LADDER_NO_MORE_RESULTS

192 SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_RSP

182 SERVERTRACKER_USER_CYCLE_MAP_LIST_RSP

194 SERVERTRACKER_USER_TOURNAMENT_MATCH_SERVER_SHORT_INFO

196 SERVERTRACKER_USER_PLAYER_STATS_RSP

198 SERVERTRACKER_USER_PLAYER_MEDALS_RSP

200 SERVERTRACKER_USER_PLAYER_BADGES_RSP

202 SERVERTRACKER_USER_CLAN_STATS_RSP

204 SERVERTRACKER_USER_CLAN_MEDALS_RSP

208 SERVERTRACKER_USER_CLAN_MATCH_HISTORY_LISTING_RSP

210 SERVERTRACKER_USER_CLAN_MATCH_HISTORY_DETAILS_RSP

206 SERVERTRACKER_USER_KEEPALIVE_RSP
*/

MMG_ServerTracker::Pinger::Pinger()
{
}

bool MMG_ServerTracker::PrivHandlePingers()
{
	return true;
}

MMG_ServerTracker::MMG_ServerTracker()
{
}

bool MMG_ServerTracker::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);
	MN_WriteMessage	responseMessage3(2048);
	
	//wchar_t messageheader1[2];				// first 8 byte of message are constant for whatever reason, not even profile / account influences this
	uchar messageheader[4];					// first 8 byte of message are constant for whatever reason, not even profile / account influences this
	uint messageheader2;					// first 8 byte of message are constant for whatever reason, not even profile / account influences this
	ushort activefilters;					// active filters flags, tells which data is actually sent to massgate
	uchar gamemode[3], rankbalanced;		// filters to be read from message
	uchar dedicated, passworded;			// filters to be read from message
	uchar ranked, population, mod;			// filters to be read from message
	uchar mapnamelength, servernamelength;	// filters to be read from message
	// friends on server and ping filter options do not change the incoming message
	
	wchar_t mapname[16], servername[16];	// filter strings to be read from message
	//memset(messageheader1, 0, sizeof(messageheader1));
	memset(mapname, 0, sizeof(mapname));
	memset(servername, 0, sizeof(servername));

	switch(aDelimiter)
	{
		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGREQUEST:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_MINIPINGREQUEST:");
			//responseDelimeter = MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGRESPONSE;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_LIST_SERVERS:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS:");

			uint CurrentlyRegisteredServers[2];
			uint NumberOfServers = sizeof(CurrentlyRegisteredServers)/sizeof(CurrentlyRegisteredServers[0]);
			int i = 0;

			/* this is the incoming message format:
			message header: 8 byte with whatever information
			2 byte active filters information
			at least 2 bytes detailed filter settings		*/

			// to do: replace with actual header parts
			
			//aMessage->ReadString(messageheader1, ARRAYSIZE(messageheader1));
			for (i = 0; i < 4; i++)					// according to IDA, this is actually a string
			{										// constant value of 35, 0, 0, 0 during message analysis
				aMessage->ReadUChar(messageheader[i]);
				ranked = messageheader[i];
				ranked = ranked;
			}
			aMessage->ReadUInt(messageheader2);		// according to IDA, this must be an int, constant value of 126 (0x7E) during message analysis

			aMessage->ReadUShort(activefilters);
			/* active filters information flag order
			1 bit dedicated filter
			1 bit ranked filter
			1 bit rank balanced filter
			3 bit game mode filter
			1 bit population
				1 bit empty / always 0
			1 bit mapname filter
			1 bit servername filter
			1 bit passworded filter
			1 bit mod filter
				2 bit empty / always 0												*/

			/* detailed filter settings format (data format):
			if a flag is not set to 1, then the related bytes are not in the message
			1 byte: is dedicated server
			1 byte: is ranked game
			1 byte: is rank balanced game
			1 byte: is domination game
			1 byte: is assault game
			1 byte: is tug of war game
			1 byte: server population
			1 byte: map name filter string length (filter string is in unicode)
			1 byte: nullbyte
			variable length: filter string for map name, null terminated
			1 byte: server name filter string length (filter string is in unicode)
			1 byte: nullbyte
			variable length: filter string for server name, null terminated
			1 byte: is passworded game
			1 byte: is modded
			1 byte: nullbyte														*/
			
			// bitwise check for active filter flags
			if (0x0001 == (activefilters & 0x0001)) // dedicated flag is at 0x0001
			{
				aMessage->ReadUChar(dedicated);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: dedicated filter %u", dedicated);
			}

			if (0x0002 == (activefilters & 0x0002)) // ranked flag is at 0x0002
			{
				aMessage->ReadUChar(ranked);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: ranked filter %u", ranked);
			}

			if (0x0004 == (activefilters & 0x0004)) // rankbalanced flag is at 0x0004
			{
				aMessage->ReadUChar(rankbalanced);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: rank balanced filter %u", rankbalanced);
			}

			if (0x0038 == (activefilters & 0x0038)) // gamemode flags are at 0x0038
			{
				aMessage->ReadUChar(gamemode[0]);
				aMessage->ReadUChar(gamemode[1]);
				aMessage->ReadUChar(gamemode[2]);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: gamemode filter %u %u %u", gamemode[0], gamemode[1], gamemode[2]);

				if (gamemode[0])
				{
					DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: domination");
				}
				if (gamemode[1])
				{
					DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: assault");
				}
				if (gamemode[2])
				{
					DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: tugofwar");
				}
			}

			if (0x0100 == (activefilters & 0x0100)) // population four-state flag is at 0x0100
			{
				aMessage->ReadUChar(population);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: population filter %u", population);
			}

			if (0x0400 == (activefilters & 0x0400)) // mapname flag is at 0x0400
			{
				aMessage->ReadString(mapname, ARRAYSIZE(mapname));
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: mapname filter %ws", mapname);
			}

			if (0x0800 == (activefilters & 0x0800)) // servername flag is at 0x0800
			{
				aMessage->ReadString(servername, ARRAYSIZE(servername));
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: servername filter %ws", servername);
			}

			if (0x1000 == (activefilters & 0x1000)) // passworded flag is at 0x1000
			{
				aMessage->ReadUChar(passworded);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: passworded filter %u", passworded);
			}

			if (0x2000 == (activefilters & 0x2000)) // mod flag is at 0x2000
			{
				aMessage->ReadUChar(mod);
				DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS: mod filter %u", mod);
			}

			//send found number of servers
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NUM_TOTAL_SERVERS);
			responseMessage.WriteUInt(NumberOfServers);

			if (!aClient->SendData(&responseMessage))
				return false;

			//array of servers
			const int TOTAL_SERVERS = 2;

			//list of matched servers retrieved from global list
			MMG_TrackableServerBriefInfo shortServerList[TOTAL_SERVERS];
			MMG_TrackableServerFullInfo fullServerList[TOTAL_SERVERS];

			//server 1 short info
			wcscpy(shortServerList[0].m_GameName, L"Server 1");
			shortServerList[0].m_IP = 0;
			shortServerList[0].m_ModId = 0;
			shortServerList[0].m_ServerId = 0;
			shortServerList[0].m_MassgateCommPort = 0;
			shortServerList[0].m_CycleHash = 0;
			shortServerList[0].m_ServerType = 0;
			shortServerList[0].m_IsRankBalanced = 0;

			//server 2 short info
			wcscpy(shortServerList[1].m_GameName, L"Server 2");
			shortServerList[1].m_IP = 0;
			shortServerList[1].m_ModId = 0;
			shortServerList[1].m_ServerId = 0;
			shortServerList[1].m_MassgateCommPort = 0;
			shortServerList[1].m_CycleHash = 0;
			shortServerList[1].m_ServerType = 0;
			shortServerList[1].m_IsRankBalanced = 0;

			//server 1 complete info
			fullServerList[0].m_GameVersion = 1;
			fullServerList[0].m_ProtocolVersion = 1;
			fullServerList[0].m_CurrentMapHash = 1;
			fullServerList[0].m_CycleHash = 1;
			wcscpy(fullServerList[0].m_ServerName, L"Server 1");
			fullServerList[0].m_ServerReliablePort = 3;

			//bitmask
			fullServerList[0].gapc4 = 0;
			fullServerList[0]._bf198 = 0;

			fullServerList[0].m_ServerType = 0;

			fullServerList[0].m_IP = 1;
			fullServerList[0].m_ModId = 1;
			fullServerList[0].m_MassgateCommPort = 1;
			fullServerList[0].m_GameTime = 12;
			fullServerList[0].m_ServerId = 1;
			fullServerList[0].m_CurrentLeader = 1;
			fullServerList[0].m_HostProfileId = 1;
			fullServerList[0].m_WinnerTeam = 1;

			//server 2 complete info
			fullServerList[1].m_GameVersion = 1;
			fullServerList[1].m_ProtocolVersion = 1;
			fullServerList[1].m_CurrentMapHash = 1;
			fullServerList[1].m_CycleHash = 1;
			wcscpy(fullServerList[1].m_ServerName, L"Server 2");
			fullServerList[1].m_ServerReliablePort = 3;

			//bitmask
			fullServerList[1].gapc4 = 0;
			fullServerList[1]._bf198 = 0;

			fullServerList[1].m_ServerType = 0;

			fullServerList[1].m_IP = 1;
			fullServerList[1].m_ModId = 1;
			fullServerList[1].m_MassgateCommPort = 1;
			fullServerList[1].m_GameTime = 12;
			fullServerList[1].m_ServerId = 1;
			fullServerList[1].m_CurrentLeader = 1;
			fullServerList[1].m_HostProfileId = 1;
			fullServerList[1].m_WinnerTeam = 1;

			//send matched server list
			// works but is definitely not done
			for (i = 0; i < NumberOfServers; i++)
			{
				MN_WriteMessage	responseMessage1(2048); // short server info
				MN_WriteMessage	responseMessage2(2048); // complete server info

				MMG_TrackableServerBriefInfo *currentShort;
				MMG_TrackableServerFullInfo *currentFull;
				currentShort = &shortServerList[i];
				currentFull = &fullServerList[i];

				//short server info
				responseMessage1.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);

				//write server info to stream, has the same effect as the commented block above
				currentShort->ToStream(&responseMessage1);

				if (!aClient->SendData(&responseMessage1)) //send short server info
					return false;
				
				//complete server info
				responseMessage2.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO);

				//write server info to stream
				currentFull->ToStream(&responseMessage2);
			
				//success flag?
				responseMessage2.WriteUInt(1);
			
				if (!aClient->SendData(&responseMessage2)) //send complete server info
					return false;
			}

			//send "no more server info" delimiter
			responseMessage3.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO);
			if (!aClient->SendData(&responseMessage3))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVERS_BY_NAME:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_GET_SERVERS_BY_NAME:");
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:");

			// works but is definitely not done
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_RSP);
			responseMessage.WriteUInt64(1);
			responseMessage.WriteUShort(1);
			
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:");
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:");
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ:");
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}