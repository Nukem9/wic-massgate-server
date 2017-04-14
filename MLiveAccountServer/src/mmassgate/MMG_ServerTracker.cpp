#include "../stdafx.h"

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
	{ 0x1CCD30B72CBB801B, L"do_Countryside" },
	{ 0x366789C15AE10C5E, L"tw_Radar" },
	{ 0x36F05A2AA0295C85, L"do_Canal" },
	{ 0x429233342E470501, L"do_Studio" },
	{ 0x48C9F7176567AE50, L"as_Ozzault" },
	{ 0x4C00F431FBEAE817, L"do_Riverbed" },
	{ 0x525B0650EDB3F907, L"do_SpaceNeedle" },
	{ 0x644ADB0FDB2C4659, L"do_Quarry" },
	{ 0x683A6788987BBD08, L"do_Fjord" },
	{ 0x769CA9D3DB3D4BC7, L"as_Bridge" },
	{ 0x831F7B7F952D5B74, L"as_Hillside" },
	{ 0x8F02D72ADA98376A, L"do_Xmas" },
	{ 0x9B3224F6ECAC28E3, L"do_Airport" },
	{ 0x9E09C0C0A8287490, L"do_Hometown" },
	{ 0xA034BA00003A26EF, L"do_Liberty" },
	{ 0xA26056C7BDAFA137, L"as_AirBase" },
	{ 0xA6125C4498FAAFE5, L"tw_Wasteland" },
	{ 0xA8F4AD3889B7DB44, L"tw_Highway" },
	{ 0xB192ED3AC88EC95E, L"as_Dome" },
	{ 0xB4587B3812BA036C, L"tw_Bocage" },
	{ 0xB51B002E5A355FF1, L"tw_Arizona" },
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
	{ 0xF00E7E6DD9850EF3, L"do_Virginia" },
	{ 0xF6889983A6B4D4E1, L"do_Valley" },
	{ 0xFAA7FB8CAD40E3A2, L"do_Paradise" },
	{ 0xFE456DAC7CED58F9, L"do_Vineyard" },
};

bool MMG_ServerTracker::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(4096);

	switch(aDelimiter)
	{
		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_LIST_SERVERS:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_LIST_SERVERS:");

			// read server filters from the client
			MMG_ServerFilter filters;
			if (!filters.FromStream(aMessage))
				return false;

			// list of matched servers retrieved from global list, currently disregarding filters
			uint serverCount, matchedCount;
			std::vector<MMG_TrackableServerFullInfo> serverFullInfo;
			std::vector<MMG_TrackableServerBriefInfo> serverBriefInfo;

			MMG_TrackableServer::ourInstance->GetServerListInfo(&filters, &serverFullInfo, &serverBriefInfo, &serverCount);
			matchedCount = serverFullInfo.size();

			//send total number of servers in global list
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NUM_TOTAL_SERVERS);
			responseMessage.WriteUInt(serverCount);

			//send matched server list
			for (uint i = 0; i < matchedCount; i++)
			{
				//write short server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);
				serverBriefInfo[i].ToStream(&responseMessage);
				
				//write complete server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO);
				serverFullInfo[i].ToStream(&responseMessage);

				// Must be zero (protocol verification only)
				responseMessage.WriteUInt(0);
			}

			//send "no more server info" delimiter
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_GET_SERVER_BY_ID:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_GET_SERVER_BY_ID:");
			uint playerOnlineStatus, gameVersion, protocolVersion;
			if (!aMessage->ReadUInt(playerOnlineStatus) || !aMessage->ReadUInt(gameVersion) || !aMessage->ReadUInt(protocolVersion))
				return false;

			MMG_ServerFilter filters;
			filters.gameVersion = gameVersion;
			filters.protocolVersion = protocolVersion;
			
			uint serverCount, matchedCount;
			std::vector<MMG_TrackableServerFullInfo> serverFullInfo;
			std::vector<MMG_TrackableServerBriefInfo> serverBriefInfo;

			MMG_TrackableServer::ourInstance->GetServerListInfo(&filters, &serverFullInfo, &serverBriefInfo, &serverCount);
			matchedCount = serverFullInfo.size();

			for (uint i = 0; i < matchedCount; i++)
			{
				if (serverFullInfo[i].m_ServerId == playerOnlineStatus)
				{
					//write short server info to stream
					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);
					serverBriefInfo[i].ToStream(&responseMessage);
				
					//write complete server info to stream
					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO);
					serverFullInfo[i].ToStream(&responseMessage);

					// Must be zero (protocol verification only)
					responseMessage.WriteUInt(0);

					break;
				}
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_NO_MORE_SERVER_INFO);
			
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CYCLE_MAP_LIST_REQ:");

			uint64 cycleHash;
			if (!aMessage->ReadUInt64(cycleHash))
				return false;

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

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_LADDER_GET_REQ:");

			uint startPos = 0;
			uint profileId = 0;
			uint numItems = 0;
			uint requestId = 0;
			
			if (!aMessage->ReadUInt(startPos)
				|| !aMessage->ReadUInt(profileId)
				|| !aMessage->ReadUInt(numItems)
				|| !aMessage->ReadUInt(requestId))
				return false;

			MMG_LadderProtocol::LadderRsp myResponse;
			myResponse.requestId = requestId;
			uint foundItems = 0;
			uint requestPos = 0;

			if (startPos == 0)
			{
				uint thePosition = 0;
				
				MySQLDatabase::ourInstance->QueryProfileLadderPosition(profileId, &thePosition);

				if (numItems == 1)
					// open profile page
					requestPos = thePosition;
				else
					// show leaderboard/my position/jumpto position
					requestPos = 1 + ((thePosition / 100) * 100);
			}
			else 
			{
				// top 100/next/prev page
				requestPos = 1 + ((startPos / 100) * 100);
			}

			MySQLDatabase::ourInstance->QueryPlayerLadder(requestPos, numItems, &foundItems, &myResponse);

			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP:");
			// write case 1
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
			responseMessage.WriteUChar(1);

			responseMessage.WriteUInt(myResponse.startPos);
			responseMessage.WriteUInt(myResponse.requestId);
			responseMessage.WriteUInt(myResponse.ladderSize);
			responseMessage.WriteUInt(foundItems);

			for (uint i = 0; i < foundItems; i++)
				responseMessage.WriteUInt(myResponse.ladderItems[i].score);
			
			// write case 2
			// sometimes a packet can be bigger than 4096
			// todo: this could be better but its ok for now
			uint chunkSize = 50; // profile count, not packet size
			uint itemsLeft = foundItems;

			while (itemsLeft > chunkSize)
			{
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
				responseMessage.WriteUChar(2);

				responseMessage.WriteUInt(chunkSize);

				for (uint i = (foundItems - itemsLeft); i < (foundItems - itemsLeft) + chunkSize; i++)
					myResponse.ladderItems[i].profile.ToStream(&responseMessage);

				if (!aClient->SendData(&responseMessage))
					return false;

				itemsLeft -= chunkSize;
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
			responseMessage.WriteUChar(2);

			responseMessage.WriteUInt(itemsLeft);

			for (uint i = (foundItems - itemsLeft); i < foundItems; i++)
				myResponse.ladderItems[i].profile.ToStream(&responseMessage);

			// write case 3
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
			responseMessage.WriteUChar(3);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_FRIENDS_LADDER_GET_REQ:");

			uint requestId = 0;
			uint profileCount = 0;

			uint profileIds[512];
			memset(profileIds, 0, sizeof(profileIds));

			// always requests at least one profileid, the first being the current logged in profile
			// also includes clan members

			if (!aMessage->ReadUInt(requestId) || !aMessage->ReadUInt(profileCount))
				return false;

			for (uint i = 0; i < profileCount; i++)
			{
				if (!aMessage->ReadUInt(profileIds[i]))
					return false;
			}

			LadderEntry friendsLadder[512];
			memset(friendsLadder, 0, sizeof(friendsLadder));

			uint currentProfile = 0;
			uint friendCount = 0;

			do
			{
				MMG_LadderProtocol::LadderRsp ladder;
				uint foundItems = 0;

				if (!MySQLDatabase::ourInstance->QueryPlayerLadder(profileIds[currentProfile], &foundItems, &ladder))
					return false;

				if (foundItems > 0)
				{
					friendsLadder[friendCount].m_ProfileId = ladder.ladderItems[0].profile.m_ProfileId;
					friendsLadder[friendCount].m_Score = ladder.ladderItems[0].score;
					friendCount++;
				}

				currentProfile++;
			} while (currentProfile < profileCount);

			DebugLog(L_INFO, "SERVERTRACKER_USER_FRIENDS_LADDER_GET_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_FRIENDS_LADDER_GET_RSP);
			responseMessage.WriteUInt(requestId);
			responseMessage.WriteUInt(friendCount);

			for (uint i = 0; i < friendCount; i++)
			{
				responseMessage.WriteUInt(friendsLadder[i].m_ProfileId);
				responseMessage.WriteUInt(friendsLadder[i].m_Score);
			}

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_STATS_REQ:");

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;
			
			MMG_Stats::PlayerStatsRsp playerStats;
			MySQLDatabase::ourInstance->QueryProfileStats(profileId, &playerStats);
			MySQLDatabase::ourInstance->QueryProfileLadderPosition(profileId, &playerStats.m_CurrentLadderPosition);

			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_STATS_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_RSP);
			playerStats.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_MEDALS_REQ:");

			uint profileId = 0;
			if (!aMessage->ReadUInt(profileId))
				return false;

			uint buffer[256];
			memset(buffer, 0, sizeof(buffer));

			MySQLDatabase::ourInstance->QueryProfileMedalsRawData(profileId, buffer, sizeof(buffer));

			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_MEDALS_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_MEDALS_RSP);
			responseMessage.WriteUInt(profileId);
			responseMessage.WriteUInt(19);
			responseMessage.WriteRawData(buffer, sizeof(buffer));

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_BADGES_REQ:");

			uint profileId = 0;
			if (!aMessage->ReadUInt(profileId))
				return false;

			uint buffer[256];
			memset(buffer, 0, sizeof(buffer));

			MySQLDatabase::ourInstance->QueryProfileBadgesRawData(profileId, buffer, sizeof(buffer));

			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_BADGES_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_RSP);
			responseMessage.WriteUInt(profileId);
			responseMessage.WriteUInt(14);
			responseMessage.WriteRawData(buffer, sizeof(buffer));

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		
		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_STATS_REQ:");

			uint clanId;
			if (!aMessage->ReadUInt(clanId))
				return false;
			
			MMG_Stats::ClanStatsRsp clanStats;
			clanStats.m_ClanId = clanId;

			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_STATS_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_STATS_RSP);
			clanStats.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_REQ:
		{
			DebugLog(L_INFO, "SERVERTRACKER_USER_CLAN_MEDALS_REQ:");

			uint clanProfileId;
			if (!aMessage->ReadUInt(clanProfileId))
				return false;
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_CLAN_MEDALS_RSP);
			responseMessage.WriteUInt(clanProfileId);	// ClanId
			responseMessage.WriteUInt(8);				// Medal count (MUST be 8)

			uint buffer[256];
			MMG_BitWriter<unsigned int> writer(buffer, sizeof(buffer) * 8);

			for (int i = 0; i < 8; i++)
			{
				writer.WriteBits(3, 2);// Medal type (0 to 3)
				writer.WriteBits(0, 2);// Medal stars (0 to 3)
			}

			responseMessage.WriteRawData(buffer, sizeof(buffer));

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}