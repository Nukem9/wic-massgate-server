#include "../stdafx.h"

#define StatsLog(format, ...) DebugLog(L_INFO, "[Stats]: "format, __VA_ARGS__)

MN_TcpServer *g_StatsServer = nullptr;

void StatsService_Startup()
{
	if(!g_StatsServer)
		g_StatsServer = MN_TcpServer::Create("127.0.0.1", WIC_STATS_PORT);

//	g_StatsServer->SetCallback(StatsService_DataRecievedCallback);
	
	if(!g_StatsServer->Start())
		StatsLog("Error: Failed to start StatsService sever module");
	else
		StatsLog("Service started.");
}

void StatsService_Shutdown()
{
	if(g_StatsServer)
		g_StatsServer->Stop();
}

void StatsService_DataRecievedCallback(SOCKET aSocket, sockaddr_in *aAddr, PVOID aData, uint aDataLen)
{
	SvClient *myClient = SvClientManager::ourInstance->FindClient(aAddr->sin_addr.s_addr);

	if(!myClient)
	{
		StatsLog("Error: Unable to find client");
		closesocket(aSocket);

		return;
	}

	if(!myClient->IsLoggedIn())
	{
		StatsLog("Error: Client is not logged in");
		return;
	}

	myClient->UpdateActivity();

	StatsLog("Got a message");
}