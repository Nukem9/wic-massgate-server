#pragma once

class MN_TcpServer
{
public:
	typedef void (__cdecl * pfnConnectionReceivedCallback)(SOCKET aSocket, sockaddr_in *aAddr);

private:
	long		m_IpAddress;
	uint		m_Port;

	sockaddr_in	m_Addr;
	SOCKET		m_Socket;

	HANDLE		m_ThreadHandle;
	HANDLE		m_ThreadKillEvent;

	pfnConnectionReceivedCallback m_ConnectionReceivedCallback;

public:
	static MN_TcpServer *Create(const char *aIP, uint aPort);
	static MN_TcpServer *Create(long aIP, uint aPort);

	bool		Start			();
	void		Stop			();
	void		SetCallback		(pfnConnectionReceivedCallback aCallback);

	const char *GetLastError	();

private:
	MN_TcpServer(const char *aIP, uint aPort);
	MN_TcpServer(long aIP, uint aPort);

	void		Init			(long aIP, uint aPort);
	bool		Bind			();
	bool		Listen			();
	SOCKET		Accept			(sockaddr_in *aAddr);
	uint		Recieve			(SOCKET aSocket, PVOID aBuffer, uint aBufferLen);

	static DWORD WINAPI ServerThread(LPVOID lpArg);
};