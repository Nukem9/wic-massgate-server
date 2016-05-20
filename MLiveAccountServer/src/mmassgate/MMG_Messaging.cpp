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

MMG_Messaging::MMG_Messaging()
{
}

bool MMG_Messaging::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	MN_WriteMessage	responseMessage(2048);

	switch(aDelimiter)
	{
		case MMG_ProtocolDelimiters::MESSAGING_RETRIEVE_PROFILENAME:
		{
			DebugLog(L_INFO, "MESSAGING_RETRIEVE_PROFILENAME:");

			ushort count;
			if (!aMessage->ReadUShort(count))
				return false;

#ifndef USING_MYSQL_DATABASE

				//handle profiles (count). unnecessary really, since count is going to be 0 if youre not using the database
#else
			for (int i = 0; i < count; i++)
			{
				MMG_Profile myFriend;

				uint id;
				if (!aMessage->ReadUInt(id))
					return false;

				bool QueryOK = MySQLDatabase::ourInstance->QueryProfileName(id, &myFriend);

				if(QueryOK)
					this->SendProfileName(aClient, &responseMessage, &myFriend);
			}
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST:");

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

			this->SendFriend(aClient, &responseMessage);
			this->SendAcquaintance(aClient, &responseMessage);
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
			// NOTE:
			// there doesnt seem to be a response, there is a lot of client side validation, making it unnecessary
			// if the client behaves weird, use MESSAGING_GET_FRIENDS_RESPONSE
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_REMOVE_FRIEND_REQUEST:");

			uint profileId;
			if (!aMessage->ReadUInt(profileId))
				return false;

#ifdef USING_MYSQL_DATABASE

			MySQLDatabase::ourInstance->RemoveFriend(aClient->GetProfile()->m_ProfileId, profileId);

#endif

			DebugLog(L_INFO, "MESSAGING_REMOVE_FRIEND_RESPONSE:");
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_REMOVE_FRIEND_RESPONSE);
			responseMessage.WriteUInt(profileId);	//NOTE: might not be profileId
			aClient->SendData(&responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_IM_CHECK_PENDING_MESSAGES:
		{
			DebugLog(L_INFO, "MESSAGING_IM_CHECK_PENDING_MESSAGES:");

			//TODO
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

			bool QueryOK = MySQLDatabase::ourInstance->SaveUserOptions(aClient->GetProfile()->m_ProfileId, commOptions);
			//if (QueryOK)
				//response message?
#endif
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_GET_COMMUNICATION_OPTIONS_REQ:");

			this->SendCommOptions(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS:
		{
			DebugLog(L_INFO, "MESSAGING_GET_IM_SETTINGS:");

			this->SendIMSettings(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_GET_PPS_SETTINGS_REQ:");

			this->SendPingsPerSecond(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_REQUEST:
		{
			// TODO
			wchar_t clanName[32];
			wchar_t clanTag[8];
			char displayTag[8];
			uint zero;

			aMessage->ReadString(clanName, ARRAYSIZE(clanName));
			aMessage->ReadString(clanTag, ARRAYSIZE(clanTag));
			aMessage->ReadString(displayTag, ARRAYSIZE(displayTag));
			aMessage->ReadUInt(zero);

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_CREATE_RESPONSE);
			responseMessage.WriteUChar(1); // successflag
			responseMessage.WriteUInt(4321); // clan id
			//responseMessage.WriteUChar(0);
			//responseMessage.WriteUInt(0);
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_FULL_INFO_REQUEST:");

			uint ProfileId = 0;
			uint aUInt1 = 0;
			aMessage->ReadUInt(ProfileId);
			aMessage->ReadUInt(aUInt1);
			
			// TODO
			wchar_t clanName[32];
			wchar_t clanTag[11];
			wchar_t clanMotto[256];
			wchar_t clanMessageOfTheDay[256];
			wchar_t clanHomepage[256];
			uint clanNumberOfPlayers = 0;
			//uint clanPlayerId[512];
			uint clanLeaderId = 0, clanPlayerOfTheWeekId = 0;
			memset(clanName, 0, sizeof(clanName));
			memset(clanTag, 0, sizeof(clanTag));
			memset(clanMotto, 0, sizeof(clanMotto));
			memset(clanMessageOfTheDay, 0, sizeof(clanMessageOfTheDay));
			memset(clanHomepage, 0, sizeof(clanHomepage));
			
			wcscpy(clanName, L"Developers & Moderators");
			wcscpy(clanTag, L"devs^");
			wcscpy(clanMotto, L"Today is a good day for clan wars!");
			wcscpy(clanMessageOfTheDay, L"Welcome clanmembers!");
			wcscpy(clanHomepage, L"massgate.org");
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_RESPONSE);
			responseMessage.WriteString(clanName);
			responseMessage.WriteString(clanTag);
			responseMessage.WriteString(clanMotto);
			responseMessage.WriteString(clanMessageOfTheDay);
			responseMessage.WriteString(clanHomepage);
			
			responseMessage.WriteUInt(1);	// number of players
			responseMessage.WriteUInt(ProfileId);	// player list - TODO: replace with loop
			responseMessage.WriteUInt(ProfileId);	// clan leader
			responseMessage.WriteUInt(0);	// player of the week
			
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_SET_STATUS_ONLINE:
		{
			DebugLog(L_INFO, "MESSAGING_SET_STATUS_ONLINE:");

			/*
			IDA wic.exe sub_7A1360 MMG_Messaging::SetStatusOnline
			after WriteDelimiter, the client appends a Uint ( 0, some sort of padding? ) to the packet
			handling this random 0 is necessary otherwise "message from client delimiter 0 type 0"
			will show up in the debug log.
			
			Massgate will crash if these 0's arent handled since the message reader will 
			treat it as the next delimiter.

			the same thing happens for cases:
				- MESSAGING_GET_CLIENT_METRICS -UChar, (IDA sub_7A1200 MMG_Messaging::GetClientMetrics)
				- MESSAGING_STARTUP_SEQUENCE_COMPLETE -2 x UInt, (IDA sub_7A1B00, see EXMASS_Client::ReceiveNotification lines 41 and 42)

			remove the handle padding code in the 3 cases to reproduce the crash
			*/

			//handle padding
			uint randomZero;
			if (!aMessage->ReadUInt(randomZero))
				return false;

			//TODO: is there a response for the client?
			// need some way to tell every other client that someone is online
			aClient->GetProfile()->m_OnlineStatus = 1;
			aClient->SetLoginStatus(true);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_CLIENT_METRICS:
		{
			DebugLog(L_INFO, "MESSAGING_GET_CLIENT_METRICS:");

			//handle padding
			uchar randomZero;
			if (!aMessage->ReadUChar(randomZero))
				return false;

			//TODO: i dont know what these are
			//DebugBreak();
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_SET_CLIENT_SETTINGS:
		{
			DebugLog(L_INFO, "MESSAGING_SET_CLIENT_SETTINGS:");

			//TODO
			//DebugBreak();
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE:
		{
			DebugLog(L_INFO, "MESSAGING_STARTUP_SEQUENCE_COMPLETE:");

			//handle padding
			uint randomZero;
			if (!aMessage->ReadUInt(randomZero) || !aMessage->ReadUInt(randomZero))
				return false;

			this->SendStartupSequenceComplete(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLIENT_REQ_GET_PCC:
		{
			DebugLog(L_INFO, "MESSAGING_CLIENT_REQ_GET_PCC:");

			// This is called when the clan profile page is opened, clan collosseum filters

			uint PCCRequestCount = 0;
			uint PCCRequestId = 0;
			uchar PCCRequestType = 0;
			aMessage->ReadUInt(PCCRequestCount);
			for (int i = 0; i < PCCRequestCount; i++)
			{
				aMessage->ReadUInt(PCCRequestId);
				aMessage->ReadUChar(PCCRequestType);
			}

			uint NumberOfPlayers = 0;
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_CLIENT_RSP_GET_PCC);
			responseMessage.WriteUInt(NumberOfPlayers);	// number of players

			for (int i = 0; i < NumberOfPlayers; i++)
			{
				responseMessage.WriteUInt(1); // ProfileId ???
				responseMessage.WriteUInt(1); // ???
				responseMessage.WriteUChar(1); // player rank?
				responseMessage.WriteString(""); // ??? 256 byte size
			}
			
			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_REQ:
		{
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
			// TODO
			// Is this used?
			// Retry
			// this->SendOptionalContent(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_POST_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GUESTBOOK_POST_REQ: Sending information");

			//18:00:76:00:4a:00:00:00:01:00:00:00:02:00:00:00:04:00:68:00:69:00:21:00:00:00 for text "hi!" 180076004a000000010000000200000004006800690021000000
			
			uint RequestId = 0, ProfileId = 0, MessageId = 0;
			wchar_t GuestBookMessage[128];

			aMessage->ReadUInt(ProfileId);
			aMessage->ReadUInt(RequestId);
			aMessage->ReadUInt(MessageId);
			aMessage->ReadString(GuestBookMessage, ARRAYSIZE(GuestBookMessage));
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_RSP);
			
			//currentEditables->ToStream(&responseMessage);

			//if (!aClient->SendData(&responseMessage))
			//	return false;
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GUESTBOOK_GET_REQ: Sending information");

			//0a:00:77:00:01:00:00:00:4a:00:00:00

			MN_WriteMessage	responseMessage1(8192);

			uint RequestId = 0, ProfileId = 0;
			aMessage->ReadUInt(RequestId);
			aMessage->ReadUInt(ProfileId);
			
			MMG_ProfileGuestBookProtocol myProfileGuestBook;

			myProfileGuestBook.m_RequestId = RequestId;
			myProfileGuestBook.m_IgnoresGettingProfile = 0;
			myProfileGuestBook.m_Count = 1;
			myProfileGuestBook.m_MaxSize = 8192;
			
			const int guestbooklength = 1;
			MMG_ProfileGuestBookEntry myGuestBookEntry[guestbooklength];
			for (int i = 0; i < guestbooklength; i++)
			{
				wcscpy(myGuestBookEntry[i].m_GuestBookMessage, L"Sup b1tches");
				myGuestBookEntry[i].m_TimeStamp = 6060 * i;
				myGuestBookEntry[i].m_ProfileId = ProfileId;
				myGuestBookEntry[i].m_MessageId = 1;
				
				myProfileGuestBook.m_GuestBookEntry[i] = myGuestBookEntry[i];
			}
			
			myProfileGuestBook.m_Data = sizeof(myProfileGuestBook.m_GuestBookEntry);
			
			responseMessage1.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_RSP);
			
			myProfileGuestBook.ToStream(&responseMessage1);

			//if (!aClient->SendData(&responseMessage))
			//	return false;
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_DELETE_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GUESTBOOK_DELETE_REQ: Sending information");

			//responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_RSP);
			
			//currentEditables->ToStream(&responseMessage);

			//if (!aClient->SendData(&responseMessage))
			//	return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_SET_EDITABLES_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_SET_EDITABLES_REQ: Sending information");

			/*// read message byte by byte
			uchar aUChar[6];
			for (int i=0;i<6;i++)
				aMessage->ReadUChar(aUChar[i]);
			*/
			
			// there might be something else in the message, however that tab is not accessible yet
			MMG_ProfileEditableVariablesProtocol myNewEditables;
			myNewEditables.FromStream(aMessage);
			
			// to do: save strings to profile (or profile related) database

			// to do: read freshly saved strings from profile (or profile related) database

			MMG_ProfileEditableVariablesProtocol myProfileEditableVariables;
			wcscpy(myProfileEditableVariables.m_Motto, L"Hello!");
			wcscpy(myProfileEditableVariables.m_Homepage, L"Bye!");
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_RSP);
			
			myProfileEditableVariables.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GET_EDITABLES_REQ: Sending information");

			uint ProfileId = 0;
			aMessage->ReadUInt(ProfileId);

			// query database with ProfileId for saved editables

			MMG_ProfileEditableVariablesProtocol myProfileEditableVariables;
			wcscpy(myProfileEditableVariables.m_Motto, L"Hi to everyone");
			wcscpy(myProfileEditableVariables.m_Homepage, L"GL & HF");
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_RSP);
			
			myProfileEditableVariables.ToStream(&responseMessage);

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
#ifndef USING_MYSQL_DATABASE
			if (operation == 1)
				printf("add\n");
			else
				printf("remove\n");
#else
			if (operation == 1)
				MySQLDatabase::ourInstance->AddIgnoredProfile(aClient->GetProfile()->m_ProfileId, profileId);
			else
				MySQLDatabase::ourInstance->RemoveIgnoredProfile(aClient->GetProfile()->m_ProfileId, profileId);
#endif
			// wic.exe IDA sub_BD7C90 (called from WICMASS_ContactsScreenHandler)
			// NOTE:
			// there doesnt seem to be a response, there is a lot of client side validation, making it unnecessary
			// if the client behaves weird, use MESSAGING_IGNORELIST_GET_RSP
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_IGNORELIST_GET_REQ:");

			this->SendProfileIgnoreList(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_CLAN_COLOSSEUM_GET_FILTER_WEIGHTS_REQ:");

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

bool MMG_Messaging::SendFriend(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_FRIENDS_RESPONSE:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_RESPONSE);

#ifndef USING_MYSQL_DATABASE
	//write uint (num friends)
	aMessage->WriteUInt(0);

	//for each friend
		//write uint (profile id)
#else
	uint friendCount=0;
	uint *myFriends;

	bool QueryOK = MySQLDatabase::ourInstance->QueryFriends(aClient->GetProfile()->m_ProfileId, &friendCount, &myFriends);

	if (QueryOK)
	{
		aMessage->WriteUInt(friendCount);

		for (uint i=0; i < friendCount; i++)
		{
			aMessage->WriteUInt(myFriends[i]);
		}
	}
	else
	{
		aMessage->WriteUInt(0);
	}

	if(friendCount > 0)
		delete [] myFriends;

	myFriends = nullptr;
#endif

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendAcquaintance(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_ACQUAINTANCES_RESPONSE:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_ACQUAINTANCES_RESPONSE);

#ifndef USING_MYSQL_DATABASE
	//write uint (num acquaitances)
	aMessage->WriteUInt(0);

	//for each acquaintance
		//write uint (profile id)
		//write uint number times played
#else

	//NOTE: this does not return acquaintances just yet, at the moment it sends
	//everyone currently registered.

	uint acquaintanceCount=0;
	uint *myAcquaintances;

	bool QueryOK = MySQLDatabase::ourInstance->QueryAcquaintances(aClient->GetProfile()->m_ProfileId, &acquaintanceCount, &myAcquaintances);

	if (QueryOK)
	{
		aMessage->WriteUInt(acquaintanceCount);

		for (uint i=0; i < acquaintanceCount; i++)
		{
			aMessage->WriteUInt(myAcquaintances[i]);	//profileId
			aMessage->WriteUInt(0);						//numTimesPlayed
		}
	}
	else
	{
		aMessage->WriteUInt(0);
	}

	if(acquaintanceCount > 0)
		delete [] myAcquaintances;

	myAcquaintances = nullptr;
#endif

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendCommOptions(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_COMMUNICATION_OPTIONS_RSP:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_RSP);

	// Temporary
	// to avoid another database query, a client now has an MMG_Options object which 
	// is read on login (MySQLDatabase::ourInstance->QueryUserProfile) from the commoptions field.
	// im not sure if this will cause problems later, the options will most likely
	// need to be stored in another table as there are extra fields in the ida data structure
	// that are not in the class.
	//
	aMessage->WriteUInt(aClient->GetOptions()->ToUInt());

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendIMSettings(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_GET_IM_SETTINGS:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS);

	// Temporary
	// to avoid another database query, a client now has an MMG_Options object which 
	// is read on login (MySQLDatabase::ourInstance->QueryUserProfile) from the commoptions field.
	// im not sure if this will cause problems later, the options will most likely
	// need to be stored in another table as there are extra fields in the ida data structure
	// that are not in the class.
	//
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
	aMessage->WriteUInt(WIC_CLIENT_PPS);

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
	//MMG_OptionalContentProtocol::ourInstance->ToStream(aMessage);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendProfileIgnoreList(SvClient *aClient, MN_WriteMessage *aMessage)
{
	DebugLog(L_INFO, "MESSAGING_IGNORELIST_GET_RSP:");
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_IGNORELIST_GET_RSP);

#ifndef USING_MYSQL_DATABASE
	//write uint (num ignored profiles)
	aMessage->WriteUInt(0);

	//for each ignored profile
		//write uint (profile id)
#else
	uint ignoredCount=0;
	uint *myIgnoreList;

	bool QueryOK = MySQLDatabase::ourInstance->QueryIgnoredProfiles(aClient->GetProfile()->m_ProfileId, &ignoredCount, &myIgnoreList);

	if (QueryOK)
	{
		aMessage->WriteUInt(ignoredCount);

		for (uint i=0; i < ignoredCount; i++)
		{
			aMessage->WriteUInt(myIgnoreList[i]);
		}
	}
	else
	{
		aMessage->WriteUInt(0);
	}

	if(ignoredCount > 0)
		delete [] myIgnoreList;

	myIgnoreList = nullptr;
#endif

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