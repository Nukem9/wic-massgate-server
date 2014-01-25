#include "stdafx.h"

void Winsock_Startup()
{
	WSADATA wsaData;

	int e_result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if(e_result != 0)
		DebugLog(L_ERROR, "WSAStartup error: %s", WSAErrorToString(e_result));
}

void Winsock_Shutdown()
{
	int e_result = WSACleanup();

	if(e_result != 0)
		DebugLog(L_ERROR, "WSACleanup error: %s", WSAErrorToString(e_result));
}

void CreateServices()
{
	// Server Client Manager
	SvClientManager::Create();

	MMG_AccountProtocol::Create();
	MMG_Messaging::Create();
	MMG_OptionalContentProtocol::Create();
	MMG_ServerTracker::Create();
}

void Startup()
{
	CreateServices();

	if(!Database::Initialize())
		DebugLog(L_ERROR, "Failed to initialize database back end");

	Database::InitializeTables();
	Database::CreateUserAccount("TestUser@example.com", L"pass");
	Database::CreateUserAccount("TestUser1@example.com", L"pass");
	Database::CreateUserAccount("TestUser2@example.com", L"pass");
	Database::CreateUserAccount("TestUser3@example.com", L"pass");

	Winsock_Startup();
	HTTPService_Startup();
	LiveAccount_Startup();
	//StatsService_Startup();

	if(!SvClientManager::ourInstance->Start())
		DebugLog(L_ERROR, "Failed to start Server Client Manager");
}

void Shutdown()
{
	//StatsService_Shutdown();
	LiveAccount_Shutdown();
	HTTPService_Shutdown();
	Winsock_Shutdown();

	Database::Unload();
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