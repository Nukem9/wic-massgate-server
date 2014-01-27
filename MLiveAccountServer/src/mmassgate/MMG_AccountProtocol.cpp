#include "../stdafx.h"

bool MMG_AccountProtocol::Query::FromStream(MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_ReadMessage *aMessage)
{
	// Account message type delimiter
	this->m_Delimiter = aDelimiter;

	// Client game protocol version
	if (!aMessage->ReadUShort(this->m_Protocol))
		return false;

	if (this->m_Protocol != MassgateProtocolVersion)
	{
		this->m_StatusCode = IncorrectProtocolVersion;
		return true;
	}
	
	// Cipher type
	if (!aMessage->ReadUChar((uchar &)this->m_CipherIdentifier))
		return false;

	// CDKey sequence
	if (!aMessage->ReadUInt(this->m_EncryptionKeySequenceNumber))
		return false;

	// Custom: read the encryption keys for BlockTEA
	// Why?
	// CipherKeys are derived from a TigerMD5 hash of the CDKey
	// It's near impossible to reverse a key based on the sequence number
	// without a database of keys (which Massgate owns)
	for(int i = 0; i < ARRAYSIZE(this->m_CipherKeys); i++)
	{
		if (!aMessage->ReadULong(this->m_CipherKeys[i]))
			return false;
	}

	// "Dummy" encrypted data block length, but it's written AGAIN after this
	ushort dummyLen;
	if (!aMessage->ReadUShort(dummyLen))
		return false;

	// Read encrypted data block and length
	sizeptr_t	dataLen;
	char		dataBuffer[1024];

	if (!aMessage->ReadRawData(dataBuffer, sizeof(dataBuffer), &dataLen))
		return false;

	// Decrypt the data block
	if (!MMG_ICipher::DecryptWith(this->m_CipherIdentifier, this->m_CipherKeys, (uint *)&dataBuffer, dataLen))
		return false;

	// Create a new message reader with a decrypted data block
	MN_ReadMessage decryptedMsg(1024);
	if (!decryptedMsg.BuildMessage(&dataBuffer, dataLen))
		return false;

	// Read client variables
	if (!decryptedMsg.ReadUInt(this->m_RandomKey) || !decryptedMsg.ReadUInt(this->m_ProductId) || !decryptedMsg.ReadUInt(this->m_DistributionId))
		return false;

	switch(this->m_Delimiter)
	{
		case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ:
		{
			uint fingerprintSeed;
			uint fingerprintXor;

			if (!decryptedMsg.ReadUInt(fingerprintSeed) || !decryptedMsg.ReadUInt(fingerprintXor))
				return false;

			this->m_Authenticate.m_Fingerprint = fingerprintSeed ^ fingerprintXor;

			// Email
			if (!decryptedMsg.ReadString(this->m_Authenticate.m_Email, ARRAYSIZE(this->m_Authenticate.m_Email)))
				return false;

			// Password
			if (!decryptedMsg.ReadString(this->m_Authenticate.m_Password, ARRAYSIZE(this->m_Authenticate.m_Password)))
				return false;

			// Old profile usage (credentials stored in a file)
			if (!decryptedMsg.ReadUInt(this->m_Authenticate.m_UseProfile) || !decryptedMsg.ReadUChar(this->m_Authenticate.m_HasOldCredentials))
				return false;

			if (this->m_Authenticate.m_HasOldCredentials)
			{
				//MMG_AuthToken::FromStream(aMessage);
			}

			DebugLog(L_INFO, "ACCOUNT_AUTH_ACCOUNT_REQ: %s %ws", this->m_Authenticate.m_Email, this->m_Authenticate.m_Password);
		}
		break;

		case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
		{
			if (!this->m_Authenticate.m_Credentials.FromStream(aMessage))
				return false;
		}
		break;

		case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ:
		{
			uint fingerprintSeed;
			uint fingerprintXor;

			if (!decryptedMsg.ReadUInt(fingerprintSeed) || !decryptedMsg.ReadUInt(fingerprintXor))
				return false;

			this->m_RetrieveProfiles.m_Fingerprint = fingerprintSeed ^ fingerprintXor;

			// Email
			if (!decryptedMsg.ReadString(this->m_RetrieveProfiles.m_Email, ARRAYSIZE(this->m_RetrieveProfiles.m_Email)))
				return false;

			// Password
			if (!decryptedMsg.ReadString(this->m_RetrieveProfiles.m_Password, ARRAYSIZE(this->m_RetrieveProfiles.m_Password)))
				return false;
		}
		break;
	}

	return true;
}

bool MMG_AccountProtocol::Query::VerifyProductId()
{
	return (this->m_ProductId == WIC_PRODUCT_ID && this->m_DistributionId == WIC_CLIENT_DISTRIBUTION);
}

MMG_AccountProtocol::MMG_AccountProtocol()
{
}

bool MMG_AccountProtocol::HandleMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	Query myQuery;
	if (!myQuery.FromStream(aDelimiter, aMessage))
		return false;

	// Copy over the data to the main client manager class
	aClient->m_CipherIdentifier				= myQuery.m_CipherIdentifier;
	aClient->m_EncryptionKeySequenceNumber	= myQuery.m_EncryptionKeySequenceNumber;
	memcpy(aClient->m_CipherKeys, myQuery.m_CipherKeys, sizeof(myQuery.m_CipherKeys));

	MN_WriteMessage cryptMessage(1024);
	MMG_ProtocolDelimiters::Delimiter responseDelimiter;

	// Write the base identifier (random number, only a marker)
	cryptMessage.WriteUInt(myQuery.m_RandomKey);

	if (myQuery.m_StatusCode == IncorrectProtocolVersion || !myQuery.VerifyProductId())
	{
		// Invalid game version
		// This takes priority over maintenance due to potential protocol differences
		DebugLog(L_INFO, "ACCOUNT_PATCH_INFORMATION: Client has an old version");
		responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_PATCH_INFORMATION;

		this->WritePatchInformation(&cryptMessage, &myQuery);
	}
	else if (false)
	{
		// Massgate/Server maintenance notice
		responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_NOT_CONNECTED;

		this->WriteMaintenanceInformation(&cryptMessage);
	}
	else
	{
		switch(myQuery.m_Delimiter)
		{
			// Account authorization: Login
			case MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_REQ:
			{
				// Query the database and determine if credentials were valid
				uint accProfileId = 0;//Database::AuthUserAccount(myQuery.m_Authenticate.m_Email, myQuery.m_Authenticate.m_Password);

				DebugLog(L_INFO, "ACCOUNT_AUTH_ACCOUNT_RSP: Sending login response (id %i)", accProfileId);
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP;

				if (accProfileId == WIC_INVALID_ACCOUNT)
				{
					cryptMessage.WriteUChar(AuthFailed_BadCredentials);

					cryptMessage.WriteUChar(0);	// auth.mySuccessFlag
					cryptMessage.WriteUShort(0);// auth.myLatestVersion
				}
				else
				{
					// Update the maximum client timeout
					aClient->SetLoginStatus(true);
					aClient->SetTimeout(WIC_LOGGEDIN_NET_TIMEOUT);

					cryptMessage.WriteUChar(AuthSuccess);
					cryptMessage.WriteUChar(1);	// auth.mySuccessFlag
					cryptMessage.WriteUShort(0);// auth.myLatestVersion

					// Query profile
					//if (!Database::QueryUserProfile(accProfileId, aClient->GetProfile()))
					//	DebugLog(L_ERROR, "Failed to retrieve profile for valid account ");
					wcscpy_s(aClient->GetProfile()->m_Name, L"Nukem");
					aClient->GetProfile()->m_OnlineStatus = 1;
					aClient->GetProfile()->m_Rank = 18;

					// Write the profile info stream
					aClient->GetProfile()->ToStream(&cryptMessage);

					MMG_AuthToken *myAuthToken = aClient->GetToken();

					// TigerMD5 of ...? (possibly crypt keys)
					myAuthToken->m_Hash.m_Hash[0] = 0x558C0A1C;
					myAuthToken->m_Hash.m_Hash[1] = 0xA59C9FCA;
					myAuthToken->m_Hash.m_Hash[2] = 0x6566857D;
					myAuthToken->m_Hash.m_Hash[3] = 0x8A3FF551;
					myAuthToken->m_Hash.m_Hash[4] = 0xB69D17E5;
					myAuthToken->m_Hash.m_Hash[5] = 0xD7BBF74D;
					myAuthToken->m_Hash.m_HashLength = 6 * sizeof(ulong);
					myAuthToken->m_Hash.m_GeneratedFromHashAlgorithm = HASH_ALGORITHM_TIGER;
					myAuthToken->ToStream(&cryptMessage);// Write the authorization token info stream

					cryptMessage.WriteUInt(WIC_CREDAUTH_RESEND_S);	// periodicityOfCredentialsRequests (How long until the first is sent)
					cryptMessage.WriteUInt(0);						// myLeaseTimeLeft (Limited access key)
					cryptMessage.WriteUInt(45523626);				// myAntiSpoofToken (Random number)
				}
			}
			break;

			// Client requests a session update to prevent dropping
			case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
			{
				DebugLog(L_INFO, "ACCOUNT_NEW_CREDENTIALS_REQ");
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_RSP;

				// Default to success until it's actually implemented (if ever)
				cryptMessage.WriteUChar(AuthSuccess);

				// Write the authorization token info stream
				aClient->GetToken()->ToStream(&cryptMessage);

				// doCredentialsRequestAgain (in seconds)
				cryptMessage.WriteUInt(WIC_CREDAUTH_RESEND_S);
			}
			break;

			// Retrieve account profiles list (maximum 5)
			case MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_REQ:
			{
				DebugLog(L_INFO, "ACCOUNT_RETRIEVE_PROFILES_REQ");
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP;

				cryptMessage.WriteUChar(AuthSuccess);
				cryptMessage.WriteUChar(1);	// mySuccessFlag
				cryptMessage.WriteUInt(1);	// numUserProfiles
				cryptMessage.WriteUInt(0);	// lastUsedProfileId

				MMG_Profile *myProfile = aClient->GetProfile();
				wcscpy_s(myProfile->m_Name, L"Nukem");
				myProfile->m_ProfileId = 0;
				myProfile->m_ClanId = 0;
				myProfile->m_OnlineStatus = 0;
				myProfile->m_Rank = 18;
				myProfile->m_RankInClan = 0;

				// Write the profile info stream
				myProfile->ToStream(&cryptMessage);
			}
			break;

			default:
				DebugLog(L_INFO, "Unknown delimiter %i", aDelimiter);
			return false;
		}
	}

	// Write the main message header
	MN_WriteMessage	responseMsg(1024);
	responseMsg.WriteDelimiter(responseDelimiter);
	responseMsg.WriteUShort(MassgateProtocolVersion);
	responseMsg.WriteUChar(aClient->m_CipherIdentifier);
	responseMsg.WriteUInt(aClient->m_EncryptionKeySequenceNumber);

	// Encrypt and write the data to the main (outgoing) packet
	// Packet buffer can be modified because it is no longer used
	sizeptr_t dataLength = cryptMessage.GetDataLength();
	voidptr_t dataStream = cryptMessage.GetDataStream();

	if (!MMG_ICipher::EncryptWith(aClient->m_CipherIdentifier, aClient->m_CipherKeys, (uint *)dataStream, dataLength))
		return false;

	responseMsg.WriteUShort(dataLength);
	responseMsg.WriteRawData(dataStream, dataLength);

	// Finally send the message
	if (!aClient->SendData(&responseMsg))
		return false;

	return true;
}

void MMG_AccountProtocol::WritePatchInformation(MN_WriteMessage *aMessage, Query *aQuery)
{
	aMessage->WriteUChar(IncorrectProtocolVersion);

	// See WIC_REVISION_VER
	aMessage->WriteUInt(VER_1011);// Latest version
	aMessage->WriteUInt(VER_1000);// Client version

	char *url = "http://massgate.net/patches/wic/latest.txt";
	aMessage->WriteString(url);// masterPatchListUrl
	aMessage->WriteString(url);// masterChangelogUrl
}

void MMG_AccountProtocol::WriteMaintenanceInformation(MN_WriteMessage *aMessage)
{
	aMessage->WriteUChar(MassgateMaintenance);
}