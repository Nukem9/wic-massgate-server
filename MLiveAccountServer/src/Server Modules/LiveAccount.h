#pragma once

void LiveAccount_Startup();
void LiveAccount_Shutdown();
void LiveAccount_ConnectionReceivedCallback(SOCKET aSocket, sockaddr_in *aAddr);
void LiveAccount_DisconnectReceivedCallback(SvClient *aClient);
void LiveAccount_DataReceivedCallback(SvClient *aClient, voidptr_t aData, sizeptr_t aDataLen);
bool LiveAccount_ConsumeMessage(SvClient *aClient, MN_ReadMessage *aMessage, MMG_ProtocolDelimiters::Delimiter aDelimiter);