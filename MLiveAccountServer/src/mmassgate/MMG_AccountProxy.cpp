#include "../stdafx.h"

MMG_AccountProxy::MMG_AccountProxy()
{
}

bool MMG_AccountProxy::SetClientOffline(SvClient *aClient)
{
	DebugLog(L_INFO, "MMG_AccountProxy::SetClientOffLine()");

	MN_WriteMessage	responseMessage(2048);

	MMG_Profile *myProfile = aClient->GetProfile();

	myProfile->m_OnlineStatus = WIC_PROFILE_STATUS_OFFLINE;

#ifdef USING_MYSQL_DATABASE
	// TODO: remove m_OnlineStatus from database
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

	myProfile->m_OnlineStatus = WIC_PROFILE_STATUS_ONLINE;

#ifdef USING_MYSQL_DATABASE
	// TODO: remove m_OnlineStatus from database
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

	myProfile->m_OnlineStatus = WIC_PROFILE_STATUS_PLAYING;

#ifdef USING_MYSQL_DATABASE
	// TODO: remove m_OnlineStatus from database
	MySQLDatabase::ourInstance->SetStatusPlaying(myProfile->m_ProfileId);
#endif

	responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
	myProfile->ToStream(&responseMessage);

	SvClientManager::ourInstance->SendDataAll(&responseMessage, myProfile->m_ProfileId);

	return true;
}