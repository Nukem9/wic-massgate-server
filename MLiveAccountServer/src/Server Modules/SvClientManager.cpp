#include "../stdafx.h"

SvClient::SvClient()
{
	this->m_Profile		= nullptr;
	this->m_AuthToken	= nullptr;

	this->Reset();
}

SvClient::~SvClient()
{
	this->Reset();
}

void SvClient::UpdateActivity()
{
	InterlockedExchange((volatile LONG *)&this->m_LastActiveTime, MI_Time::GetSystemTime());
}

bool SvClient::IsActive()
{
	return (MI_Time::GetSystemTime() - this->m_LastActiveTime) < this->m_TimeoutTime;
}

bool SvClient::IsLoggedIn()
{
	return this->m_LoggedIn;
}

void SvClient::SetTimeout(uint aTimeout)
{
	InterlockedExchange((volatile LONG *)&this->m_TimeoutTime, aTimeout);
}

void SvClient::SetLoginStatus(bool aStatus)
{
	this->m_LoggedIn = aStatus;
}

bool SvClient::SendData(MN_WriteMessage *aMessage)
{
	return aMessage->SendMe(this->m_Socket);
}

MMG_Profile *SvClient::GetProfile()
{
	return this->m_Profile;
}

MMG_AuthToken *SvClient::GetToken()
{
	return this->m_AuthToken;
}

SOCKET SvClient::GetSocket()
{
	return this->m_Socket;
}

uint SvClient::GetIPAddress()
{
	return this->m_IpAddress;
}

uint SvClient::GetPort()
{
	return this->m_Port;
}

void SvClient::Reset()
{
	// Invalidate to prevent any other lookups
	this->m_Valid = false;
	this->m_Index = 0;

	this->m_LastActiveTime	= 0;
	this->m_TimeoutTime		= 0;

	// Close the socket if it's open
	if(this->m_Socket != INVALID_SOCKET)
	{
		shutdown(this->m_Socket, 2/*SD_BOTH*/);
		closesocket(this->m_Socket);

		this->m_Socket = INVALID_SOCKET;
	}

	this->m_IpAddress	= 0;
	this->m_Port		= 0;

	// Reset encryption information
	this->m_CipherIdentifier			= CIPHER_UNKNOWN;
	this->m_EncryptionKeySequenceNumber = 0;
	memset(this->m_CipherKeys, 0, sizeof(this->m_CipherKeys));

	// Client is no longer logged in
	this->m_LoggedIn = false;

	// Profile structure
	if(this->m_Profile)
	{
		delete this->m_Profile;
		this->m_Profile = nullptr;
	}

	// Authorization token structure
	if(this->m_AuthToken)
	{
		delete this->m_AuthToken;
		this->m_AuthToken = nullptr;
	}
}

bool SvClient::CanReadFrom()
{
	// Timeout time of 10 milliseconds
	static timeval waitd = {0, 10000};

	// Check for an incoming connection
	fd_set readFlags;

	FD_ZERO(&readFlags);
	FD_SET(this->m_Socket, &readFlags);

	// Query the socket
	int result = select(0, &readFlags, nullptr, nullptr, &waitd);

	if(result < 0)
		return false;

	// Check if the socket can be read from
	if(!FD_ISSET(this->m_Socket, &readFlags))
		return false;

	// Clear the read flag
	FD_CLR(this->m_Socket, &readFlags);

	return true;
}

SvClientManager::SvClientManager()
{
	this->m_ThreadHandle			= nullptr;
	this->m_DataReceivedCallback	= nullptr;
	this->m_Clients					= (SvClient *)VirtualAlloc(nullptr, WIC_SERVER_MAX_CLIENTS * sizeof(SvClient), MEM_COMMIT, PAGE_READWRITE);
	this->m_ClientCount				= 0;
	this->m_ClientMaxCount			= WIC_SERVER_MAX_CLIENTS;
}

SvClientManager::~SvClientManager()
{
	if(this->m_ThreadHandle)
	{
		TerminateThread(this->m_ThreadHandle, 0);
		CloseHandle(this->m_ThreadHandle);

		this->m_ThreadHandle = nullptr;
	}

	if(this->m_Clients)
	{
		VirtualFree(this->m_Clients, 0, MEM_RELEASE);
		this->m_Clients = nullptr;
	}
}

bool SvClientManager::Start()
{
	this->m_ThreadHandle = CreateThread(nullptr, 0, MainThread, this, 0, nullptr);

	if(this->m_ThreadHandle == nullptr)
		return false;

	return true;
}

void SvClientManager::SetCallback(pfnDataReceivedCallback aCallback)
{
	this->m_DataReceivedCallback = aCallback;
}

SvClient *SvClientManager::FindClient(sockaddr_in *aAddr)
{
	uint ip		= ntohl(aAddr->sin_addr.s_addr);
	uint port	= ntohs(aAddr->sin_port);

	this->m_Mutex.Lock();

	for(uint i = 0; i < this->m_ClientMaxCount; i++)
	{
		SvClient *client = &this->m_Clients[i];

		if(!client->m_Valid)
			continue;

		// Found a client, compare address
		if(client->m_IpAddress == ip && client->m_Port == port)
		{
			this->m_Mutex.Unlock();
			return client;
		}
	}

	this->m_Mutex.Unlock();

	return nullptr;
}

SvClient *SvClientManager::ConnectClient(SOCKET aSocket, sockaddr_in *aAddr)
{
	return this->AddClient(ntohl(aAddr->sin_addr.s_addr), ntohs(aAddr->sin_port), aSocket);
}

void SvClientManager::DisconnectClient(SvClient *aClient)
{
	if(!aClient->m_Valid)
		return;

	this->RemoveClient(aClient);
}

uint SvClientManager::GetClientCount()
{
	return this->m_ClientCount;
}

SvClient *SvClientManager::AddClient(uint aIpAddr, uint aPort, SOCKET aSocket)
{
	SvClient *slot = nullptr;

	this->m_Mutex.Lock();

	for(uint i = 0; i < this->m_ClientMaxCount; i++)
	{
		SvClient *client = &this->m_Clients[i];

		// Found unused slot
		if(!client->m_Valid)
		{
			client->m_Index = i;
			slot = client;

			break;
		}
	}

	if(slot)
	{
		slot->m_Profile		= new MMG_Profile();
		slot->m_AuthToken	= new MMG_AuthToken();
		slot->m_Socket		= aSocket;
		slot->m_IpAddress	= aIpAddr;
		slot->m_Port		= aPort;

		// Automatically set it to non-blocking
		u_long sockMode = 1;
		ioctlsocket(aSocket, FIONBIO, &sockMode);

		slot->SetTimeout(WIC_DEFAULT_NET_TIMEOUT);
		slot->UpdateActivity();

		slot->m_Valid = true;

		// Update the total server client count
		InterlockedIncrement((volatile LONG *)&this->m_ClientCount);
	}

	this->m_Mutex.Unlock();

	return slot;
}

void SvClientManager::RemoveClient(SvClient *aClient)
{
	this->m_Mutex.Lock();

	// Decrement the total client count
	InterlockedDecrement((volatile LONG *)&this->m_ClientCount);

	// Reset variables
	aClient->Reset();

	this->m_Mutex.Unlock();
}

DWORD WINAPI SvClientManager::MainThread(LPVOID lpArg)
{
	SvClientManager *myManager = (SvClientManager *)lpArg;

	// Data buffer
	char recvBuffer[16384];
	memset(recvBuffer, 0, sizeof(recvBuffer));

	// Loop forever
	for(uint droppedClients = 0;; droppedClients = 0)
	{
		myManager->m_Mutex.Lock();

		for(uint i = 0; i < myManager->m_ClientMaxCount; i++)
		{
			SvClient *client = &myManager->m_Clients[i];

			if(!client->m_Valid)
				continue;

			// Check if the client timed out from the last response
			if(!client->IsActive())
			{
				myManager->RemoveClient(client);
				droppedClients++;

				continue;
			}

			if(!client->CanReadFrom())
				continue;

			// Read the buffer
			int result = recv(client->GetSocket(), recvBuffer, sizeof(recvBuffer), 0);

			// Check if an error occurred or the client disconnected
			bool wasError = (result == SOCKET_ERROR || result == 0);

			if(myManager->m_DataReceivedCallback)
				myManager->m_DataReceivedCallback(client, recvBuffer, result, wasError);
		}

		myManager->m_Mutex.Unlock();

		if(droppedClients > 0)
			DebugLog(L_INFO, "%s(): Dropped %i client(s).", __FUNCTION__, droppedClients);

		// Prevent 100% CPU usage
		Sleep(10);
	}

	return 0;
}