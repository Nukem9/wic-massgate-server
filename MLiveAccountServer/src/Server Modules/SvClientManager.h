#pragma once

class SvClient
{
public:
	friend class SvClientManager;

	CipherIdentifier	m_CipherIdentifier;
	uint				m_EncryptionKeySequenceNumber;
	ulong				m_CipherKeys[4];

private:
	bool				m_Valid;
	uint				m_Index;

	uint				m_LastActiveTime;
	uint				m_TimeoutTime;
	bool				m_LoggedIn;
	bool				m_IsPlayer;
	bool				m_IsServer;

	SOCKET				m_Socket;
	uint				m_IpAddress;
	uint				m_Port;

	MMG_Profile			*m_Profile;
	MMG_AuthToken		*m_AuthToken;
	MMG_Options			*m_Options;

public:
	SvClient();
	~SvClient();

	void			UpdateActivity	();
	bool			IsActive		();
	bool			IsLoggedIn		();
	bool			IsPlayer		();
	bool			IsServer		();

	void			SetTimeout		(uint aTimeout);
	void			SetSocket		(SOCKET aSocket);
	void			SetLoginStatus	(bool aStatus);
	void			SetIsPlayer		(bool aIsPlayer);
	void			SetIsServer		(bool aIsServer);

	bool			SendData		(MN_WriteMessage *aMessage);

	MMG_Profile		*GetProfile		();
	MMG_AuthToken	*GetToken		();
	MMG_Options		*GetOptions		();

	SOCKET			GetSocket		();
	uint			GetIPAddress	();
	uint			GetPort			();

	void			Reset			();
	bool			CanReadFrom		(bool *ErrorResult = nullptr);

private:
};

CLASS_SINGLE(SvClientManager)
{
public:
	typedef void (__cdecl * pfnDisconnectCallback)(SvClient *aClient);
	typedef void (__cdecl * pfnDataReceivedCallback)(SvClient *aClient, voidptr_t aData, sizeptr_t aDataLen);

private:
	MT_Mutex	m_Mutex;

	HANDLE		m_ThreadHandle;

	pfnDisconnectCallback	m_DisconnectCallback;
	pfnDataReceivedCallback m_DataReceivedCallback;

	SvClient	*m_Clients;
	uint		m_ClientCount;
	uint		m_ClientMaxCount;

public:
	SvClientManager();
	~SvClientManager();

	bool		Start			();

	void		SetDisconnectCallback(pfnDisconnectCallback aCallback);
	void		SetDataCallback	(pfnDataReceivedCallback aCallback);

	SvClient	*FindClient		(sockaddr_in *aAddr);
	SvClient	*FindPlayerByProfileId	(uint profileId);
	bool		AccountInUse	(uint accountId);
	bool		ProfileInUse	(uint profileId);

	SvClient	*ConnectClient	(SOCKET aSocket, sockaddr_in *aAddr);
	void		DisconnectClient(SvClient *aClient);

	void		EmergencyDisconnectAll();
	bool		SendMessageToOnlinePlayers(MN_WriteMessage *aMessage);

	uint		GetClientCount	();

private:
	SvClient	*AddClient		(uint aIpAddr, uint aPort, SOCKET aSocket);
	void		RemoveClient	(SvClient *aClient);

	static DWORD WINAPI MainThread(LPVOID lpArg);
};