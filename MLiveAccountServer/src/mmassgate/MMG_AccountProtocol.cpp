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
				if(!this->m_Authenticate.m_Credentials.FromStream(&decryptedMsg))
					return false;
			}

#ifndef USING_MYSQL_DATABASE
			DebugLog(L_INFO, "ACCOUNT_AUTH_ACCOUNT_REQ: %s %ws", this->m_Authenticate.m_Email, this->m_Authenticate.m_Password);
#else
			DebugLog(L_INFO, "ACCOUNT_AUTH_ACCOUNT_REQ: %s", this->m_Authenticate.m_Email);
#endif

		}
		break;

		case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ:
		{
			// Email
			if (!decryptedMsg.ReadString(this->m_Create.m_Email, ARRAYSIZE(this->m_Create.m_Email)))
				return false;

			// Password
			if (!decryptedMsg.ReadString(this->m_Create.m_Password, ARRAYSIZE(this->m_Create.m_Password)))
				return false;

			// Country code
			if (!decryptedMsg.ReadString(this->m_Create.m_Country, ARRAYSIZE(this->m_Create.m_Country)))
				return false;

			// Email-related information
			if (!decryptedMsg.ReadUChar(this->m_Create.m_EmailMeGameRelated) || !decryptedMsg.ReadUChar(this->m_Create.m_AcceptsEmail))
				return false;

#ifndef USING_MYSQL_DATABASE
			DebugLog(L_INFO, "ACCOUNT_CREATE_ACCOUNT_REQ: %s %ws", this->m_Create.m_Email, this->m_Create.m_Password);
#else
			DebugLog(L_INFO, "ACCOUNT_CREATE_ACCOUNT_REQ: %s", this->m_Create.m_Email);
#endif

		}
		break;

		case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ:
		{
			/* Nothing */
			DebugLog(L_INFO, "ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ:");
		}
		break;

		case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
		{
			if (!this->m_Authenticate.m_Credentials.FromStream(&decryptedMsg))
				return false;

			DebugLog(L_INFO, "ACCOUNT_NEW_CREDENTIALS_REQ:");
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

#ifndef USING_MYSQL_DATABASE
			DebugLog(L_INFO, "ACCOUNT_RETRIEVE_PROFILES_REQ: %s %ws", this->m_RetrieveProfiles.m_Email, this->m_RetrieveProfiles.m_Password);
#else
			DebugLog(L_INFO, "ACCOUNT_RETRIEVE_PROFILES_REQ: %s", this->m_RetrieveProfiles.m_Email);
#endif

		}
		break;

		case MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ:
		{
			// operations: add		6382692 ASCII 'add'
			//			   delete	6579564 ASCII 'del'
			if (!decryptedMsg.ReadUInt(this->m_ModifyProfile.m_Operation))
				return false;

			// profile id - used for delete
			if (!decryptedMsg.ReadUInt(this->m_ModifyProfile.m_ProfileId))
				return false;

			// profile name - used for add
			if (!decryptedMsg.ReadString(this->m_ModifyProfile.m_Name, ARRAYSIZE(this->m_ModifyProfile.m_Name)))
				return false;

			uint fingerprintSeed;
			uint fingerprintXor;

			if (!decryptedMsg.ReadUInt(fingerprintSeed) || !decryptedMsg.ReadUInt(fingerprintXor))
				return false;
			
			this->m_ModifyProfile.m_Fingerprint = fingerprintSeed ^ fingerprintXor;

			// email used for both add and delete
			if (!decryptedMsg.ReadString(this->m_ModifyProfile.m_Email, ARRAYSIZE(this->m_ModifyProfile.m_Email)))
				return false;

			// password used for both add and delete
			if (!decryptedMsg.ReadString(this->m_ModifyProfile.m_Password, ARRAYSIZE(this->m_ModifyProfile.m_Password)))
				return false;

			switch (this->m_ModifyProfile.m_Operation)
			{
			case 'add':
				DebugLog(L_INFO, "ACCOUNT_MODIFY_PROFILE_REQ: (ADD) %s", this->m_ModifyProfile.m_Email);
				break;

			case 'del':
				DebugLog(L_INFO, "ACCOUNT_MODIFY_PROFILE_REQ: (DELETE) %u %s", this->m_ModifyProfile.m_ProfileId, this->m_ModifyProfile.m_Email);
				break;

			default:
				assert(false);
				break;
			}
		}
		break;
	}

	return true;
}

bool MMG_AccountProtocol::Query::VerifyProductId()
{
	if (this->m_DistributionId != WIC_CLIENT_DISTRIBUTION)
		return false;

	switch (this->m_ProductId)
	{
	case PRODUCT_ID_WIC07_STANDARD_KEY:
	case PRODUCT_ID_WIC07_TIMELIMITED_KEY:
	case PRODUCT_ID_WIC08_STANDARD_KEY:
	case PRODUCT_ID_WIC08_TIMELIMITED_KEY:
		return true;
	}

	return false;
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
	else if (this->m_MaintenanceMode)
	{
		// Massgate/Server maintenance notice
		DebugLog(L_INFO, "ACCOUNT_NOT_CONNECTED: Sending Maintenance Notice");
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
#ifndef USING_MYSQL_DATABASE
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
					aClient->SetIsPlayer(true);
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
					aClient->GetProfile()->m_ProfileId = 1234;
					//aClient->GetProfile()->m_ClanId = 4321;
					//aClient->GetProfile()->m_RankInClan = 1;

					// Write the profile info stream
					aClient->GetProfile()->ToStream(&cryptMessage);

					MMG_AuthToken *myAuthToken = aClient->GetToken();

					//sync client->authtoken.profileid to client->profile.profileid
					myAuthToken->m_ProfileId = 1234; //myAuthToken->m_ProfileId = aClient->GetProfile()->m_ProfileId;
					myAuthToken->m_AccountId = 69;
					
					// TigerMD5 of ...? (possibly crypt keys)
					/*myAuthToken->m_Hash.m_Hash[0] = 0x558C0A1C;
					myAuthToken->m_Hash.m_Hash[1] = 0xA59C9FCA;
					myAuthToken->m_Hash.m_Hash[2] = 0x6566857D;
					myAuthToken->m_Hash.m_Hash[3] = 0x8A3FF551;
					myAuthToken->m_Hash.m_Hash[4] = 0xB69D17E5;
					myAuthToken->m_Hash.m_Hash[5] = 0xD7BBF74D;*/
					memset(&myAuthToken->m_Hash.m_Hash, 0, 6 * sizeof(ulong));

					myAuthToken->m_Hash.m_HashLength = 6 * sizeof(ulong);
					myAuthToken->m_Hash.m_GeneratedFromHashAlgorithm = HASH_ALGORITHM_TIGER;
					myAuthToken->ToStream(&cryptMessage);// Write the authorization token info stream

					cryptMessage.WriteUInt(WIC_CREDAUTH_RESEND_S);	// periodicityOfCredentialsRequests (How long until the first is sent)
					cryptMessage.WriteUInt(0);						// myLeaseTimeLeft (Limited access key)
					cryptMessage.WriteUInt(45523626);				// myAntiSpoofToken (Random number)
				}
#else		
				//DebugLog(L_INFO, "ACCOUNT_AUTH_ACCOUNT_RSP: Sending login response (id %i)", AccountId);
				DebugLog(L_INFO, "ACCOUNT_AUTH_ACCOUNT_RSP:");
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_AUTH_ACCOUNT_RSP;

				MMG_AuthToken *myAuthToken = aClient->GetToken();
				MMG_Profile *myProfile = aClient->GetProfile();

				//password check should be done by massgate server, not by database
				PasswordHash hasher(8, false);

				char myPasswordHash[WIC_PASSWORDHASH_MAX_LENGTH];
				memset(myPasswordHash, 0, sizeof(myPasswordHash));

				char myPasswordMD5[64];
				memset(myPasswordMD5, 0, sizeof(myPasswordMD5));

				MC_Misc::MD5String(myQuery.m_Authenticate.m_Password, myPasswordMD5);

				uchar isBanned = 0;

				uint myStatusCode = 0;
				uint mySuccessFlag = 0;

				// Query the database
				bool AuthQueryOK = MySQLDatabase::ourInstance->AuthUserAccount(myQuery.m_Authenticate.m_Email, myPasswordHash, &isBanned, myAuthToken);

				// TODO: generate a better authtoken
				myAuthToken->m_TokenId = MC_MTwister().Random();

				//determine if credentials were valid
				if(myAuthToken->m_AccountId == 0 && AuthQueryOK)		//account doesnt exist
				{
					myStatusCode = AuthFailed_NoSuchAccount;
					mySuccessFlag = 0;
				}
				else if(!hasher.CheckPassword(myPasswordMD5, myPasswordHash) && AuthQueryOK)	//wrong password
				{
					myStatusCode = AuthFailed_BadCredentials;
					mySuccessFlag = 0;
				}
				else if(isBanned && AuthQueryOK)						//account has been banned
				{
					myStatusCode = AuthFailed_AccountBanned;
					mySuccessFlag = 0;
				}
				else if(myAuthToken->m_ProfileId == 0 && AuthQueryOK)	//no profiles exist. bring up add profile box
				{
					myStatusCode = AuthFailed_RequestedProfileNotFound;
					mySuccessFlag = 0;
				}
				else if(SvClientManager::ourInstance->AccountInUse(myAuthToken->m_AccountId))
				{
					myStatusCode = AuthFailed_AccountInUse;
					mySuccessFlag = 0;
				}
				else if(!AuthQueryOK)									// something went wrong executing the query
				{
					myStatusCode = AuthFailed_General; //ServerError
					mySuccessFlag = 0;
				}
				else													//should be ok to retrieve a profile
				{
					// TODO insert missing data
					if (myAuthToken->m_AccountId > 0)
					{
						// update geoip info
						char country[WIC_COUNTRY_MAX_LENGTH];
						memset(country, 0, sizeof(country));

						strcpy_s(country, GeoIP::ClientLocateIP(aClient->GetIPAddress()));
						MySQLDatabase::ourInstance->UpdateRealCountry(myAuthToken->m_AccountId, country);

						// update missing sequence number & cipherkeys for handmade accounts
						MySQLDatabase::ourInstance->UpdateCDKeyInfo(myAuthToken->m_AccountId, myQuery.m_EncryptionKeySequenceNumber, myQuery.m_CipherKeys);

						// if current password hash is md5 based, update to blowfish
						if (!strncmp(myPasswordHash, "$H$", 3))
						{
							hasher.HashPassword(myPasswordHash, myPasswordMD5);
							MySQLDatabase::ourInstance->UpdatePassword(myAuthToken->m_AccountId, myPasswordHash);
						}
					}

					bool ProfileQueryOK;

					//AuthFailed_AccountInUse, AuthFailed_ProfileInUse, AuthFailed_CdKeyInUse, AuthFailed_IllegalCDKey(not using)

					//profile selection box was used
					if(myQuery.m_Authenticate.m_UseProfile)
					{
						ProfileQueryOK = MySQLDatabase::ourInstance->QueryUserProfile(myAuthToken->m_AccountId, myQuery.m_Authenticate.m_UseProfile, myProfile);

						//if(myQuery.m_Authenticate.m_UseProfile > 0 && ProfileQueryOK)
						if(ProfileQueryOK)											//ok to login, set active profile
						{
							myAuthToken->m_ProfileId = myProfile->m_ProfileId;
							myStatusCode = AuthSuccess;
							mySuccessFlag = 1;
						}
						else //!ProfileQueryOK										// something went wrong executing the query
						{
							myStatusCode = AuthFailed_General; //ServerError
							mySuccessFlag = 0;
						}
					}

					//login button was used
					else
					{
						//if(myQuery.m_Authenticate.m_HasOldCredentials)
						//myQuery.m_Authenticate.m_Credentials, myQuery.m_Authenticate.m_Profile, myQuery.m_Authenticate.m_UseProfile
						//myAuthToken, myProfile

						ProfileQueryOK = MySQLDatabase::ourInstance->QueryUserProfile(myAuthToken->m_AccountId, myAuthToken->m_ProfileId, myProfile);

						if(myAuthToken->m_ProfileId > 0 && ProfileQueryOK)			// ok to login, set active profile
						{
							myAuthToken->m_ProfileId = myProfile->m_ProfileId;
							myStatusCode = AuthSuccess;
							mySuccessFlag = 1;
						}
						else //!ProfileQueryOK										// something went wrong executing the query
						{
							myStatusCode = AuthFailed_General; //ServerError
							mySuccessFlag = 0;
						}
					}

					//TODO: use SetLoginStatus() + SetTimeout(), we need
					//a way to determine if a profile or account is currently in use
					//auth.myLatestVersion??

					// Update the maximum client timeout
					aClient->SetLoginStatus(true);
					aClient->SetIsPlayer(true);
					aClient->SetTimeout(WIC_LOGGEDIN_NET_TIMEOUT);
				}

				//write response message to the stream;
				cryptMessage.WriteUChar(myStatusCode);
				cryptMessage.WriteUChar(mySuccessFlag);	// auth.mySuccessFlag
				cryptMessage.WriteUShort(0);			// auth.myLatestVersion

				// Write the profile info stream
				myProfile->ToStream(&cryptMessage);

				// TigerMD5 of ...? (possibly crypt keys)
				/*myAuthToken->m_Hash.m_Hash[0] = 0x558C0A1C;
				myAuthToken->m_Hash.m_Hash[1] = 0xA59C9FCA;
				myAuthToken->m_Hash.m_Hash[2] = 0x6566857D;
				myAuthToken->m_Hash.m_Hash[3] = 0x8A3FF551;
				myAuthToken->m_Hash.m_Hash[4] = 0xB69D17E5;
				myAuthToken->m_Hash.m_Hash[5] = 0xD7BBF74D;
				memset(&myAuthToken->m_Hash.m_Hash, 0, 6 * sizeof(ulong));*/

				myAuthToken->m_Hash.m_HashLength = 6 * sizeof(ulong);
				myAuthToken->m_Hash.m_GeneratedFromHashAlgorithm = HASH_ALGORITHM_TIGER;

				// Write the authorization token info stream
				myAuthToken->ToStream(&cryptMessage);

				cryptMessage.WriteUInt(WIC_CREDAUTH_RESEND_S);	// periodicityOfCredentialsRequests (How long until the first is sent)
				cryptMessage.WriteUInt(0);						// myLeaseTimeLeft (Limited access key)
				//cryptMessage.WriteUInt(45523626);				// myAntiSpoofToken (Random number)
				cryptMessage.WriteUInt(myAuthToken->m_TokenId);	// TODO
#endif
			}
			break;

			// Client request to create a new account
			case MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_REQ:
			{
				DebugLog(L_INFO, "ACCOUNT_CREATE_ACCOUNT_RSP:");
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_CREATE_ACCOUNT_RSP;
#ifdef USING_MYSQL_DATABASE
				uint myAccountId=0, myCdkeyId=0;

				uint myStatusCode = 0;
				uint mySuccessFlag = 0;

				bool CheckEmailQueryOK = MySQLDatabase::ourInstance->CheckIfEmailExists(myQuery.m_Create.m_Email, &myAccountId);
				bool CheckCDKeyQueryOK = MySQLDatabase::ourInstance->CheckIfCDKeyExists(myQuery.m_EncryptionKeySequenceNumber, &myCdkeyId);

				if (myAccountId > 0 && CheckEmailQueryOK)			//account exists with that email
				{
					myStatusCode = CreateFailed_EmailExists;
					mySuccessFlag = 0;
				}
				else if(myCdkeyId > 0 && CheckCDKeyQueryOK)		//an account has already been created with this cdkey
				{
					myStatusCode = CreateFailed_CdKeyExhausted;
					mySuccessFlag = 0;
				}
				else if(!CheckEmailQueryOK || !CheckCDKeyQueryOK)
				{
					myStatusCode = CreateFailed_General; //ServerError
					mySuccessFlag = 0;
				}
				else							//should be ok to create account
				{
					char realcountry[WIC_COUNTRY_MAX_LENGTH];
					memset(realcountry, 0, sizeof(realcountry));

					strcpy_s(realcountry, GeoIP::ClientLocateIP(aClient->GetIPAddress()));

					PasswordHash hasher(8, false);

					char myPasswordHash[WIC_PASSWORDHASH_MAX_LENGTH];
					memset(myPasswordHash, 0, sizeof(myPasswordHash));

					char myPasswordMD5[64];
					memset(myPasswordMD5, 0, sizeof(myPasswordMD5));

					MC_Misc::MD5String(myQuery.m_Create.m_Password, myPasswordMD5);
					hasher.HashPassword(myPasswordHash, myPasswordMD5);

					bool CreateQueryOK = MySQLDatabase::ourInstance->CreateUserAccount(myQuery.m_Create.m_Email, myPasswordHash, 
						myQuery.m_Create.m_Country, realcountry, &myQuery.m_Create.m_EmailMeGameRelated, &myQuery.m_Create.m_AcceptsEmail, myQuery.m_EncryptionKeySequenceNumber, myQuery.m_CipherKeys);

					if(CreateQueryOK)			//create user account succeeded
					{
						myStatusCode = CreateSuccess;
						mySuccessFlag = 1;
					}
					else //!CreateQueryOK		// something went wrong executing the query
					{
						myStatusCode = CreateFailed_General; //ServerError
						mySuccessFlag = 0;
					}
				}

				cryptMessage.WriteUChar(myStatusCode);
				cryptMessage.WriteUChar(mySuccessFlag);					// mySuccessFlag
#endif
			}
			break;

			// Prepare (sent before ACCOUNT_CREATE_ACCOUNT_REQ) authorization for cd-key
			case MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_REQ:
			{
				DebugLog(L_INFO, "ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP:");
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_PREPARE_CREATE_ACCOUNT_RSP;

				char country[WIC_COUNTRY_MAX_LENGTH];	// Guessed by IPv4 geolocation information

#ifndef USING_MYSQL_DATABASE
				strcpy_s(country, "US");
#else
				char* countrycode = GeoIP::ClientLocateIP(aClient->GetIPAddress());
				strcpy_s(country, countrycode);
#endif
				cryptMessage.WriteUChar(AuthSuccess);	// Otherwise AuthFailed_CdKeyExpired
				cryptMessage.WriteUChar(1);				// mySuccessFlag
				cryptMessage.WriteString(country);		// yourCountry
			}
			break;

			// Client requests a session update to prevent dropping
			case MMG_ProtocolDelimiters::ACCOUNT_NEW_CREDENTIALS_REQ:
			{
				DebugLog(L_INFO, "ACCOUNT_NEW_CREDENTIALS_RSP:");
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
				DebugLog(L_INFO, "ACCOUNT_RETRIEVE_PROFILES_RSP: getting profiles for %s", myQuery.m_RetrieveProfiles.m_Email);
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP;
#ifndef USING_MYSQL_DATABASE
				cryptMessage.WriteUChar(AuthSuccess);
				cryptMessage.WriteUChar(1);	// mySuccessFlag
				cryptMessage.WriteUInt(1);	// numUserProfiles
				cryptMessage.WriteUInt(1234);	// lastUsedProfileId

				MMG_Profile *myProfile = aClient->GetProfile();
				wcscpy_s(myProfile->m_Name, L"Nukem");
				myProfile->m_ProfileId = 1234;
				//myProfile->m_ClanId = 4321;
				myProfile->m_OnlineStatus = 0;
				myProfile->m_Rank = 18;
				//myProfile->m_RankInClan = 1;

				// Write the profile info stream
				myProfile->ToStream(&cryptMessage);
#else
				MMG_AuthToken *myAuthToken = aClient->GetToken();
				MMG_Profile myProfiles[5];

				PasswordHash hasher(8, false);

				char myPasswordHash[WIC_PASSWORDHASH_MAX_LENGTH];
				memset(myPasswordHash, 0, sizeof(myPasswordHash));

				char myPasswordMD5[64];
				memset(myPasswordMD5, 0, sizeof(myPasswordMD5));

				MC_Misc::MD5String(myQuery.m_RetrieveProfiles.m_Password, myPasswordMD5);

				uchar isBanned = 0;
				ulong myProfileCount = 0;
				uint lastUsedId = 0;

				uint myStatusCode = 0;
				uint mySuccessFlag = 0;

				bool AuthQueryOK = MySQLDatabase::ourInstance->AuthUserAccount(myQuery.m_RetrieveProfiles.m_Email, myPasswordHash, &isBanned, myAuthToken);

				//determine if credentials were valid
				if(myAuthToken->m_AccountId == 0 && AuthQueryOK)		//account doesnt exist
				{
					myStatusCode = AuthFailed_BadCredentials;	//AuthFailed_NoSuchAccount
					mySuccessFlag = 0;
				}
				else if(!hasher.CheckPassword(myPasswordMD5, myPasswordHash) && AuthQueryOK)	//wrong password
				{
					myStatusCode = AuthFailed_BadCredentials;
					mySuccessFlag = 0;
				}
				else if(isBanned && AuthQueryOK)						//account has been banned
				{
					myStatusCode = AuthFailed_AccountBanned;
					mySuccessFlag = 0;
				}
				else if(SvClientManager::ourInstance->AccountInUse(myAuthToken->m_AccountId))
				{
					myStatusCode = AuthFailed_AccountInUse;
					mySuccessFlag = 0;
				}
				else if(!AuthQueryOK)									// something went wrong executing the query
				{
					myStatusCode = AuthFailed_General; //ServerError
					mySuccessFlag = 0;
				}
				else													//should be ok to retrieve profile list
				{
					// TODO insert missing data
					if (myAuthToken->m_AccountId > 0)
					{
						// update geoip info
						char country[WIC_COUNTRY_MAX_LENGTH];
						memset(country, 0, sizeof(country));

						strcpy_s(country, GeoIP::ClientLocateIP(aClient->GetIPAddress()));
						MySQLDatabase::ourInstance->UpdateRealCountry(myAuthToken->m_AccountId, country);

						// update missing sequence number & cipherkeys for handmade accounts
						MySQLDatabase::ourInstance->UpdateCDKeyInfo(myAuthToken->m_AccountId, myQuery.m_EncryptionKeySequenceNumber, myQuery.m_CipherKeys);

						// if current password hash is md5 based, update to blowfish
						if (!strncmp(myPasswordHash, "$H$", 3))
						{
							hasher.HashPassword(myPasswordHash, myPasswordMD5);
							MySQLDatabase::ourInstance->UpdatePassword(myAuthToken->m_AccountId, myPasswordHash);
						}
					}

					bool RetrieveProfilesQueryOK = MySQLDatabase::ourInstance->RetrieveUserProfiles(myAuthToken->m_AccountId, &myProfileCount, myProfiles);

					if(RetrieveProfilesQueryOK)
					{
						myStatusCode = AuthSuccess;
						mySuccessFlag = 1;

						lastUsedId = myProfiles[0].m_ProfileId;	//myAuthToken->m_ProfileId
					}
					else
					{
						myStatusCode = AuthFailed_General; //ServerError
						mySuccessFlag = 0;
						
						myProfileCount = 0;
						lastUsedId = 0;
					}
				}

				cryptMessage.WriteUChar(myStatusCode);
				cryptMessage.WriteUChar(mySuccessFlag);	// mySuccessFlag
				cryptMessage.WriteUInt(myProfileCount);	// numUserProfiles
				cryptMessage.WriteUInt(lastUsedId);	// lastUsedProfileId

				//write profile/s to stream
				for(uint i=0; i < myProfileCount; i++)
					myProfiles[i].ToStream(&cryptMessage);
#endif
			}
			break;

			case MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_REQ:
			{
#ifdef USING_MYSQL_DATABASE
				//DebugLog(L_INFO, "ACCOUNT_MODIFY_PROFILE_RSP: modify profiles");
				//responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_MODIFY_PROFILE_RSP;
				responseDelimiter = MMG_ProtocolDelimiters::ACCOUNT_RETRIEVE_PROFILES_RSP;

				MMG_AuthToken *myAuthToken = aClient->GetToken();
				MMG_Profile myProfiles[5];

				uchar isBanned = 0;
				ulong myProfileCount = 0;
				uint lastUsedId = 0;

				uint myProfileId = 0;

				uint myStatusCode = 0;
				uint mySuccessFlag = 0;

				if (myQuery.m_ModifyProfile.m_Operation == 'add')
				{
					DebugLog(L_INFO, "ACCOUNT_MODIFY_PROFILE_RSP: add new profile for %s", myQuery.m_ModifyProfile.m_Email);
					
					bool CheckProfileQueryOK = MySQLDatabase::ourInstance->CheckIfProfileExists(myQuery.m_ModifyProfile.m_Name, &myProfileId);
					
					if (myProfileId > 0 && CheckProfileQueryOK)			//profile exists with that name
					{
						myStatusCode = ModifyFailed_ProfileNameTaken;
						mySuccessFlag = 1;
					}
					else if (!CheckProfileQueryOK)
					{
						myStatusCode = ModifyFailed_General;		//server / database error
						mySuccessFlag = 1;
					}
					else							//should be ok to create profile
					{
						bool CreateProfileQueryOK = MySQLDatabase::ourInstance->CreateUserProfile(myAuthToken->m_AccountId, myQuery.m_ModifyProfile.m_Name, myQuery.m_ModifyProfile.m_Email);
					
						if(CreateProfileQueryOK)			//create profile success
						{
							myStatusCode = ModifySuccess;
							mySuccessFlag = 1;
						}
						else //!CreateProfileQueryOK		// something went wrong executing the query
						{
							myStatusCode = ModifyFailed_General; //ServerError
							mySuccessFlag = 1;
						}
					}
				}
				else if (myQuery.m_ModifyProfile.m_Operation == 'del')
				{
					DebugLog(L_INFO, "ACCOUNT_MODIFY_PROFILE_RSP: delete profile %d for %s", myQuery.m_ModifyProfile.m_ProfileId, myQuery.m_ModifyProfile.m_Email);

					MMG_Profile myProfile;
					bool ProfileQueryOK = MySQLDatabase::ourInstance->QueryProfileName(myQuery.m_ModifyProfile.m_ProfileId, &myProfile);
					
					if (!ProfileQueryOK)
					{
						myStatusCode = ModifyFailed_General;
						mySuccessFlag = 0;
					}
					else if (ProfileQueryOK && myProfile.m_ClanId > 0)
					{
						myStatusCode = DeleteProfile_Failed_Clan;
						mySuccessFlag = 1;
					}
					else
					{
						bool DeleteProfileQueryOK = MySQLDatabase::ourInstance->DeleteUserProfile(myAuthToken->m_AccountId, myQuery.m_ModifyProfile.m_ProfileId, myQuery.m_ModifyProfile.m_Email);
					
						if(DeleteProfileQueryOK)			//delete profile success
						{
							myStatusCode = ModifySuccess;
							mySuccessFlag = 1;
						}
						else //!DeleteProfileQueryOK		// something went wrong executing the query
						{
							myStatusCode = ModifyFailed_General; //ServerError
							mySuccessFlag = 1;
						}
					}
				}
				else
				{
					DebugLog(L_INFO, "ACCOUNT_MODIFY_PROFILE_RSP: unknown operation");
				}
				
				// retrieve and send profile list
				bool RetrieveProfilesQueryOK = MySQLDatabase::ourInstance->RetrieveUserProfiles(myAuthToken->m_AccountId, &myProfileCount, myProfiles);

				if (RetrieveProfilesQueryOK && mySuccessFlag)
				{
					lastUsedId = myProfiles[0].m_ProfileId;	//myAuthToken->m_ProfileId
				}
				else
				{
					myStatusCode = ModifyFailed_General; //ServerError
					mySuccessFlag = 0;
				
					myProfileCount = 0;
					lastUsedId = 0;
				}
				
				cryptMessage.WriteUChar(myStatusCode);
				cryptMessage.WriteUChar(mySuccessFlag);	// mySuccessFlag
				cryptMessage.WriteUInt(myProfileCount);	// numUserProfiles
				cryptMessage.WriteUInt(lastUsedId);		// lastUsedProfileId

				//write profile/s to stream
				for(uint i=0; i < myProfileCount; i++)
					myProfiles[i].ToStream(&cryptMessage);
#endif
			}
			break;

			default:
				DebugLog(L_WARN, "Unknown delimiter %i", aDelimiter);
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