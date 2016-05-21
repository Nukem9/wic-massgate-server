#include "../stdafx.h"

MMG_AccountProxy::MMG_AccountProxy()
{
}

bool MMG_AccountProxy::SetClientOffline(SvClient *aClient)
{
	DebugLog(L_INFO, "MMG_AccountProxy::SetClientOffLine()");

	MN_WriteMessage	responseMessage(2048);

	MMG_Profile *myProfile = aClient->GetProfile();

	myProfile->m_OnlineStatus = 0;

#ifdef USING_MYSQL_DATABASE
	MySQLDatabase::ourInstance->SetStatusOffline(myProfile->m_ProfileId);
#endif

	responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	myProfile->ToStream(&responseMessage);

	SvClientManager::ourInstance->SendDataAll(&responseMessage, myProfile->m_ProfileId);

	return true;
}

bool MMG_AccountProxy::SetClientOnline(SvClient *aClient)
{
	DebugLog(L_INFO, "MMG_AccountProxy::SetClientOnline()");

	MN_WriteMessage	responseMessage(2048);

	MMG_Profile *myProfile = aClient->GetProfile();

	myProfile->m_OnlineStatus = 1;

#ifdef USING_MYSQL_DATABASE
	MySQLDatabase::ourInstance->SetStatusOnline(myProfile->m_ProfileId);
#endif

	responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	myProfile->ToStream(&responseMessage);

	SvClientManager::ourInstance->SendDataAll(&responseMessage, myProfile->m_ProfileId);

	return true;
}

bool MMG_AccountProxy::SetClientPlaying(SvClient *aClient)
{
	DebugLog(L_INFO, "MMG_AccountProxy::SetClientPlaying()");

	MN_WriteMessage	responseMessage(2048);

	MMG_Profile *myProfile = aClient->GetProfile();

	myProfile->m_OnlineStatus = 2;

#ifdef USING_MYSQL_DATABASE
	MySQLDatabase::ourInstance->SetStatusPlaying(myProfile->m_ProfileId);
#endif

	responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	myProfile->ToStream(&responseMessage);

	SvClientManager::ourInstance->SendDataAll(&responseMessage, myProfile->m_ProfileId);

	return true;
}