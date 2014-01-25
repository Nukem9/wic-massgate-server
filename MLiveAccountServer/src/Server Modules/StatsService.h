#pragma once

void StatsService_Startup();
void StatsService_Shutdown();
void StatsService_ConnectionRecievedCallback(SOCKET aSocket, sockaddr_in *aAddr);