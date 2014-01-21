#pragma once

void StatsService_Startup();
void StatsService_Shutdown();
void StatsService_DataRecievedCallback(SOCKET aSocket, sockaddr_in *aAddr, PVOID aData, uint aDataLen);