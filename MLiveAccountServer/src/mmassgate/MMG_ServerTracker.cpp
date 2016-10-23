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
	MN_WriteMessage	responseMessage(2048);

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
			
			uint serverCount;
			std::vector<MMG_TrackableServerFullInfo> serverFullInfo;
			std::vector<MMG_TrackableServerBriefInfo> serverBriefInfo;
			MMG_ServerFilter filters;

			// first get the full lst
			MMG_TrackableServer::ourInstance->GetServerListInfo(&filters, &serverFullInfo, &serverBriefInfo, &serverCount);

			// search for the server by server id
			std::vector<MMG_TrackableServerFullInfo>::iterator full_iter;
			std::vector<MMG_TrackableServerBriefInfo>::iterator brief_iter;

			full_iter = serverFullInfo.begin();
			brief_iter = serverBriefInfo.begin();

			// TODO protocolVersion
			while (full_iter != serverFullInfo.end())
			{
				if (full_iter->m_ServerId == playerOnlineStatus && full_iter->m_GameVersion == gameVersion)
					break;

				++full_iter;
				++brief_iter;
			}
			
			if (full_iter != serverFullInfo.end())
			{
				//write short server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_SHORT_SERVER_INFO);
				brief_iter->ToStream(&responseMessage);
				
				//write complete server info to stream
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_COMPLETE_SERVER_INFO);
				full_iter->ToStream(&responseMessage);

				// Must be zero (protocol verification only)
				responseMessage.WriteUInt(0);
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

			uint startPos, numItems;
			uint profileId, requestId;
			uint foundItems;

			MMG_LadderProtocol::LadderRsp myResponse;

			if (!aMessage->ReadUInt(startPos)
				|| !aMessage->ReadUInt(profileId)
				|| !aMessage->ReadUInt(numItems)
				|| !aMessage->ReadUInt(requestId))
				return false;

			//printf("%d %d %d %d \n", startPos, profileId, numItems, requestId);

			foundItems = 0;

			// write case 1
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
			responseMessage.WriteUChar(1);

			responseMessage.WriteUInt(0);			//startpos
			responseMessage.WriteUInt(requestId);	//requestid
			responseMessage.WriteUInt(0);			//ladder total size
			responseMessage.WriteUInt(0);			//ladder page size

			for (uint i = 0; i < foundItems; i++)
				responseMessage.WriteUInt(myResponse.ladderItems[i].score);

			//write case 2
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
			responseMessage.WriteUChar(2);

			responseMessage.WriteUInt(foundItems);	//ladder page size

			for (uint i = 0; i < foundItems; i++)
				myResponse.ladderItems[i].profile.ToStream(&responseMessage);

			//write case 3
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_LADDER_GET_RSP);
			responseMessage.WriteUChar(3);

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
			playerStats.m_ProfileId = profileId;

			DebugLog(L_INFO, "SERVERTRACKER_USER_PLAYER_STATS_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_STATS_RSP);
			playerStats.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

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
			
			//responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::SERVERTRACKER_USER_PLAYER_BADGES_RSP);
			//responseMessage.WriteUInt(ProfileId);
			//responseMessage.WriteUInt(0);
			//responseMessage.WriteRawData(dataStream, dataLength);
			
			//MN_WriteMessage bufferMessage(1024);
			//sizeptr_t dataLength = bufferMessage.GetDataLength();
			//voidptr_t dataStream = bufferMessage.GetDataStream();

			//if (!aClient->SendData(&responseMessage))
			//	return false;
		}
		break;
		*/
		
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