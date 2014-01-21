#include "../stdafx.h"

SvClient::SvClient()
{
	this->m_Profile = nullptr;
	this->m_AuthToken = nullptr;

	this->Reset();
}

SvClient::~SvClient()
{
	this->Reset();
}

void SvClient::UpdateActivity()
{
	InterlockedExchange((LONG *)&this->m_LastActiveTime, MI_Time::GetSystemTime());
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
	InterlockedExchange((LONG *)&this->m_TimeoutTime, aTimeout);
}

void SvClient::SetLoginStatus(bool aStatus)
{
	this->m_LoggedIn = aStatus;
}

MMG_Profile *SvClient::GetProfile()
{
	return this->m_Profile;
}

MMG_AuthToken *SvClient::GetToken()
{
	return this->m_AuthToken;
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

	this->m_IpAddress = 0;

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

SOCKET SvClient::GetSocket()
{
	return this->m_Socket;
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
		VirtualFree(this->m_Clients, 0, MEM_FREE);
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

SvClient *SvClientManager::FindClient(uint aIpAddr)
{
	this->m_Mutex.Lock();
	for(uint i = 0; i < this->m_ClientMaxCount; i++)
	{
		SvClient *client = &this->m_Clients[i];

		if(!client->m_Valid)
			continue;

		// Found a client, compare address
		if(client->m_IpAddress == aIpAddr)
		{
			this->m_Mutex.Unlock();
			return client;
		}
	}
	this->m_Mutex.Unlock();

	return nullptr;
}

SvClient *SvClientManager::ConnectClient(uint aIpAddr, SOCKET aSocket)
{
	SvClient *client = this->AddClient(aIpAddr, aSocket);

	if(client)
	{
		client->m_Profile	= new MMG_Profile();
		client->m_AuthToken	= new MMG_AuthToken();
		client->m_Socket	= aSocket;

		client->m_IpAddress = aIpAddr;

		client->SetTimeout(WIC_DEFAULT_NET_TIMEOUT);
		client->UpdateActivity();

		client->m_Valid = true;
	}

	return client;
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

SvClient *SvClientManager::AddClient(uint aIpAddr, SOCKET aSocket)
{
	this->m_Mutex.Lock();
	for(uint i = 0; i < this->m_ClientMaxCount; i++)
	{
		SvClient *client = &this->m_Clients[i];

		// Found unused slot
		if(!client->m_Valid)
		{
			client->m_Index = i;

			// Update the total server client count
			InterlockedIncrement((LONG *)&this->m_ClientCount);

			this->m_Mutex.Unlock();
			return client;
		}
	}
	this->m_Mutex.Unlock();

	return nullptr;
}

void SvClientManager::RemoveClient(SvClient *aClient)
{
	this->m_Mutex.Lock();

	// Decrement the total client count
	InterlockedDecrement((LONG *)&this->m_ClientCount);

	// Reset variables
	aClient->Reset();

	this->m_Mutex.Unlock();
}

DWORD WINAPI SvClientManager::MainThread(LPVOID lpArg)
{
	SvClientManager *myManager = (SvClientManager *)lpArg;

	char recvBuffer[16384];
	memset(recvBuffer, 0, sizeof(recvBuffer));

	for(uint myDroppedClients = 0;; myDroppedClients = 0)
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
				myDroppedClients++;

				continue;
			}

			int result = recv(client->GetSocket(), recvBuffer, sizeof(recvBuffer), 0);

			if(result == 0 || result == SOCKET_ERROR)
				continue;

			if(myManager->m_DataReceivedCallback)
				myManager->m_DataReceivedCallback(client, recvBuffer, result);
		}
		myManager->m_Mutex.Unlock();

		if(myDroppedClients > 0)
			DebugLog(L_INFO, "%s(): Dropped %i client(s).", __FUNCTION__, myDroppedClients);

		// Prevent 100% CPU usage
		Sleep(10);
	}

	return 0;
}