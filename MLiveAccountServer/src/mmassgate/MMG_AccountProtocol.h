#pragma once

CLASS_SINGLE(MMG_AccountProtocol)
{
public:
	enum ActionStatusCodes : int
	{
		NoStatus = 100,
		IncorrectProtocolVersion,
		Authenticating,
		AuthSuccess,
		AuthFailed_General,
		AuthFailed_NoSuchAccount,
		AuthFailed_BadCredentials,
		AuthFailed_AccountBanned,
		AuthFailed_CdKeyExpired,
		ActivateCodeSuccess,
		ActivateCodeFailed_General,
		ActivateCodeFailed_AuthFailed,
		ActivateCodeFailed_InvalidCode,
		ActivateCodeFailed_NoTimeLimit,
		ActivateCodeFailed_AlreadyUpdated,
		Creating,
		CreateSuccess,
		CreateFailed_General,
		CreateFailed_EmailExists,
		CreateFailed_UsernameExists,
		CreateFailed_CdKeyExhausted,
		AuthFailed_IllegalCDKey,
		AuthFailed_CdKeyInUse,
		AuthFailed_AccountInUse,
		AuthFailed_ProfileInUse,
		Modifying,
		ModifySuccess,
		ModifyFailed_General,
		DeleteProfile_Failed_Clan,
		ServerError,
		PasswordReminderSent,
		Pong,
		LogoutSuccess,
		AuthFailed_RequestedProfileNotFound,
		ModifyFailed_ProfileNameTaken,
		MassgateMaintenance,
	};

	class Query
	{
	public:
		class Authenticate
		{
		public:
			MMG_Profile		m_Profile;
			MMG_AuthToken	m_Credentials;

			char			m_Email[64];
			wchar_t			m_Password[16];
			uint			m_UseProfile;
			uint			m_Fingerprint;
			uchar			m_HasOldCredentials;

		public:
			Authenticate() : m_Profile(), m_Credentials()
			{
				memset(this->m_Email, 0, sizeof(this->m_Email));
				memset(this->m_Password, 0, sizeof(this->m_Password));
				this->m_UseProfile			= 0;
				this->m_Fingerprint			= 0;
				this->m_HasOldCredentials	= 0;
			}
		};

		class RetrieveProfiles
		{
		public:
			char			m_Email[64];
			wchar_t			m_Password[16];
			uint			m_Fingerprint;

		public:
			RetrieveProfiles()
			{
				memset(this->m_Email, 0, sizeof(this->m_Email));
				memset(this->m_Password, 0, sizeof(this->m_Password));
				this->m_Fingerprint = 0;
			}
		};

		ActionStatusCodes	m_StatusCode;

		ushort								m_Protocol;
		MMG_ProtocolDelimiters::Delimiter	m_Delimiter;

		CipherIdentifier	m_CipherIdentifier;
		uint				m_EncryptionKeySequenceNumber;
		ulong				m_CipherKeys[4];

		uint				m_RandomKey;
		uint				m_ProductId;
		uint				m_DistributionId;

		Authenticate		m_Authenticate;
		RetrieveProfiles	m_RetrieveProfiles;

	private:

	public:
		Query() : m_Authenticate(), m_RetrieveProfiles()
		{
			this->m_StatusCode					= NoStatus;

			this->m_Protocol					= 0;
			this->m_Delimiter					= MMG_ProtocolDelimiters::ACCOUNT_START;

			this->m_CipherIdentifier			= CIPHER_UNKNOWN;
			this->m_EncryptionKeySequenceNumber = 0;
			memset(this->m_CipherKeys, 0, sizeof(this->m_CipherKeys));

			this->m_RandomKey					= 0;
			this->m_ProductId					= 0;
			this->m_DistributionId				= 0;
		}

		bool FromStream(MMG_ProtocolDelimiters::Delimiter aDelimiter, MN_ReadMessage *aMessage);
		bool VerifyProductId();
	};

private:

public:
	MMG_AccountProtocol();

	bool HandleMessage				(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);
	void WritePatchInformation		(MN_WriteMessage *aMessage, Query *aQuery);
	void WriteMaintenanceInformation(MN_WriteMessage *aMessage);
};