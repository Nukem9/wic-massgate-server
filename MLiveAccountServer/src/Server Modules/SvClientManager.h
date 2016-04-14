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

public:
	SvClient();
	~SvClient();

	void			UpdateActivity	();
	bool			IsActive		();
	bool			IsLoggedIn		();

	void			SetTimeout		(uint aTimeout);
	void			SetSocket		(SOCKET aSocket);
	void			SetLoginStatus	(bool aStatus);
	void			SetIsPlayer		(bool aIsPlayer);
	void			SetIsServer		(bool aIsServer);

	bool			SendData		(MN_WriteMessage *aMessage);

	MMG_Profile		*GetProfile		();
	MMG_AuthToken	*GetToken		();

	SOCKET			GetSocket		();
	uint			GetIPAddress	();
	uint			GetPort			();

	void			Reset			();
	bool			CanReadFrom		();

private:
};

CLASS_SINGLE(SvClientManager)
{
public:
	typedef void (__cdecl * pfnDataReceivedCallback)(SvClient *aClient, voidptr_t aData, sizeptr_t aDataLen, bool aError);

private:
	MT_Mutex	m_Mutex;

	HANDLE		m_ThreadHandle;

	pfnDataReceivedCallback m_DataReceivedCallback;

	SvClient	*m_Clients;
	uint		m_ClientCount;
	uint		m_ClientMaxCount;

public:
	SvClientManager();
	~SvClientManager();

	bool		Start			();
	void		SetCallback		(pfnDataReceivedCallback aCallback);

	SvClient	*FindClient		(sockaddr_in *aAddr);

	SvClient	*ConnectClient	(SOCKET aSocket, sockaddr_in *aAddr);
	void		DisconnectClient(SvClient *aClient);

	uint		GetClientCount	();

private:
	SvClient	*AddClient		(uint aIpAddr, uint aPort, SOCKET aSocket);
	void		RemoveClient	(SvClient *aClient);

	static DWORD WINAPI MainThread(LPVOID lpArg);
};