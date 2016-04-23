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
			const int TOTAL_SERVERS = 2;

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

			//send matched server list
			for (int i = 0; i < TOTAL_SERVERS; i++)
			{
				//write short server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);
				shortServerList[i].ToStream(&responseMessage);
				
				//write complete server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO);
				fullServerList[i].ToStream(&responseMessage);
			
				//success flag?
				responseMessage.WriteUInt(1);
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