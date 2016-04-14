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
		SvClientManager::ourInstance->DisconnectClient(aClient);

		in_addr addr;
		addr.s_addr = aClient->GetIPAddress();

		// TODO: Notify MMG_AccountProxy
		// TODO: Notify MMG_ServerTracker

		LiveAccLog("Disconnecting client on %s...", inet_ntoa(addr));
		return;
	}

	MN_ReadMessage message(8192);
	if (!message.BuildMessage(aData, aDataLen))
		return;

	MMG_ProtocolDelimiters::Delimiter delimiter;
	if (!message.ReadDelimiter((ushort &)delimiter))
		return;

	auto myType = MMG_ProtocolDelimiters::GetType(delimiter);

	LiveAccLog("Message from client: delimiter %i - type: %i", delimiter, myType);

	// Each time a new request is sent, update the timeout limit
	aClient->UpdateActivity();

	switch(myType)
	{
		// Account
		case MMG_ProtocolDelimiters::DELIM_ACCOUNT:
		{
			if (!MMG_AccountProtocol::ourInstance->HandleMessage(aClient, &message, delimiter))
			{
				LiveAccLog("MMG_AccountProtocol: Failed HandleMessage()");
				return;
			}
		}
		break;

		// Message
		case MMG_ProtocolDelimiters::DELIM_MESSAGE:
		{
			if (!aClient->IsLoggedIn())
			{
				LiveAccLog("MMG_Messaging: User not logged in");
				return;
			}

			if (!MMG_Messaging::ourInstance->HandleMessage(aClient, &message, delimiter))
			{
				LiveAccLog("MMG_Messaging: Failed HandleMessage()");
				return;
			}
		}
		break;

		// Dedicated Server
		case MMG_ProtocolDelimiters::DELIM_MESSAGE_DS:
		{
			if (!MMG_Messaging::ourInstance->HandleMessage(aClient, &message, delimiter))
			{
				LiveAccLog("MMG_Messaging: Failed HandleMessage()");
				return;
			}
		}
		break;

		// Server Tracker (Server list queries)
		case MMG_ProtocolDelimiters::DELIM_SERVERTRACKER_USER:
		{
			if (!MMG_ServerTracker::ourInstance->HandleMessage(aClient, &message, delimiter))
			{
				LiveAccLog("MMG_ServerTracker: Failed HandleMessage()");
				return;
			}
		}
		break;

		// Server Tracker (Dedicated server manager)
		case MMG_ProtocolDelimiters::DELIM_SERVERTRACKER_SERVER:
		{
			if (!MMG_TrackableServer::ourInstance->HandleMessage(aClient, &message, delimiter))
			{
				LiveAccLog("MMG_TrackableServer: Failed HandleMessage()");
				return;
			}
		}
		break;

		// Chat
		case MMG_ProtocolDelimiters::DELIM_CHAT:
		{
			if (!MMG_Chat::ourInstance->HandleMessage(aClient, &message, delimiter))
			{
				LiveAccLog("MMG_Chat: Failed HandleMessage()");
				return;
			}
		}
		break;
	}
}