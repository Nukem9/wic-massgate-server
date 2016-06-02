#include "../stdafx.h"

#define LiveAccLog(format, ...) DebugLog(L_INFO, "[LiveAcc]: "format, __VA_ARGS__)

MN_TcpServer *g_LiveAccountServer = nullptr;

void LiveAccount_Startup()
{
	if (!g_LiveAccountServer)
		g_LiveAccountServer = MN_TcpServer::Create("127.0.0.1", WIC_LIVEACCOUNT_PORT);

	// Callback for when a client requests a connection
	g_LiveAccountServer->SetCallback(LiveAccount_ConnectionRecievedCallback);

	// Callback for when data is received
	SvClientManager::ourInstance->SetCallback(LiveAccount_DataRecievedCallback);
	
	if (!g_LiveAccountServer->Start())
		DebugLog(L_ERROR, "[LiveAcc]: Failed to start server module");
	else
		LiveAccLog("Service started");
}

void LiveAccount_Shutdown()
{
	if (g_LiveAccountServer)
		g_LiveAccountServer->Stop();

	g_LiveAccountServer = nullptr;
}

void LiveAccount_ConnectionRecievedCallback(SOCKET aSocket, sockaddr_in *aAddr)
{
	// Find client by IP address
	auto myClient = SvClientManager::ourInstance->FindClient(aAddr);

	// IP wasn't found, add it
	if (!myClient)
	{
		LiveAccLog("Connecting a client on %s:%d....", inet_ntoa(aAddr->sin_addr), ntohs(aAddr->sin_port));

		if (!SvClientManager::ourInstance->ConnectClient(aSocket, aAddr))
			DebugLog(L_ERROR, "[LiveAcc]: Failed to connect client");
	}
}

void LiveAccount_DataRecievedCallback(SvClient *aClient, voidptr_t aData, sizeptr_t aDataLen, bool aError)
{
	// Check if the client should be disconnected
	if (aError)
	{
		in_addr addr;
		addr.s_addr = ntohl(aClient->GetIPAddress());

		if (aClient->IsLoggedIn() && aClient->IsPlayer())
		{
			// TODO: Notify MMG_AccountProxy
			MMG_AccountProxy::ourInstance->SetClientOffline(aClient);
		}

		if (aClient->IsServer())
		{
			// TODO: Notify MMG_ServerTracker
			// temporary
			MMG_TrackableServer::ourInstance->m_ServerList.erase(aClient->GetIPAddress() ^ (aClient->GetPort() + 0x1010101));
		}

		LiveAccLog("Disconnecting client on %s...", inet_ntoa(addr));
		SvClientManager::ourInstance->DisconnectClient(aClient);

		return;
	}
	
#ifdef USING_MYSQL_DATABASE
	// check for database
	if (!MySQLDatabase::ourInstance->HasConnection())
	{
		// set maintenance mode
		printf("\n");
		DebugLog(L_WARN, "[CRITICAL] Check the MySQL server.");
		printf("\n");
		MMG_AccountProtocol::ourInstance->m_MaintenanceMode = true;
	}
#endif

	MN_ReadMessage message(8192);
	if (!message.BuildMessage(aData, aDataLen))
		return;

	while (true)
	{
		MMG_ProtocolDelimiters::Delimiter delimiter;
		if (!message.ReadDelimiter((ushort &)delimiter))
			break;

		if (!LiveAccount_ConsumeMessage(aClient, &message, delimiter))
			break;

		if (!message.DeriveMessage())
			break;
	}
}

bool LiveAccount_ConsumeMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter)
{
	//
	// Each time a new request is sent, update the timeout limit
	//
	aClient->UpdateActivity();

	//
	// Log delimiter types to console
	//
	auto myType = MMG_ProtocolDelimiters::GetType(aDelimiter);

	LiveAccLog("Message from client: delimiter %i - type: %i", aDelimiter, myType);

	//
	// Handle all delimiter types
	//
	switch (myType)
	{
	// Account
	case MMG_ProtocolDelimiters::DELIM_ACCOUNT:
	{
		if (!MMG_AccountProtocol::ourInstance->HandleMessage(aClient, aMessage, aDelimiter))
		{
			LiveAccLog("MMG_AccountProtocol: Failed HandleMessage()");
			return false;
		}
	}
	break;

	// Messaging
	case MMG_ProtocolDelimiters::DELIM_MESSAGE:
	{
		if (!aClient->IsLoggedIn())
		{
			LiveAccLog("MMG_Messaging: User not logged in");
			return false;
		}

		if (!MMG_Messaging::ourInstance->HandleMessage(aClient, aMessage, aDelimiter))
		{
			LiveAccLog("MMG_Messaging: Failed HandleMessage()");
			return false;
		}
	}
	break;

	// Server Tracker (Server list queries)
	case MMG_ProtocolDelimiters::DELIM_SERVERTRACKER_USER:
	{
		if (!MMG_ServerTracker::ourInstance->HandleMessage(aClient, aMessage, aDelimiter))
		{
			LiveAccLog("MMG_ServerTracker: Failed HandleMessage()");
			return false;
		}
	}
	break;

	// Server Tracker (Dedicated server manager)
	case MMG_ProtocolDelimiters::DELIM_SERVERTRACKER_SERVER:
	{
		if (!MMG_TrackableServer::ourInstance->HandleMessage(aClient, aMessage, aDelimiter))
		{
			LiveAccLog("MMG_TrackableServer: Failed HandleMessage()");
			return false;
		}
	}
	break;

	// Chat
	case MMG_ProtocolDelimiters::DELIM_CHAT:
	{
		if (!MMG_Chat::ourInstance->HandleMessage(aClient, aMessage, aDelimiter))
		{
			LiveAccLog("MMG_Chat: Failed HandleMessage()");
			return false;
		}
	}
	break;
	}

	return true;
}