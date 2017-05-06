#include "../stdafx.h"

void MMG_Messaging::IM_Settings::ToStream(MN_WriteMessage *aMessage)
{
	aMessage->WriteUChar(this->m_Friends);
	aMessage->WriteUChar(this->m_Clanmembers);
	aMessage->WriteUChar(this->m_Acquaintances);
	aMessage->WriteUChar(this->m_Anyone);
}

bool MMG_Messaging::IM_Settings::FromStream(MN_ReadMessage *aMessage)
{
	if (!aMessage->ReadUChar(this->m_Friends) || !aMessage->ReadUChar(this->m_Clanmembers))
		return false;

	if (!aMessage->ReadUChar(this->m_Acquaintances) || !aMessage->ReadUChar(this->m_Anyone))
		return false;

	return true;
}

void MMG_Messaging::ProfileStateObserver::update(MC_Subject *subject, StateType type)
{
	MN_WriteMessage	responseMessage(2048);
	MMG_Profile *profile = (MMG_Profile*)subject;

#ifndef USING_MYSQL_DATABASE
	responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	profile->ToStream(&responseMessage);
#else
	if (!MySQLDatabase::ourInstance->TestDatabase())
	{
		MySQLDatabase::ourInstance->EmergencyMassgateDisconnect();
		return;
	}

	MySQLDatabase::ourInstance->BeginTransaction();

	switch (type)
	{
		case ClanId:
		{
			if (!MySQLDatabase::ourInstance->UpdateProfileClanId(profile->m_ProfileId, profile->m_ClanId)
				|| !MySQLDatabase::ourInstance->UpdateProfileClanRank(profile->m_ProfileId, profile->m_RankInClan))
			{

				MySQLDatabase::ourInstance->RollbackTransaction();
				return;
			}

			MMG_Profile nameBuffer;

			if (!MySQLDatabase::ourInstance->QueryProfileName(profile->m_ProfileId, &nameBuffer))
			{
				MySQLDatabase::ourInstance->RollbackTransaction();
				return;
			}

			memset(profile->m_Name, 0, sizeof(profile->m_Name));
			wcsncpy(profile->m_Name, nameBuffer.m_Name, WIC_NAME_MAX_LENGTH);

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PLAYER_JOINED_CLAN);
			profile->ToStream(&responseMessage);
		}
		break;

		case OnlineStatus:
		{
			if (!MySQLDatabase::ourInstance->UpdateProfileOnlineStatus(profile->m_ProfileId, profile->m_OnlineStatus))
			{
				MySQLDatabase::ourInstance->RollbackTransaction();
				return;
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile->ToStream(&responseMessage);
		}
		break;

		case Rank:
		{
			if (!MySQLDatabase::ourInstance->UpdateProfileRank(profile->m_ProfileId, profile->m_Rank))
			{
				MySQLDatabase::ourInstance->RollbackTransaction();
				return;
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile->ToStream(&responseMessage);
		}
		break;

		case RankInClan:
		{
			if (!MySQLDatabase::ourInstance->UpdateProfileClanRank(profile->m_ProfileId, profile->m_RankInClan))
			{
				MySQLDatabase::ourInstance->RollbackTransaction();
				return;
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
			profile->ToStream(&responseMessage);
		}
		break;

		default:
		break;
	}

	MySQLDatabase::ourInstance->CommitTransaction();
#endif
	SvClientManager::ourInstance->SendMessageToOnlinePlayers(&responseMessage);
}

MMG_Messaging::MMG_Messaging()
{
}

bool MMG_Messaging::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	switch(aDelimiter)
	{
		case MMG_ProtocolDelimiters::MESSAGING_RETRIEVE_PROFILENAME:
		{
			DebugLog(L_INFO, "MESSAGING_RETRIEVE_PROFILENAME:");

			MN_WriteMessage	responseMessage(4096);

			ushort profileCount;
			if (!aMessage->ReadUShort(profileCount))
				return false;

#ifndef USING_MYSQL_DATABASE

			//handle profiles (count).
			for (ushort i = 0; i < profileCount; i++)
			{
				uint profileId = 0;
				MMG_Profile myFriend;

				if (!aMessage->ReadUInt(profileId))
					return false;

				myFriend.m_ProfileId = profileId;

				if (profileId == 1235)
				{
					wcscpy_s(myFriend.m_Name, L"tenerefis");
					myFriend.m_OnlineStatus = 1;
					myFriend.m_Rank = 18;
				}
				else if (profileId == 1236)
				{
					wcscpy_s(myFriend.m_Name, L"HouseBee");
					myFriend.m_Rank = 18;
				}
				else
				{
					swprintf(myFriend.m_Name, L"Profile_%u", profileId);
				}

				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
				myFriend.ToStream(&responseMessage);

				// if current size will exceed max packet size when adding the next profile, send the packet then keep going
				if (responseMessage.GetDataLength() + 66 > 4096)
				{
					if (!aClient->SendData(&responseMessage))
						return false;
				}
			}

			if (responseMessage.GetDataLength() > 2 && !aClient->SendData(&responseMessage))
				return false;
#else
			uint profileIds[128];
			memset(profileIds, 0, sizeof(profileIds));

			MMG_Profile profileList[128];

			for (ushort i = 0; i < profileCount; i++)
			{
				if (!aMessage->ReadUInt(profileIds[i]))
					return false;
			}

			MySQLDatabase::ourInstance->QueryProfileList(profileCount, profileIds, profileList);

			DebugLog(L_INFO, "MESSAGING_RESPOND_PROFILENAME: sending %d profile name/s", profileCount);

			uint chunkSize = 64; // profile count, not packet size
			uint itemsLeft = profileCount;

			while (itemsLeft > chunkSize)
			{
				for (uint i = (profileCount - itemsLeft); i < (profileCount - itemsLeft) + chunkSize; i++)
				{
					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
					profileList[i].ToStream(&responseMessage);
				}

				if (!aClient->SendData(&responseMessage))
					return false;

				itemsLeft -= chunkSize;
			}

			for (uint i = (profileCount - itemsLeft); i < profileCount; i++)
			{
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
				profileList[i].ToStream(&responseMessage);
			}

			if (!aClient->SendData(&responseMessage))
				return false;
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST:");

			MN_WriteMessage	responseMessage(4096);

			/*MMG_Profile *myProfile = aClient->GetProfile();

			this->SendProfileName(aClient, &responseMessage, myProfile);
			this->SendPingsPerSecond(aClient, &responseMessage);

			if (myProfile->m_ClanId)
			{
				MMG_ProtocolDelimiters::Delimiter delim;
				if (!aMessage->ReadDelimiter((ushort &)delim))
					return false;

				uint clanId;
				uint unkZero;
				if (!aMessage->ReadUInt(clanId) || !aMessage->ReadUInt(unkZero))
					return false;

				if (myProfile->m_ClanId != clanId || unkZero != 0)
					return false;

				// TODO: Write clan information
			}

			this->SendCommOptions(aClient, &responseMessage);
			this->SendStartupSequenceComplete(aClient, &responseMessage);
			*/

#ifndef USING_MYSQL_DATABASE
			DebugLog(L_INFO, "MESSAGING_GET_ACQUAINTANCES_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_ACQUAINTANCES_RESPONSE);

			//write uint (num acquaintances)
			responseMessage.WriteUInt(150);

			for (int i = 0; i < 150; i++)
			{
				responseMessage.WriteUInt(1237+i);
				responseMessage.WriteUInt(5);
			}

			//for each acquaintance
				//write uint (profile id)
				//write uint number times played

			DebugLog(L_INFO, "MESSAGING_GET_FRIENDS_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_RESPONSE);

			//write uint (num friends)
			responseMessage.WriteUInt(2);
			responseMessage.WriteUInt(1235);
			responseMessage.WriteUInt(1236);

			//for each friend
				//write uint (profile id)
#else
			//send acquaintances first, otherwise it screws up the contacts list
			DebugLog(L_INFO, "MESSAGING_GET_ACQUAINTANCES_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_ACQUAINTANCES_RESPONSE);

			uint totalCount = 0;
			Acquaintance acquaintances[512];
			memset(acquaintances, 0, sizeof(acquaintances));

			if (!MySQLDatabase::ourInstance->QueryAcquaintances(aClient->GetProfile()->m_ProfileId, &totalCount, acquaintances))
				responseMessage.WriteUInt(0);
			else
			{
				responseMessage.WriteUInt(totalCount);

				for (uint i=0; i < totalCount; i++)
				{
					responseMessage.WriteUInt(acquaintances[i].m_ProfileId);
					responseMessage.WriteUInt(acquaintances[i].m_NumTimesPlayed);
				}
			}

			// send friends
			DebugLog(L_INFO, "MESSAGING_GET_FRIENDS_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_RESPONSE);

			uint friendCount = 0;
			uint myFriends[100];
			memset(myFriends, 0, sizeof(myFriends));

			if (!MySQLDatabase::ourInstance->QueryFriends(aClient->GetProfile()->m_ProfileId, &friendCount, myFriends))
				responseMessage.WriteUInt(0);
			else
			{
				responseMessage.WriteUInt(friendCount);

				for (uint i=0; i < friendCount; i++)
					responseMessage.WriteUInt(myFriends[i]);
			}
#endif
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_ADD_FRIEND_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_ADD_FRIEND_REQUEST:");

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;

#ifdef USING_MYSQL_DATABASE
			MySQLDatabase::ourInstance->AddFriend(aClient->GetProfile()->m_ProfileId, profileId);
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_REMOVE_FRIEND_REQUEST:");

			MN_WriteMessage	responseMessage(2048);

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;

#ifdef USING_MYSQL_DATABASE
			MySQLDatabase::ourInstance->RemoveFriend(aClient->GetProfile()->m_ProfileId, profileId);
#endif

			DebugLog(L_INFO, "MESSAGING_REMOVE_FRIEND_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_RESPONSE);
			responseMessage.WriteUInt(profileId);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_IM_CHECK_PENDING_MESSAGES:
		{
			DebugLog(L_INFO, "MESSAGING_IM_CHECK_PENDING_MESSAGES:");

			MN_WriteMessage	responseMessage(4096);

			uint msgCount = 0;
			MMG_InstantMessageListener::InstantMessage myMsgs[20];

#ifdef USING_MYSQL_DATABASE
			MySQLDatabase::ourInstance->QueryPendingMessages(aClient->GetProfile()->m_ProfileId, &msgCount, myMsgs);
#endif

			// if there are messages, send them
			// pending messages will be removed once they have been acknowledged.
			if (msgCount > 0)
			{
				for (uint i = 0; i < msgCount; i++)
				{
					//DebugLog(L_INFO, "MESSAGING_IM_RECEIVE");
					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
					myMsgs[i].ToStream(&responseMessage);

					if (responseMessage.GetDataLength() + sizeof(MMG_InstantMessageListener::InstantMessage) >= 4096)
					{
						if (!aClient->SendData(&responseMessage))
							break;
					}
				}

				if (responseMessage.GetDataLength() > 0 && !aClient->SendData(&responseMessage))
					return false;
			}
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_IM_SEND:
		{
			DebugLog(L_INFO, "MESSAGING_IM_SEND");
			
			MMG_InstantMessageListener::InstantMessage myInstantMessage;
			if(!myInstantMessage.FromStream(aMessage))
				return false;
			
			//handle padding
			uint padZero;
			if (!aMessage->ReadUInt(padZero))
				return false;

			//check to see if recipient is online
			SvClient *recipient = SvClientManager::ourInstance->FindPlayerByProfileId(myInstantMessage.m_RecipientProfile);

			if (!recipient)
			{
				// intended recipient is offline
#ifdef USING_MYSQL_DATABASE
				MySQLDatabase::ourInstance->AddInstantMessage(aClient->GetProfile()->m_ProfileId, &myInstantMessage);
#endif
			}
			else
			{
				MN_WriteMessage	responseMessage(2048);

				// if recipient does not ack, the message is lost, un-read messages are NOT saved client side
#ifdef USING_MYSQL_DATABASE
				MySQLDatabase::ourInstance->AddInstantMessage(aClient->GetProfile()->m_ProfileId, &myInstantMessage);
#endif
				//DebugLog(L_INFO, "MESSAGING_IM_RECEIVE");
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
				myInstantMessage.ToStream(&responseMessage);

				if (!recipient->SendData(&responseMessage))
					return false;
			}
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_IM_ACK:
		{
			DebugLog(L_INFO, "MESSAGING_IM_ACK:");

			uint messageId;
			if (!aMessage->ReadUInt(messageId))
				return false;

#ifdef USING_MYSQL_DATABASE
			MySQLDatabase::ourInstance->RemoveInstantMessage(aClient->GetProfile()->m_ProfileId, messageId);
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_SET_COMMUNICATION_OPTIONS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_SET_COMMUNICATION_OPTIONS_REQ:");

			uint commOptions;
			if (!aMessage->ReadUInt(commOptions))
				return false;

			// TODO
			//MMG_Options myOptions;
			//myOptions.FromUInt(commOptions);

			aClient->GetOptions()->FromUInt(commOptions);

#ifdef USING_MYSQL_DATABASE
			MySQLDatabase::ourInstance->SaveUserOptions(aClient->GetProfile()->m_ProfileId, commOptions);
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_GET_COMMUNICATION_OPTIONS_REQ:");

			MN_WriteMessage	responseMessage(2048);

#ifndef USING_MYSQL_DATABASE
			aClient->GetOptions()->FromUInt(992); // i dont remember the default values
#else
			uint commOptions;
			MySQLDatabase::ourInstance->QueryUserOptions(aClient->GetProfile()->m_ProfileId, &commOptions);

			aClient->GetOptions()->FromUInt(commOptions);
#endif
			if (!this->SendCommOptions(aClient, &responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS:
		{
			DebugLog(L_INFO, "MESSAGING_GET_IM_SETTINGS:");

			MN_WriteMessage	responseMessage(2048);

#ifndef USING_MYSQL_DATABASE
			aClient->GetOptions()->FromUInt(992); // i dont remember the default values
#else
			uint commOptions;
			MySQLDatabase::ourInstance->QueryUserOptions(aClient->GetProfile()->m_ProfileId, &commOptions);

			aClient->GetOptions()->FromUInt(commOptions);
#endif
			if (!this->SendIMSettings(aClient, &responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_GET_PPS_SETTINGS_REQ:");

			MN_WriteMessage	responseMessage(2048);

			this->SendPingsPerSecond(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_CREATE_REQUEST:");

			MN_WriteMessage	responseMessage(2048);

			wchar_t clanName[WIC_CLANNAME_MAX_LENGTH];
			wchar_t clanTag[WIC_CLANTAG_MAX_LENGTH];
			char displayTag[8];
			uint padZero;
			memset(clanName, 0, sizeof(clanName));
			memset(clanTag, 0, sizeof(clanTag));
			memset(displayTag, 0, sizeof(displayTag));
			
			if (!aMessage->ReadString(clanName, ARRAYSIZE(clanName)))
				return false;

			if (!aMessage->ReadString(clanTag, ARRAYSIZE(clanTag)))
				return false;

			if (!aMessage->ReadString(displayTag, ARRAYSIZE(displayTag)))
				return false;

			if (!aMessage->ReadUInt(padZero))
				return false;

#ifndef USING_MYSQL_DATABASE
			DebugLog(L_INFO, "MESSAGING_CLAN_CREATE_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_RESPONSE);
			responseMessage.WriteUChar(myClanStrings::InternalMassgateError);
			responseMessage.WriteUInt(0);

			if (!aClient->SendData(&responseMessage))
				return false;
#else
			uchar myStatusCode;
			uint myClanId;

			uint nameId = 0, tagId = 0;

			bool ClanNameQueryOK = MySQLDatabase::ourInstance->CheckIfClanNameExists(clanName, &nameId);
			bool ClanTagQueryOK = MySQLDatabase::ourInstance->CheckIfClanTagExists(clanTag, &tagId);
			
			if (nameId > 0 && ClanNameQueryOK)
			{
				myStatusCode = myClanStrings::FAIL_INVALID_NAME;
				myClanId = 0;
			}
			else if (tagId > 0 && ClanTagQueryOK)
			{
				myStatusCode = myClanStrings::FAIL_TAG_TAKEN;
				myClanId = 0;
			}
			//else if(wcsncmp(clanName, clanTag, WIC_CLANTAG_MAX_LENGTH) == 0)
			//{
			//	myStatusCode = myClanStrings::FAIL_OTHER;
			//	myClanId = 0;
			//}
			//else if (checkvalidchars)
			//{
			//	myStatusCode = myClanStrings::FAIL_OTHER;
			//	myClanId = 0;
			//}
			else if (!ClanNameQueryOK || !ClanTagQueryOK)
			{
				myStatusCode = myClanStrings::InternalMassgateError;
				myClanId = 0;
			}
			else
			{
				uint newClanId=0;
				bool CreateClanQueryOk = MySQLDatabase::ourInstance->CreateClan(aClient->GetProfile()->m_ProfileId, clanName, clanTag, displayTag, &newClanId);

				if (CreateClanQueryOk)
				{
					myStatusCode = 1;
					myClanId = newClanId;

					aClient->GetProfile()->setClanId(newClanId, 1);
				}
				else
				{
					myStatusCode = myClanStrings::InternalMassgateError;
					myClanId = 0;
				}
			}

			DebugLog(L_INFO, "MESSAGING_CLAN_CREATE_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_RESPONSE);
			responseMessage.WriteUChar(myStatusCode);
			responseMessage.WriteUInt(myClanId);

			if (!aClient->SendData(&responseMessage))
				return false;
#endif
		}
		break;

#ifdef USING_MYSQL_DATABASE

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_MODIFY_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_MODIFY_REQUEST:");

			wchar_t ClanMotto[WIC_MOTTO_MAX_LENGTH];
			wchar_t ClanMessageoftheday[WIC_MOTD_MAX_LENGTH];
			wchar_t ClanHomepage[WIC_HOMEPAGE_MAX_LENGTH];
			memset(ClanMotto, 0, sizeof(ClanMotto));
			memset(ClanMessageoftheday, 0, sizeof(ClanMessageoftheday));
			memset(ClanHomepage, 0, sizeof(ClanHomepage));
			
			if (!aMessage->ReadString(ClanMotto, ARRAYSIZE(ClanMotto)))
				return false;

			if (!aMessage->ReadString(ClanMessageoftheday, ARRAYSIZE(ClanMessageoftheday)))
				return false;

			if (!aMessage->ReadString(ClanHomepage, ARRAYSIZE(ClanHomepage)))
				return false;

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;

			MySQLDatabase::ourInstance->SaveClanEditableVariables(aClient->GetProfile()->m_ClanId, profileId, ClanMotto, ClanMessageoftheday, ClanHomepage);
			// no response, rest of packet at this point is another delimiter (sub_7A0FE0, MESSAGING_CLAN_FULL_INFO_REQUEST)
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_CLAN_MODIFY_RANKS_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_MODIFY_RANKS_REQUEST:");

			MN_WriteMessage	responseMessage(2048);

			uint profileId, option;
			if (!aMessage->ReadUInt(profileId) || !aMessage->ReadUInt(option))
				return false;

			uint memberCount = 0;
			MMG_Clan::FullInfo myClan;
			ProfileStateObserver tempObserver;
			MMG_Profile profile;
			profile.addObserver(&tempObserver);

			MMG_Profile *myProfile = aClient->GetProfile();
			MMG_Profile *requestedProfile;

			MySQLDatabase::ourInstance->QueryProfileName(profileId, &profile);
			MySQLDatabase::ourInstance->QueryClanFullInfo(myProfile->m_ClanId, &memberCount, &myClan);

			SvClient *onlinePlayer = SvClientManager::ourInstance->FindPlayerByProfileId(profile.m_ProfileId);
			if (onlinePlayer)
				requestedProfile = onlinePlayer->GetProfile();
			else
				requestedProfile = &profile;

			if (option == 0)
			{
				if (myClan.m_PlayerOfWeek == requestedProfile->m_ProfileId)
					MySQLDatabase::ourInstance->UpdateClanPlayerOfWeek(myClan.m_ClanId, 0);

				if (requestedProfile->m_RankInClan < 3)
					MySQLDatabase::ourInstance->DeleteProfileClanInvites(requestedProfile->m_ProfileId, myClan.m_ClanId);

				if (requestedProfile->m_RankInClan < 2)
					MySQLDatabase::ourInstance->DeleteProfileClanMessages(requestedProfile->m_ProfileId);

				if (myProfile->m_ProfileId == requestedProfile->m_ProfileId)
				{
					if (myProfile->m_RankInClan == 1)
					{
						if (memberCount > 1)
						{
							ProfileStateObserver leaderObserver;
							MMG_Profile clanMembers[512];
							uint officers[512], grunts[512];
							uint i=0, j=0, k=0;
							uint newLeaderId = 0;
							uint newLeaderIndex = 0;

							memset(officers, 0, sizeof(officers));
							memset(grunts, 0, sizeof(grunts));

							MySQLDatabase::ourInstance->QueryProfileList(memberCount, myClan.m_ClanMembers, clanMembers);

							while (myClan.m_ClanMembers[i] > 0)
							{
								if (clanMembers[i].m_RankInClan == 2)
									officers[j++] = myClan.m_ClanMembers[i];

								if (clanMembers[i].m_RankInClan == 3)
									grunts[k++] = myClan.m_ClanMembers[i];

								i++;
							}

							if (j>0)
								newLeaderId = officers[MC_MTwister().Random(0, j-1)];
							else
								newLeaderId = grunts[MC_MTwister().Random(0, k-1)];

							for (newLeaderIndex = 0; newLeaderIndex < memberCount; newLeaderIndex++)
							{
								if (myClan.m_ClanMembers[newLeaderIndex] == newLeaderId)
									break;
							}

							clanMembers[newLeaderIndex].addObserver(&leaderObserver);

							SvClient *newLeaderOnline = SvClientManager::ourInstance->FindPlayerByProfileId(clanMembers[newLeaderIndex].m_ProfileId);
							if (newLeaderOnline)
								newLeaderOnline->GetProfile()->setRankInClan(1);
							else
								clanMembers[newLeaderIndex].setRankInClan(1);
						}
						else
						{
							MySQLDatabase::ourInstance->DeleteClan(myClan.m_ClanId);
						}
					}
					
					myProfile->setClanId(0);
				}
				else
				{
					requestedProfile->setClanId(0);
				}
			}
			else
			{
				if (option == 1 && myProfile->m_RankInClan == 1)
				{
					requestedProfile->setRankInClan(1);

					MySQLDatabase::ourInstance->DeleteProfileClanMessages(myProfile->m_ProfileId);
					myProfile->setRankInClan(2);
				}
				else
				{
					if (option == 3)
						MySQLDatabase::ourInstance->DeleteProfileClanInvites(requestedProfile->m_ProfileId, myClan.m_ClanId);
				
					requestedProfile->setRankInClan(option);
				}
			}
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_FULL_INFO_REQUEST:");

			MN_WriteMessage	responseMessage(4096);
			
			uint clanId, padZero;
			if (!aMessage->ReadUInt(clanId) || !aMessage->ReadUInt(padZero))
				return false;

			uint memberCount;
			MMG_Clan::FullInfo clanFullInfo;

			// request clan info from database
			MySQLDatabase::ourInstance->QueryClanFullInfo(clanId, &memberCount, &clanFullInfo);

			DebugLog(L_INFO, "MESSAGING_CLAN_FULL_INFO_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_RESPONSE);
			
			responseMessage.WriteString(clanFullInfo.m_FullClanName);
			responseMessage.WriteString(clanFullInfo.m_ShortClanName);
			responseMessage.WriteString(clanFullInfo.m_Motto);
			responseMessage.WriteString(clanFullInfo.m_LeaderSays);
			responseMessage.WriteString(clanFullInfo.m_Homepage);

			responseMessage.WriteUInt(memberCount);

			for (uint i = 0; i < memberCount; i++)
				responseMessage.WriteUInt(clanFullInfo.m_ClanMembers[i]);

			responseMessage.WriteUInt(clanFullInfo.m_ClanId);
			responseMessage.WriteUInt(clanFullInfo.m_PlayerOfWeek);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_INVITE_PLAYER_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_INVITE_PLAYER_REQUEST:");

			MN_WriteMessage	responseMessage(2048);

			uint profileId, padZero;
			if (!aMessage->ReadUInt(profileId) || !aMessage->ReadUInt(padZero))
				return false;

			uchar myStatusCode;
			MMG_Clan::Description myClan;

			MMG_Profile invitedProfile;
			uint msgId = 0;
			
			MySQLDatabase::ourInstance->QueryProfileName(profileId, &invitedProfile);
			MySQLDatabase::ourInstance->CheckIfInvitedAlready(aClient->GetProfile()->m_ClanId, profileId, &msgId);

			if (invitedProfile.m_ProfileId == 0)
				myStatusCode = myClanStrings::MODIFY_FAIL_MASSGATE;
			else if (invitedProfile.m_ClanId > 0)
				myStatusCode = myClanStrings::FAIL_ALREADY_IN_CLAN;
			else if (msgId > 0)
				myStatusCode = myClanStrings::FAIL_DUPLICATE;
			//else if (aClient->GetProfile()->m_RankInClan > 2)
			//	myStatusCode = myClanStrings::FAIL_INVALID_PRIVILIGES;
			//else if (!querysuccess)
			//	myStatusCode = myClanStrings::InternalMassgateError;
			//else if (checkimsettings)
			//	myStatusCode = myClanStrings::INVITE_FAIL_PLAYER_IGNORE_MESSAGES;
			else
			{
				MySQLDatabase::ourInstance->QueryClanDescription(aClient->GetProfile()->m_ClanId, &myClan);

				MMG_InstantMessageListener::InstantMessage im;
				im.m_SenderProfile.m_ProfileId = aClient->GetProfile()->m_ProfileId;
				im.m_RecipientProfile = profileId;
				swprintf(im.m_Message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|%u|%ws|%ws|", myClan.m_ClanId, aClient->GetProfile()->m_Name, myClan.m_FullName);

				// messageid and writtenat are generated in the AddInstantMessage function
				MySQLDatabase::ourInstance->AddInstantMessage(aClient->GetProfile()->m_ProfileId, &im);
				
				myStatusCode = myClanStrings::InviteSent;

				// if client is online send them the invite directly
				SvClient *invitee = SvClientManager::ourInstance->FindPlayerByProfileId(profileId);

				if (invitee)
				{
					MN_WriteMessage inviteMsg(2048);

					inviteMsg.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
					im.ToStream(&inviteMsg);

					if (!invitee->SendData(&inviteMsg))
						return false;
				}
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_MASSGATE_GENERIC_STATUS_RESPONSE);
			responseMessage.WriteUChar(myStatusCode);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_INVITE_PLAYER_RESPONSE:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_INVITE_PLAYER_RESPONSE:");

			MN_WriteMessage	responseMessage(2048);

			uint clanId, padZero;
			uchar acceptOrNot;

			if (!aMessage->ReadUInt(clanId) || !aMessage->ReadUChar(acceptOrNot) || !aMessage->ReadUInt(padZero))
				return false;

			if (acceptOrNot)
			{
				MMG_Clan::Description theClan;
				MySQLDatabase::ourInstance->QueryClanDescription(clanId, &theClan);

				if (theClan.m_ClanId > 0)
				{
					aClient->GetProfile()->setClanId(clanId);
				}
				else
				{
					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_MASSGATE_GENERIC_STATUS_RESPONSE);
					responseMessage.WriteUChar(myClanStrings::MODIFY_FAIL_MASSGATE);

					if (!aClient->SendData(&responseMessage))
						return false;
				}
			}
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_POST_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_GUESTBOOK_POST_REQ:");

			uint clanId, messageId, requestId;

			MMG_ClanGuestbookProtocol::GetRsp::GuestbookEntry entry;

			if (!aMessage->ReadUInt(clanId) || !aMessage->ReadUInt(messageId) || !aMessage->ReadUInt(requestId))
				return false;

			if (!aMessage->ReadString(entry.m_Message, ARRAYSIZE(entry.m_Message)))
				return false;

			entry.m_ProfileId = aClient->GetProfile()->m_ProfileId;
			MySQLDatabase::ourInstance->AddClanGuestbookEntry(clanId, requestId, &entry);

			if (messageId)
			{
				// refresh the guestbook (todo: temporary)
				MN_WriteMessage responseMessage(2048);
				responseMessage.WriteUInt(requestId);
				responseMessage.WriteUInt(clanId);

				MN_ReadMessage temp(2048);
				temp.BuildMessage(responseMessage.GetDataStream(), responseMessage.GetDataLength());
				this->HandleMessage(aClient, &temp, MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_GET_REQ);
			}
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_GET_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_GUESTBOOK_GET_REQ:");
			
			MN_WriteMessage	responseMessage(4096);

			uint requestId, clanId;
			if (!aMessage->ReadUInt(requestId) || !aMessage->ReadUInt(clanId))
				return false;

			uint entryCount = 0;
			MMG_ClanGuestbookProtocol::GetRsp myResponse;
			
			MySQLDatabase::ourInstance->QueryClanGuestbook(clanId, &entryCount, &myResponse);

			uint profileIds[32];
			memset(profileIds, 0, sizeof(profileIds));

			for (uint i = 0; i < entryCount; i++)
				profileIds[i] = myResponse.m_Entries[i].m_ProfileId;

			std::qsort(profileIds, 32, sizeof(uint), [](const void *a, const void *b) {
				if (*(uint*)a < *(uint*)b) return -1;
				if (*(uint*)a > *(uint*)b) return 1;
				return 0;
			});

			std::set<uint> uniqueIds(profileIds, profileIds+32);
			for (auto iter = uniqueIds.begin(); iter != uniqueIds.end(); ++iter)
			{
				if (*iter > 0 && *iter != aClient->GetProfile()->m_ProfileId)
				{
					MMG_Profile poster;
					MySQLDatabase::ourInstance->QueryProfileName(*iter, &poster);

					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
					poster.ToStream(&responseMessage);
				}
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_GET_RSP);
			responseMessage.WriteUInt(requestId);
			responseMessage.WriteUInt(entryCount);

			for (uint i = 0; i < entryCount; i++)
			{
				responseMessage.WriteString(myResponse.m_Entries[i].m_Message);
				responseMessage.WriteUInt(myResponse.m_Entries[i].m_Timestamp);
				responseMessage.WriteUInt(myResponse.m_Entries[i].m_ProfileId);
				responseMessage.WriteUInt(myResponse.m_Entries[i].m_MessageId);
			}

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_GUESTBOOK_DELETE_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_GUESTBOOK_DELETE_REQ:");

			uint messageId;
			uchar deleteAll;
			if (!aMessage->ReadUInt(messageId) || !aMessage->ReadUChar(deleteAll))
				return false;

			MMG_Profile *myProfile = aClient->GetProfile();
			bool QueryOK = MySQLDatabase::ourInstance->DeleteClanGuestbookEntry(myProfile->m_ClanId, messageId, deleteAll);

			/*if (!QueryOK)
			{
				MN_WriteMessage responseMessage(2048);
				responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_MASSGATE_GENERIC_STATUS_RESPONSE);
				responseMessage.WriteUChar(myClanStrings::MODIFY_FAIL_INVALID_PRIVILIGES);

				if (!aClient->SendData(&responseMessage))
					return false;
			}
			*/
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_MESSAGE_SEND_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_MESSAGE_SEND_REQ:");

			MN_WriteMessage	responseMessage(2048);

			wchar_t myMessage[WIC_INSTANTMSG_MAX_LENGTH];
			memset(myMessage, 0, sizeof(myMessage));

			if (!aMessage->ReadString(myMessage, ARRAYSIZE(myMessage)))
				return false;

			uint memberCount = 0;
			MMG_Clan::FullInfo myClan;
			time_t local_timestamp = time(NULL);

			MySQLDatabase::ourInstance->QueryClanFullInfo(aClient->GetProfile()->m_ClanId, &memberCount, &myClan);

			for (uint i = 0; i < memberCount; i++)
			{
				if (myClan.m_ClanMembers[i] > 0 && myClan.m_ClanMembers[i] != aClient->GetProfile()->m_ProfileId)
				{
					MMG_InstantMessageListener::InstantMessage im;

					im.m_SenderProfile.m_ProfileId = aClient->GetProfile()->m_ProfileId;
					im.m_RecipientProfile = myClan.m_ClanMembers[i];
					swprintf(im.m_Message, WIC_INSTANTMSG_MAX_LENGTH, L"|clms|%ws", myMessage);
					im.m_WrittenAt = local_timestamp;

					MySQLDatabase::ourInstance->AddInstantMessage(aClient->GetProfile()->m_ProfileId, &im);

					// if player is online send them the message instantly
					SvClient *player = SvClientManager::ourInstance->FindPlayerByProfileId(im.m_RecipientProfile);
					if (player)
					{
						MN_WriteMessage clanMsg(2048);

						clanMsg.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IM_RECEIVE);
						im.ToStream(&clanMsg);

						if (!player->SendData(&clanMsg))
							continue;
					}
				}
			}
		}
		break;

#endif
		
		case MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_ONLINE:
		{
			DebugLog(L_INFO, "MESSAGING_SET_STATUS_ONLINE:");

			//handle padding
			uint padZero;
			if (!aMessage->ReadUInt(padZero))
				return false;

			MMG_Profile *myProfile = aClient->GetProfile();
			myProfile->setOnlineStatus(1);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_PLAYING:
		{
			DebugLog(L_INFO, "MESSAGING_SET_STATUS_PLAYING:");

			//IDA wic.exe sub_7A1290 MMG_Messaging::SetStatusPlaying

			// either MMG_AccountProxy::State_t or MMG_MasterConnection::State
			uint serverId;
			if (!aMessage->ReadUInt(serverId))
				return false;

			//handle padding
			uint padZero;
			if (!aMessage->ReadUInt(padZero))
				return false;

			aClient->GetProfile()->setOnlineStatus(serverId);
		}
		break;

#ifdef USING_MYSQL_DATABASE

		case MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_FIND_PROFILE_REQUEST:");

			MN_WriteMessage	responseMessage(4096);

			uint maxResults;
			if (!aMessage->ReadUInt(maxResults))
				return false;

			wchar_t search[WIC_NAME_MAX_LENGTH];
			memset(search, 0, sizeof(search));
			if (!aMessage->ReadString(search, ARRAYSIZE(search)))
				return false;

			uint padZero;
			if (!aMessage->ReadUInt(padZero))
				return false;

			uint resultCount=0;
			uint profileIds[100];
			memset(profileIds, 0, sizeof(profileIds));

			MySQLDatabase::ourInstance->QueryPlayerSearch(search, maxResults, &resultCount, profileIds);

			DebugLog(L_INFO, "MESSAGING_FIND_PROFILE_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_RESPONSE);
			responseMessage.WriteUInt(resultCount);

			for(uint i = 0; i < resultCount; i++)
				responseMessage.WriteUInt(profileIds[i]);

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_PROFILE_SEARCH_COMPLETE);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_FIND_CLAN_REQUEST:");

			MN_WriteMessage	responseMessage(4096);

			uint maxResults;
			if (!aMessage->ReadUInt(maxResults))
				return false;

			wchar_t search[WIC_CLANNAME_MAX_LENGTH];
			memset(search, 0, sizeof(search));
			if (!aMessage->ReadString(search, ARRAYSIZE(search)))
				return false;

			uint padZero;
			if (!aMessage->ReadUInt(padZero))
				return false;

			uint resultCount=0;
			MMG_Clan::Description clans[100];

			MySQLDatabase::ourInstance->QueryClanSearch(search, maxResults, &resultCount, clans);

			DebugLog(L_INFO, "MESSAGING_FIND_CLAN_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_RESPONSE);
			responseMessage.WriteUInt(resultCount);

			for (uint i = 0; i < resultCount; i++)
			{
				clans[i].ToStream(&responseMessage);

				if (responseMessage.GetDataLength() + sizeof(MMG_Clan::Description) >= 4096)
				{
					if (!aClient->SendData(&responseMessage))
						break;
				}
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_FIND_CLAN_SEARCH_COMPLETE);

			if (responseMessage.GetDataLength() > 0 && !aClient->SendData(&responseMessage))
				return false;
		}
		break;

#endif

		case MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS:
		{
			DebugLog(L_INFO, "MESSAGING_GET_CLIENT_METRICS:");

			MN_WriteMessage	responseMessage(2048);

			//handle padding
			uchar padZero;
			if (!aMessage->ReadUChar(padZero))
				return false;

			DebugLog(L_INFO, "MESSAGING_GET_CLIENT_METRICS:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS);

			//char key[16] = "";
			//char value[96] = "";
			
			// TODO
			// this has something to do with EXMASS_Client::ReceiveNotification case 27) 
			//
			// enum MMG_ClientMetric::Context
			// number of options
			// key
			// value

			responseMessage.WriteUChar(0);
			responseMessage.WriteUInt(0);
			//responseMessage.WriteString(key, ARRAYSIZE(key));
			//responseMessage.WriteString(value, ARRAYSIZE(value));

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_SET_CLIENT_SETTINGS:
		{
			DebugLog(L_INFO, "MESSAGING_SET_CLIENT_SETTINGS:");

			uchar metricContext;
			uint metricCount;
			
			if (!aMessage->ReadUChar(metricContext) || !aMessage->ReadUInt(metricCount))
				return false;

			for (uint i = 0; i < metricCount; i++)
			{
				char key[16] = "";
				char value[96] = "";

				if (!aMessage->ReadString(key, ARRAYSIZE(key)) || !aMessage->ReadString(value, ARRAYSIZE(value)))
					return false;

				DebugLog(L_INFO, "Metric [%u]: %s -- %s", (uint)metricContext, key, value);
			}
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE:
		{
			DebugLog(L_INFO, "MESSAGING_STARTUP_SEQUENCE_COMPLETE:");

			MN_WriteMessage	responseMessage(2048);

			//handle padding
			uint padZero;
			if (!aMessage->ReadUInt(padZero) || !aMessage->ReadUInt(padZero))
				return false;

			this->SendStartupSequenceComplete(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLIENT_REQ_GET_PCC:
		{
			DebugLog(L_INFO, "MESSAGING_CLIENT_REQ_GET_PCC:");

			MN_WriteMessage	responseMessage(2048);

			// This is called when the clan profile page is opened, player created content

			uint requestCount;
			if (!aMessage->ReadUInt(requestCount))
				return false;

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLIENT_RSP_GET_PCC);
			responseMessage.WriteUInt(requestCount);// Number of responses

			for (uint i = 0; i < requestCount; i++)
			{
				uint requestId;
				uchar requestType;

				aMessage->ReadUInt(requestId);
				aMessage->ReadUChar(requestType);

				// Write these back to the outgoing messages
				responseMessage.WriteUInt(requestId);		// Content id
				responseMessage.WriteUInt(1);				// Content sequence number
				responseMessage.WriteUChar(requestType);	// Content type (0 is profile image, 1 is clan image)
				responseMessage.WriteString("https://my_domain_here.abc/my_image.dds"); // Image url
			}

			//if (!aClient->SendData(&responseMessage))
			//	return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_ABUSE_REPORT:
		{
			DebugLog(L_INFO, "MESSAGING_ABUSE_REPORT:");
			
			uint profileIdReported;
			if (!aMessage->ReadUInt(profileIdReported))
				return false;
			
			wchar_t anAbuseReport[256];
			memset(anAbuseReport, 0, sizeof(anAbuseReport));
			if (!aMessage->ReadString(anAbuseReport, ARRAYSIZE(anAbuseReport)))
				return false;
			
			//handle padding
			uchar aZero;
			if (!aMessage->ReadUChar(aZero))
				return false;

#ifdef USING_MYSQL_DATABASE
			MMG_Profile myProfile = *aClient->GetProfile();
			MySQLDatabase::ourInstance->AddAbuseReport(myProfile.m_ProfileId, myProfile, profileIdReported, anAbuseReport);
#endif
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_REQ:
		{
			MN_WriteMessage	responseMessage(2048);

			char langCode[3]; // "EN"
			aMessage->ReadString(langCode, ARRAYSIZE(langCode));

			DebugLog(L_INFO, "MESSAGING_OPTIONAL_CONTENT_GET_REQ: %s", langCode);

			// Optional content (I.E maps)
			this->SendOptionalContent(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_RETRY_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_OPTIONAL_CONTENT_RETRY_REQ:");

			MN_WriteMessage	responseMessage(2048);
			// TODO
			// Is this used?
			// Retry
			// this->SendOptionalContent(aClient, &responseMessage);
		}
		break;
#ifdef USING_MYSQL_DATABASE
		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_POST_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GUESTBOOK_POST_REQ:");
			
			uint profileId, messageId, requestId;

			MMG_ProfileGuestBookProtocol::GetRsp::GuestbookEntry entry;

			if (!aMessage->ReadUInt(profileId) || !aMessage->ReadUInt(messageId) || !aMessage->ReadUInt(requestId))
				return false;

			if (!aMessage->ReadString(entry.m_Message, ARRAYSIZE(entry.m_Message)))
				return false;

			entry.m_ProfileId = aClient->GetProfile()->m_ProfileId;
			MySQLDatabase::ourInstance->AddProfileGuestbookEntry(profileId, requestId, &entry);

			if (messageId)
			{
				// refresh the guestbook (todo: temporary)
				MN_WriteMessage responseMessage(2048);
				responseMessage.WriteUInt(requestId);
				responseMessage.WriteUInt(profileId);

				MN_ReadMessage temp(2048);
				temp.BuildMessage(responseMessage.GetDataStream(), responseMessage.GetDataLength());
				this->HandleMessage(aClient, &temp, MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_REQ);
			}
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GUESTBOOK_GET_REQ:");
			
			MN_WriteMessage	responseMessage(4096);

			uint requestId, profileId;
			if (!aMessage->ReadUInt(requestId) || !aMessage->ReadUInt(profileId))
				return false;

			uint entryCount = 0;
			MMG_ProfileGuestBookProtocol::GetRsp myResponse;
			
			MySQLDatabase::ourInstance->QueryProfileGuestbook(profileId, &entryCount, &myResponse);

			uint profileIds[32];
			memset(profileIds, 0, sizeof(profileIds));

			for (uint i = 0; i < entryCount; i++)
				profileIds[i] = myResponse.m_Entries[i].m_ProfileId;

			std::qsort(profileIds, 32, sizeof(uint), [](const void *a, const void *b) {
				if (*(uint*)a < *(uint*)b) return -1;
				if (*(uint*)a > *(uint*)b) return 1;
				return 0;
			});

			std::set<uint> uniqueIds(profileIds, profileIds+32);
			for (auto iter = uniqueIds.begin(); iter != uniqueIds.end(); ++iter)
			{
				if (*iter > 0 && *iter != aClient->GetProfile()->m_ProfileId)
				{
					MMG_Profile poster;
					MySQLDatabase::ourInstance->QueryProfileName(*iter, &poster);

					responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
					poster.ToStream(&responseMessage);
				}
			}

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_RSP);
			responseMessage.WriteUInt(requestId);
			responseMessage.WriteUChar(0);			//TODO: IgnoresGettingProfile?
			responseMessage.WriteUInt(entryCount);

			for (uint i = 0; i < entryCount; i++)
			{
				responseMessage.WriteString(myResponse.m_Entries[i].m_Message);
				responseMessage.WriteUInt(myResponse.m_Entries[i].m_Timestamp);
				responseMessage.WriteUInt(myResponse.m_Entries[i].m_ProfileId);
				responseMessage.WriteUInt(myResponse.m_Entries[i].m_MessageId);
			}

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_DELETE_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GUESTBOOK_DELETE_REQ:");

			uint messageId;
			uchar deleteAll;	//deleteAllByThisProfile
			if (!aMessage->ReadUInt(messageId) || !aMessage->ReadUChar(deleteAll))
				return false;

			MySQLDatabase::ourInstance->DeleteProfileGuestbookEntry(aClient->GetProfile()->m_ProfileId, messageId, deleteAll);

			// no response, rest of packet is MESSAGING_PROFILE_GUESTBOOK_GET_REQ
		}
		break;
#endif
		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_SET_EDITABLES_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_SET_EDITABLES_REQ:");

			MMG_ProfileEditableVariablesProtocol::GetRsp myResponse;

			if (!myResponse.FromStream(aMessage))
				return false;

#ifdef USING_MYSQL_DATABASE
			MySQLDatabase::ourInstance->SaveEditableVariables(aClient->GetProfile()->m_ProfileId, myResponse.motto, myResponse.homepage);
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GET_EDITABLES_REQ:");

			MN_WriteMessage	responseMessage(2048);

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;

			MMG_ProfileEditableVariablesProtocol::GetRsp myResponse;

#ifndef USING_MYSQL_DATABASE
			wcscpy_s(myResponse.motto, L"");
			wcscpy_s(myResponse.homepage, L"");
#else
			MySQLDatabase::ourInstance->QueryEditableVariables(profileId, myResponse.motto, myResponse.homepage);
#endif
			DebugLog(L_INFO, "MESSAGING_PROFILE_GET_EDITABLES_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_RSP);
			myResponse.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_ADD_REMOVE_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_IGNORELIST_ADD_REMOVE_REQ:");

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;

			uchar operation;
			if (!aMessage->ReadUChar(operation))
				return false;
#ifdef USING_MYSQL_DATABASE
			if (operation == 1)
				MySQLDatabase::ourInstance->AddIgnoredProfile(aClient->GetProfile()->m_ProfileId, profileId);
			else
				MySQLDatabase::ourInstance->RemoveIgnoredProfile(aClient->GetProfile()->m_ProfileId, profileId);
#endif
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_IGNORELIST_GET_REQ:");

			MN_WriteMessage	responseMessage(2048);

			DebugLog(L_INFO, "MESSAGING_IGNORELIST_GET_RSP:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_RSP);

#ifndef USING_MYSQL_DATABASE
			//write uint (num ignored profiles)
			responseMessage.WriteUInt(0);

			//for each ignored profile
				//write uint (profile id)
#else
			uint ignoredCount = 0;
			uint myIgnoreList[64];
			memset(myIgnoreList, 0, sizeof(myIgnoreList));

			if (!MySQLDatabase::ourInstance->QueryIgnoredProfiles(aClient->GetProfile()->m_ProfileId, &ignoredCount, myIgnoreList))
				responseMessage.WriteUInt(0);
			else
			{
				responseMessage.WriteUInt(ignoredCount);

				for (uint i=0; i < ignoredCount; i++)
					responseMessage.WriteUInt(myIgnoreList[i]);
			}
#endif
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ:");

			MN_WriteMessage	responseMessage(2048);

			this->SendClanWarsFilterWeights(aClient, &responseMessage);
		}
		break;

		default:
			DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
		return false;
	}

	return true;
}

bool MMG_Messaging::SendProfileName(SvClient *aClient, MN_WriteMessage	*aMessage, MMG_Profile *aProfile)
{
	DebugLog(L_INFO, "MESSAGING_RESPOND_PROFILENAME: %ws", aProfile->m_Name);
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	aProfile->ToStream(aMessage);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendCommOptions(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_COMMUNICATION_OPTIONS_RSP:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_RSP);

	aMessage->WriteUInt(aClient->GetOptions()->ToUInt());

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendIMSettings(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_IM_SETTINGS:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS);

	IM_Settings mySettings;
	mySettings.m_Friends = aClient->GetOptions()->m_ReceiveFromFriends;
	mySettings.m_Clanmembers = aClient->GetOptions()->m_ReceiveFromClanMembers;
	mySettings.m_Acquaintances = aClient->GetOptions()->m_ReceiveFromAcquaintances;
	mySettings.m_Anyone = aClient->GetOptions()->m_ReceiveFromEveryoneElse;
	mySettings.ToStream(aMessage);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendPingsPerSecond(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_PPS_SETTINGS_RSP:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_RSP);
	aMessage->WriteInt(WIC_CLIENT_PPS);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendStartupSequenceComplete(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_STARTUP_SEQUENCE_COMPLETE:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendOptionalContent(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_OPTIONAL_CONTENT_GET_RSP:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_RSP);

	// Write the map info data stream
	MMG_OptionalContentProtocol::ourInstance->ToStream(aMessage);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendClanWarsFilterWeights(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_RSP:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_RSP);

	//TODO: i have no idea what these are for
	aMessage->WriteFloat(0.0f);	//myClanWarsHaveMapWeight
	aMessage->WriteFloat(0.0f);	//myClanWarsDontHaveMapWeight
	aMessage->WriteFloat(0.0f);	//myClanWarsPlayersWeight
	aMessage->WriteFloat(0.0f);	//myClanWarsWrongOrderWeight
	aMessage->WriteFloat(0.0f);	//myClanWarsRatingDiffWeight
	aMessage->WriteFloat(0.0f);	//myClanWarsMaxRatingDiff

	return aClient->SendData(aMessage);
}