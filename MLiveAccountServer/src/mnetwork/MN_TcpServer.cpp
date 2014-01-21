#include "../stdafx.h"

MN_TcpServer *MN_TcpServer::Create(const char *aIP, uint aPort)
{
	return new MN_TcpServer(aIP, aPort);
}

MN_TcpServer *MN_TcpServer::Create(long aIP, uint aPort)
{
	return new MN_TcpServer(aIP, aPort);
}

bool MN_TcpServer::Start()
{
	this->m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(this->m_Socket == INVALID_SOCKET)
		return false;

	if(!this->Bind() || !this->Listen())
	{
		closesocket(this->m_Socket);
		return false;
	}

	this->m_ThreadHandle = CreateThread(nullptr, 0, ServerThread, this, 0, nullptr);

	if(this->m_ThreadHandle == nullptr)
	{
		this->Stop();
		return false;
	}

	return true;
}

void MN_TcpServer::Stop()
{
	if(this->m_ThreadHandle)
	{
		TerminateThread(this->m_ThreadHandle, 0);
		CloseHandle(this->m_ThreadHandle);

		this->m_ThreadHandle = nullptr;
	}

	if(this->m_Socket)
	{
		shutdown(this->m_Socket, 2 /*SD_BOTH*/);
		closesocket(this->m_Socket);

		this->m_Socket = INVALID_SOCKET;
	}
}

void MN_TcpServer::SetCallback(pfnConnectionReceivedCallback aCallback)
{
	this->m_ConnectionReceivedCallback = aCallback;
}

const char *MN_TcpServer::GetLastError()
{
	return WSAErrorToString(WSAGetLastError());
}

MN_TcpServer::MN_TcpServer(const char *aIP, uint aPort)
{
	this->Init(inet_addr(aIP), aPort);
}

MN_TcpServer::MN_TcpServer(long aIP, uint aPort)
{
	this->Init(aIP, aPort);
}

void MN_TcpServer::Init(long aIP, uint aPort)
{
	this->m_IpAddress	= aIP;
	this->m_Port		= aPort;

	memset(&this->m_Addr, 0, sizeof(this->m_Addr));
	this->m_Addr.sin_family			= AF_INET;
	this->m_Addr.sin_addr.s_addr	= aIP;
	this->m_Addr.sin_port			= htons(aPort);
	this->m_Socket					= INVALID_SOCKET;

	this->m_ThreadHandle				= nullptr;
	this->m_ConnectionReceivedCallback	= nullptr;
}

bool MN_TcpServer::Bind()
{
	return bind(this->m_Socket, (SOCKADDR *)&this->m_Addr, sizeof(this->m_Addr)) != SOCKET_ERROR;
}

bool MN_TcpServer::Listen()
{
	return listen(this->m_Socket, SOMAXCONN) != SOCKET_ERROR;
}

SOCKET MN_TcpServer::Accept(sockaddr_in *aAddr)
{
	int addrLength = sizeof(sockaddr_in);
	memset(aAddr, 0, addrLength);

	return accept(this->m_Socket, (SOCKADDR *)aAddr, &addrLength);
}

uint MN_TcpServer::Recieve(SOCKET aSocket, PVOID aBuffer, uint aBufferLen)
{
	return recv(aSocket, (char *)aBuffer, aBufferLen, 0);
}

DWORD WINAPI MN_TcpServer::ServerThread(LPVOID lpArg)
{
	MN_TcpServer *myTcpServer = (MN_TcpServer *)lpArg;

	sockaddr_in clAddr;
	SOCKET		clSocket;

	for(;;)
	{
		clSocket = myTcpServer->Accept(&clAddr);

		if(clSocket == INVALID_SOCKET)
		{
			DebugLog(L_ERROR, "Error: %s", myTcpServer->GetLastError());
			continue;
		}

		if(myTcpServer->m_ConnectionReceivedCallback)
			myTcpServer->m_ConnectionReceivedCallback(clSocket, &clAddr);
	}

	return 0;
}