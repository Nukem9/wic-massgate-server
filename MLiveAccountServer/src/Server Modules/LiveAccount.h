#pragma once

void LiveAccount_Startup();
void LiveAccount_Shutdown();
void LiveAccount_ConnectionRecievedCallback(SOCKET aSocket, sockaddr_in *aAddr);
void LiveAccount_DataRecievedCallback(SvClient *aClient, PVOID aData, uint aDataLen, bool aError);
