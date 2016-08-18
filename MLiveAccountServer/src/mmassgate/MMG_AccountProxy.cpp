#include "../stdafx.h"

MMG_AccountProxy::MMG_AccountProxy()
{
}

bool MMG_AccountProxy::SetClientOffline(SvClient *aClient)
{
	MMG_AuthToken *myAuthToken = aClient->GetToken();

	this->m_PlayerList.erase(myAuthToken->m_AccountId);

	return true;
}

bool MMG_AccountProxy::SetClientOnline(SvClient *aClient)
{
	MMG_AuthToken *myAuthToken = aClient->GetToken();

	std::map<uint, SvClient*>::iterator iter;

	iter = this->m_PlayerList.find(myAuthToken->m_AccountId);
	if (iter == this->m_PlayerList.end())
		this->m_PlayerList.insert(std::pair<uint, SvClient*>(myAuthToken->m_AccountId, aClient));

	return true;
}

bool MMG_AccountProxy::UpdateClients(MMG_Profile *profile)
{
	// send Profile/playername update to all online clients, including self
	std::map<uint, SvClient*>::iterator iter;

	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		MN_WriteMessage	responseMessage(2048);
		responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_RESPOND_PROFILENAME);
		profile->ToStream(&responseMessage);

		if (!iter->second->SendData(&responseMessage))
			continue;
	}

	return true;
}

bool MMG_AccountProxy::SendPlayerJoinedClan(MMG_Profile *profile)
{
	std::map<uint, SvClient*>::iterator iter;

	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		MN_WriteMessage	responseMessage(2048);
		responseMessage.WriteDelimiter(MMG_ProtocolDelimiters::MESSAGING_PLAYER_JOINED_CLAN);
		profile->ToStream(&responseMessage);

		if (!iter->second->SendData(&responseMessage))
			continue;
	}

	return true;
}

bool MMG_AccountProxy::AccountInUse(uint accountId)
{
	// TODO

	if (this->m_PlayerList.find(accountId) != this->m_PlayerList.end())
		return true;

	return false;
}

bool MMG_AccountProxy::ProfileInUse(uint profileId)
{
	//TODO

	std::map<uint, SvClient*>::iterator iter;

	for (iter = this->m_PlayerList.begin(); iter != this->m_PlayerList.end(); ++iter)
	{
		MMG_Profile *p = iter->second->GetProfile();
		if (p->m_ProfileId == profileId)
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