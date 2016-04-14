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
		case MMG_ProtocolDelimiters::MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST:
		{
			DebugLog(L_INFO, "MESSAGING_GET_FRIENDS_AND_ACQUAINTANCES_REQUEST: Sending information");

			MMG_Profile *myProfile = aClient->GetProfile();

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
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_SET_COMMUNICATION_OPTIONS_REQ:
		{
			uint commOptions;

			if (!aMessage->ReadUInt(commOptions))
				return false;

			// TODO
			MMG_Options myOptions;
			myOptions.FromUInt(commOptions);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS:
		{
			this->SendIMSettings(aClient, &responseMessage);

			// TODO
			this->SendStartupSequenceComplete(aClient, &responseMessage);
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
			responseMessage.WriteUInt(1); // clan id
			aClient->SendData(&responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_CLAN_FULL_INFO_REQUEST:
		{
			// TODO
			wchar_t clanName[32];
			wchar_t clanTag[11];
			wchar_t clanMotto[256];
			wchar_t clanMessageOfTheDay[256];
			wchar_t clanHomepage[256];
			uint clanNumberOfPlayers = 0;
			uint clanPlayerId[512];
			uint clanLeaderId = 0, clanPlayerOfTheWeekId = 0;
			memset(clanName, 0, sizeof(clanName));
			memset(clanTag, 0, sizeof(clanTag));
			memset(clanMotto, 0, sizeof(clanMotto));
			memset(clanMessageOfTheDay, 0, sizeof(clanMessageOfTheDay));
			memset(clanHomepage, 0, sizeof(clanHomepage));
			
			uchar messagecontent[500];
			for (int i = 0; i < 500; i++)
				aMessage->ReadUChar(messagecontent[i]);

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
			responseMessage.WriteUInt(74);	// player list
			responseMessage.WriteUInt(74);	// clan leader
			responseMessage.WriteUInt(74);	// player of the week
			
			aClient->SendData(&responseMessage);
		}
		break;
		
		case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_REQ:
		{
			// Optional content (I.E maps)
			this->SendOptionalContent(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_RETRY_REQ:
		{
			// TODO
			// Is this used?
			// Retry
			// this->SendOptionalContent(aClient, &responseMessage);
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_GET_EDITABLES_REQ: Sending information");

			// to do: read strings from profile (or profile related) database

			MMG_ProfileEditableVariablesProtocol myProfileEditableVariables;
			wcscpy(myProfileEditableVariables.m_Motto, L"Hi to everyone");
			wcscpy(myProfileEditableVariables.m_Homepage, L"GL & HF");
			
			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GET_EDITABLES_RSP);
			
			myProfileEditableVariables.ToStream(&responseMessage);

			if (!aClient->SendData(&responseMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::MESSAGING_PROFILE_SET_EDITABLES_REQ:
		{
			DebugLog(L_INFO, "MESSAGING_PROFILE_SET_EDITABLES_REQ: Sending information");

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

			responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PROFILE_GUESTBOOK_GET_RSP);
			
			//currentEditables->ToStream(&responseMessage);

			//if (!aClient->SendData(&responseMessage))
			//	return false;
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
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	aProfile->ToStream(aMessage);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendCommOptions(SvClient *aClient, MN_WriteMessage *aMessage)
{
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_COMMUNICATION_OPTIONS_RSP);

	// Temporary
	MMG_Options myOptions;
	aMessage->WriteUInt(myOptions.ToUInt());

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendIMSettings(SvClient *aClient, MN_WriteMessage *aMessage)
{
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_IM_SETTINGS);

	// Temporary
	IM_Settings mySettings;
	mySettings.m_Friends = false;
	mySettings.m_Clanmembers = true;
	mySettings.m_Acquaintances = false;
	mySettings.m_Anyone = true;
	mySettings.ToStream(aMessage);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendPingsPerSecond(SvClient *aClient, MN_WriteMessage *aMessage)
{
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_GET_PPS_SETTINGS_RSP);
	aMessage->WriteUInt(WIC_CLIENT_PPS);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendStartupSequenceComplete(SvClient *aClient, MN_WriteMessage *aMessage)
{
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_STARTUP_SEQUENCE_COMPLETE);

	return aClient->SendData(aMessage);
}

bool MMG_Messaging::SendOptionalContent(SvClient *aClient, MN_WriteMessage *aMessage)
{
	aMessage->WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_OPTIONAL_CONTENT_GET_RSP);

	// Write the map info data stream
	//MMG_OptionalContentProtocol::ourInstance->ToStream(aMessage);

	return aClient->SendData(aMessage);
}