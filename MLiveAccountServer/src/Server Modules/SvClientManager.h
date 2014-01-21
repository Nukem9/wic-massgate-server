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

	SOCKET				m_Socket;
	uint				m_IpAddress;

	bool				m_LoggedIn;
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

	MMG_Profile		*GetProfile		();
	MMG_AuthToken	*GetToken		();
	SOCKET			GetSocket		();

	void			Reset			();

private:
};

CLASS_SINGLE(SvClientManager)
{
public:
	typedef void (__cdecl * pfnDataReceivedCallback)(SvClient *aClient, PVOID aData, uint aDataLen);

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

	SvClient	*FindClient		(uint aIpAddr);
	SvClient	*ConnectClient	(uint aIpAddr, SOCKET aSocket);
	void		DisconnectClient(SvClient *aClient);

	uint		GetClientCount	();

private:
	SvClient	*AddClient		(uint aIpAddr, SOCKET aSocket);
	void		RemoveClient	(SvClient *aClient);

	static DWORD WINAPI MainThread(LPVOID lpArg);
};