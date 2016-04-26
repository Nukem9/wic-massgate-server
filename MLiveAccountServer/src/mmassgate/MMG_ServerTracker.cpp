#include "../stdafx.h"

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

struct cycletest
{
	uint64 hash;
	wchar_t *mapname;
};

cycletest cycles[] =
{
	{ 0x01574C4E7CBEF3D7, L"do_Apocalypse" },
	{ 0x366789C15AE10C5E, L"tw_Radar" },
	{ 0x36F05A2AA0295C85, L"do_Canal" },
	{ 0x429233342E470501, L"do_Studio" },
	{ 0x4C00F431FBEAE817, L"do_Riverbed" },
	{ 0x525B0650EDB3F907, L"do_SpaceNeedle" },
	{ 0x644ADB0FDB2C4659, L"do_Quarry" },
	{ 0x683A6788987BBD08, L"do_Fjord" },
	{ 0x769CA9D3DB3D4BC7, L"as_Bridge" },
	{ 0x831F7B7F952D5B74, L"as_Hillside" },
	{ 0x8F02D72ADA98376A, L"do_Xmas" },
	{ 0x9E09C0C0A8287490, L"do_Hometown" },
	{ 0xA034BA00003A26EF, L"do_Liberty" },
	{ 0xA26056C7BDAFA137, L"as_AirBase" },
	{ 0xA6125C4498FAAFE5, L"tw_Wasteland" },
	{ 0xA8F4AD3889B7DB44, L"tw_Highway" },
	{ 0xB192ED3AC88EC95E, L"as_Dome" },
	{ 0xB66333A0F3934C34, L"do_Farmland" },
	{ 0xB9A06118E38E7651, L"do_Powerplant" },
	{ 0xBE135EFD6055077E, L"do_Riviera" },
	{ 0xC5C22A5CBDA4CEA9, L"do_Island" },
	{ 0xC94327B399856BA4, L"do_Tequila" },
	{ 0xCE2385D40F5D1752, L"do_Mauer" },
	{ 0xD9B85CACC9FEA400, L"do_Silo" },
	{ 0xEC622A23B302DC9D, L"do_Seaside" },
	{ 0xEC7E463C053AE139, L"as_Typhoon" },
	{ 0xECD1F5047AD078A5, L"do_Ruins" },
};

bool MMG_ServerTracker::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);

	switch(aDelimiter)
	{
		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGREQUEST:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_MINIPINGREQUEST:");
			//responseDelimeter = MMG_ProtocolDelimiters::SERVERTRACKER_USER_MINIPINGRESPONSE;
		}
		break;
		*/

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:");

			uint64 cycleHash;
			aMessage->ReadUInt64(cycleHash);

			DebugLog(L_INFO, "hash: %llX", cycleHash);

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_RSP);
			responseMessage.WriteUInt64(cycleHash);
			responseMessage.WriteUShort(ARRAYSIZE(cycles));

			for (int i = 0; i < ARRAYSIZE(cycles); i++)
			{
				responseMessage.WriteUInt64(cycles[i].hash);
				responseMessage.WriteString(cycles[i].mapname);
			}

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_LIST_SERVERS:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS:");

			uint gameVersion;
			aMessage->ReadUInt(gameVersion);

			uint protocolVersion;
			aMessage->ReadUInt(protocolVersion);

			ushort activefilters;
			aMessage->ReadUShort(activefilters);

			// read server fiters from the client
			ServerListFilters filters;
			filters.ReadFilters(activefilters, aMessage);

			if(filters.HasFlag(DEDICATED_FLAG))
				printf("activefilters has DEDICATED_FLAG\n");

			filters.PrintFilters(); //debug purposes

			//global server count
			const int TOTAL_SERVERS = 1;

			//send total number of servers in global list
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NUM_TOTAL_SERVERS);
			responseMessage.WriteUInt(TOTAL_SERVERS);

			// list of matched servers retrieved from global list, currently disregarding filters
			// NOTE: all servers are sent to the client
			// TODO:	- create list of servers using filters (MMG_TrackableServerFullInfo)
			//			- build MMG_TrackableServerBriefInfo from MMG_TrackableServerFullInfo
			// temporary
			MMG_TrackableServerBriefInfo shortServerList[TOTAL_SERVERS];
			MMG_TrackableServerFullInfo fullServerList[TOTAL_SERVERS];


			auto sv = &MMG_TrackableServer::ourInstance->m_ServerList[0];

			shortServerList[0].m_CycleHash = 0;
			wcscpy_s(shortServerList[0].m_GameName, (const wchar_t *)sv->m_Info.m_ServerName);
			shortServerList[0].m_IP = 0x0100007F;//sv->m_Info.m_Ip;
			shortServerList[0].m_IsRankBalanced = sv->m_Info.m_IsRankBalanced;
			shortServerList[0].m_MassgateCommPort = sv->m_Info.m_MassgateCommPort;
			shortServerList[0].m_ModId = sv->m_Info.m_ModId;
			shortServerList[0].m_ServerId = sv->m_Info.m_ServerId;
			shortServerList[0].m_ServerType = sv->m_Info.m_ServerType;

			fullServerList[0].gapc4 = 0;
			fullServerList[0].m_CurrentLeader = sv->m_Heartbeat.m_CurrentLeader;
			fullServerList[0].m_CurrentMapHash = sv->m_Heartbeat.m_CurrentMapHash;
			fullServerList[0].m_CycleHash = 0;
			fullServerList[0].m_GameTime = sv->m_Heartbeat.m_GameTime;
			fullServerList[0].m_GameVersion = sv->m_Info.m_GameVersion;
			fullServerList[0].m_HostProfileId = sv->m_Info.m_HostProfileId;
			fullServerList[0].m_IP = 0x0100007F;//sv->m_Info.m_Ip;
			fullServerList[0].m_MassgateCommPort = sv->m_Info.m_MassgateCommPort;
			fullServerList[0].m_ModId = sv->m_Info.m_ModId;
			fullServerList[0].m_Ping = 51;
			fullServerList[0].m_ProtocolVersion = PROTO_1012;
			fullServerList[0].m_ServerId = sv->m_Info.m_ServerId;
			wcscpy_s(fullServerList[0].m_ServerName, (const wchar_t *)sv->m_Info.m_ServerName);
			fullServerList[0].m_ServerReliablePort = sv->m_Info.m_ServerReliablePort;
			fullServerList[0].m_ServerType = sv->m_Info.m_ServerType;
			fullServerList[0].m_WinnerTeam = 0;
			fullServerList[0]._bf198 = 0;

			//send matched server list
			for (int i = 0; i < TOTAL_SERVERS; i++)
			{
				//write short server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);
				shortServerList[i].ToStream(&responseMessage);
				
				if (!aClient->SendData(&responseMessage))
					return false;

				//write complete server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO);
				fullServerList[i].ToStream(&responseMessage);

				// Must be zero (protocol verification only)
				responseMessage.WriteUInt(0);

				if (!aClient->SendData(&responseMessage))
					return false;
			}

			//send "no more server info" delimiter
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVERS_BY_NAME:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_GET_SERVERS_BY_NAME:");
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:
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
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:");
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:");
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ: //sub_791940
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_LADDER_TOPTEN_REQ:");
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_LADDER_GET_REQ: //sub_791940
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_LADDER_GET_REQ:");
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_REQ: //sub_791710
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_STATS_REQ:");

			uint ProfileId = 0;
			aMessage->ReadUInt(ProfileId);
			
			MMG_PlayerStats myPlayerStats;
			myPlayerStats.m_PlayerId = ProfileId;
			myPlayerStats.m_LastMatchPlayed = 60;
			myPlayerStats.m_ScoreTotal = 100;
			myPlayerStats.m_ScoreAsInfantry = 10;
			myPlayerStats.m_HighScoreAsInfantry = 10;
			myPlayerStats.m_ScoreAsSupport = 30;
			myPlayerStats.m_HighScoreAsSupport = 30;
			myPlayerStats.m_ScoreAsArmor = 10;
			myPlayerStats.m_HighScoreAsArmor = 10;
			myPlayerStats.m_ScoreAsAir = 30;
			myPlayerStats.m_HighScoreAsAir = 30;
			myPlayerStats.m_ScoreByDamagingEnemies = 15;
			myPlayerStats.m_ScoreByUsingTacticalAid = 20;
			myPlayerStats.m_ScoreByCapturingCommandPoints = 25;
			myPlayerStats.m_ScoreByRepairing = 30;
			myPlayerStats.m_ScoreByFortifying = 10;
			myPlayerStats.m_HighestScore = 10;
			myPlayerStats.m_CurrentLadderPosition = 7;
			myPlayerStats.m_TimeTotalMatchLength = 3003000; // shit ain't working
			myPlayerStats.m_TimePlayedAsUSA = 320;
			myPlayerStats.m_TimePlayedAsUSSR = 340;
			myPlayerStats.m_TimePlayedAsNATO = 360;
			myPlayerStats.m_TimePlayedAsInfantry = 400;
			myPlayerStats.m_TimePlayedAsSupport = 420;
			myPlayerStats.m_TimePlayedAsArmor = 440;
			myPlayerStats.m_TimePlayedAsAir = 460;
			myPlayerStats.m_NumberOfMatches = 10;
			myPlayerStats.m_NumberOfMatchesWon = 49;
			myPlayerStats.m_NumberOfMatchesLost = 51;
			myPlayerStats.m_CurrentWinningStreak = 2;
			myPlayerStats.m_BestWinningStreak = 3;
			myPlayerStats.m_NumberOfAssaultMatches = 20;	// not listed
			myPlayerStats.m_NumberOfAssaultMatchesWon = 11;
			myPlayerStats.m_NumberOfDominationMatches = 30;	// not listed
			myPlayerStats.m_NumberOfDominationMatchesWon = 21;
			myPlayerStats.m_NumberOfTugOfWarMatches = 40;	// not listed
			myPlayerStats.m_NumberOfTugOfWarMatchesWon = 31;
			myPlayerStats.m_NumberOfMatchesWonByTotalDomination = 19;
			myPlayerStats.m_NumberOfPerfectDefensesInAssaultMatch = 9;
			myPlayerStats.m_NumberOfPerfectPushesInTugOfWarMatch = 29;
			myPlayerStats.m_NumberOfUnitsKilled = 4000;
			myPlayerStats.m_NumberOfUnitsLost = 4100;
			myPlayerStats.m_NumberOfReinforcementPointsSpent = 400000;
			myPlayerStats.m_NumberOfTacticalAidPointsSpent = 300000;
			myPlayerStats.m_NumberOfNukesDeployed = 23;
			myPlayerStats.m_NumberOfTacticalAidCriticalHits = 30000;

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_RSP);
			myPlayerStats.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_REQ: //sub_791650
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_MEDALS_REQ:");

			uint ProfileId = 0;
			aMessage->ReadUInt(ProfileId);
			
			//responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_RSP);
			
			//if (!aClient->SendData(&responseMessage))
			//	return false;
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_REQ: //sub_7915F0
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_BADGES_REQ:");

			// this case currently disconnects the client

			uint ProfileId = 0, BadgesStreamLength = 1;
			aMessage->ReadUInt(ProfileId);
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_RSP);
			responseMessage.WriteUInt(ProfileId);
			responseMessage.WriteUInt(0);
			//responseMessage.WriteRawData(dataStream, dataLength);
			
			//MN_WriteMessage bufferMessage(1024);
			//sizeptr_t dataLength = bufferMessage.GetDataLength();
			//voidptr_t dataStream = bufferMessage.GetDataStream();

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		*/
		
		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_REQ: //sub_7916B0
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_STATS_REQ:");

			uint ClanProfileId = 0;
			aMessage->ReadUInt(ClanProfileId);
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_RSP);
			
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		*/

		/*case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_REQ: //sub_791590
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_MEDALS_REQ:");

			uint ClanProfileId = 0;
			aMessage->ReadUInt(ClanProfileId);
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_RSP);
			
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		*/

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}