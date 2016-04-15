#pragma once

void LiveAccount_Startup();
void LiveAccount_Shutdown();
void LiveAccount_ConnectionRecievedCallback(SOCKET aSocket, sockaddr_in *aAddr);
void LiveAccount_DataRecievedCallback(SvClient *aClient, voidptr_t aData, sizeptr_t aDataLen, bool aError);
bool LiveAccount_ConsumeMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);