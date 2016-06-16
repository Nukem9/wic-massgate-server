#include "../stdafx.h"

MMG_AccountProxy::MMG_AccountProxy()
{
}

bool MMG_AccountProxy::SetClientOffline(SvClient *aClient)
{
	//DebugLog(L_INFO, "MMG_AccountProxy::SetClientOffLine(%ws)", aClient->GetProfile()->m_Name);

	if (!this->SetProfileOnlineStatus(aClient, WIC_PROFILE_STATUS_OFFLINE))
		return false;

	return true;
}

bool MMG_AccountProxy::SetClientOnline(SvClient *aClient)
{
	//DebugLog(L_INFO, "MMG_AccountProxy::SetClientOnline(%ws)", aClient->GetProfile()->m_Name);

	if (!this->SetProfileOnlineStatus(aClient, WIC_PROFILE_STATUS_ONLINE))
		return false;

	return true;
}

bool MMG_AccountProxy::SetClientPlaying(SvClient *aClient)
{
	//DebugLog(L_INFO, "MMG_AccountProxy::SetClientPlaying(%ws)", aClient->GetProfile()->m_Name);
	
	if (!this->SetProfileOnlineStatus(aClient, WIC_PROFILE_STATUS_PLAYING))
		return false;
	
	return true;
}

bool MMG_AccountProxy::SetProfileOnlineStatus(SvClient *aClient, uint aStatus)
{
	DebugLog(L_INFO, "MMG_AccountProxy::SetProfileOnlineStatus(%ws, %d)", aClient->GetProfile()->m_Name, aStatus);

	//TODO: use MMG_Profile instead of SvClient, maybe keep 2 lists, authtokens and profiles

	MMG_AuthToken *myAuthToken = aClient->GetToken();
	MMG_Profile *myProfile = aClient->GetProfile();

	// set value to client object pointed to in shared memory
	myProfile->m_OnlineStatus = aStatus;

	// determine what operation to perform on the map (online player list)
	switch(aStatus)
	{
		case WIC_PROFILE_STATUS_OFFLINE:
		{
			// TODO
			this->m_PlayerList.erase(myAuthToken->m_AccountId);
		}
		break;

		case WIC_PROFILE_STATUS_ONLINE:
		{
			std::map<uint, SvClient*>::iterator iter;

			iter = this->m_PlayerList.find(myAuthToken->m_AccountId);
			if (iter == this->m_PlayerList.end())
				this->m_PlayerList.insert(std::pair<uint, SvClient*>(myAuthToken->m_AccountId, aClient));
		}
		break;

		default:

			// set client profile status = playing, the server id is used so a friend can right click->join same server
			std::map<uint, SvClient*>::iterator iter;

			iter = this->m_PlayerList.find(myAuthToken->m_AccountId);
			if (iter != this->m_PlayerList.end())
				iter->second->GetProfile()->m_OnlineStatus = aStatus;
		break;
	}

	// send update to all online players except for self
	std::map<uint, SvClient*>::iterator iter;
	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		MN_WriteMessage	responseMessage(2048);
		responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
		myProfile->ToStream(&responseMessage);

		if (!iter->second->SendData(&responseMessage))
			continue;
	}

	return true;
}

void MMG_AccountProxy::CheckProfileOnlineStatus(MMG_Profile *profile)
{
	//TODO 

	std::map<uint, SvClient*>::iterator iter;

	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		if (iter->second->GetProfile()->m_ProfileId == profile->m_ProfileId)
			profile->m_OnlineStatus = iter->second->GetProfile()->m_OnlineStatus;
	}
}

bool MMG_AccountProxy::AccountInUse(MMG_AuthToken *authtoken)
{
	// TODO

	if (this->m_PlayerList.find(authtoken->m_AccountId) == this->m_PlayerList.end())
		return true;

	return false;
}

bool MMG_AccountProxy::ProfileInUse(MMG_Profile *profile)
{
	//TODO

	std::map<uint, SvClient*>::iterator iter;

	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		MMG_Profile *p = iter->second->GetProfile();
		if (p->m_ProfileId == profile->m_ProfileId)
			return true;
	}

	return false;
}

SvClient* MMG_AccountProxy::GetClientByProfileId(uint profileId)
{
	//TODO 

	std::map<uint, SvClient*>::iterator iter;

	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		if (iter->second->GetProfile()->m_ProfileId == profileId)
			return iter->second;
	}
	
	return nullptr;
}