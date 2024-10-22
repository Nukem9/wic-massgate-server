#include "stdafx.h"

void Winsock_Startup()
{
	WSADATA wsaData;

	int e_result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (e_result != 0)
		DebugLog(L_ERROR, "WSAStartup error: %s", WSAErrorToString(e_result));
}

void Winsock_Shutdown()
{
	int e_result = WSACleanup();

	if (e_result != 0)
		DebugLog(L_ERROR, "WSACleanup error: %s", WSAErrorToString(e_result));
}

void CreateServices()
{
	// Server Client Manager
	SvClientManager::Create();

#ifdef USING_MYSQL_DATABASE
	MySQLDatabase::Create();
#endif

	MMG_AccountProtocol::Create();
	MMG_AccountProxy::Create();
	//MMG_InstantMessageListener::Create();
	//MMG_LadderProtocol::Create();
	MMG_Messaging::Create();
	MMG_OptionalContentProtocol::Create();
	//MMG_ProfileEditableVariablesProtocol::Create();
	MMG_ServerTracker::Create();
	MMG_TrackableServer::Create();
	MMG_Chat::Create();
}

void Startup()
{
	CreateServices();

#ifdef USING_MYSQL_DATABASE
	if (!MySQLDatabase::ourInstance->Initialize())
		DebugLog(L_ERROR, "Failed to initialize database back end");

	/* GeoIP init*/
	if (!GeoIP::Initialize())
		DebugLog(L_ERROR, "Failed to initialize GeoIP database");
#endif

	Winsock_Startup();
	HTTPService_Startup();
	LiveAccount_Startup();
	//StatsService_Startup();

	if (!SvClientManager::ourInstance->Start())
		DebugLog(L_ERROR, "Failed to start Server Client Manager");
}

void Shutdown()
{
	//StatsService_Shutdown();
	LiveAccount_Shutdown();
	HTTPService_Shutdown();
	Winsock_Shutdown();

#ifdef USING_MYSQL_DATABASE
	GeoIP::Unload();
	MySQLDatabase::ourInstance->Unload();
#endif
}

int main(int argc, char **argv)
{
	printf("MassgateServer (%s) startup\n", SERVER_ARCHITECTURE);

	Startup();

	for(;;)
	{
		char buffer[64];
		sprintf_s(buffer, "MassgateServer: %i client(s) connected", SvClientManager::ourInstance->GetClientCount());

		SetConsoleTitle(buffer);

		Sleep(1000);
	}
	
	// Never reached as of this time
	Shutdown();

	return 0;
}