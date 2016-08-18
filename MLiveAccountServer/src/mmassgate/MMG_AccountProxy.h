#pragma once

CLASS_SINGLE(MMG_AccountProxy)
{
public:

	// map of pointers to client objects. key == accountid <SvClient->MMG_AuthToken->m_AccountId, SvClient>
	std::map<uint, SvClient*> m_PlayerList;

	MMG_AccountProxy();

	//TODO: use MMG_Profile instead of SvClient, maybe keep 2 lists, authtokens and profiles
	bool SetClientOffline	(SvClient *aClient);
	bool SetClientOnline	(SvClient *aClient);

	bool UpdateClients(MMG_Profile *profile);
	bool SendPlayerJoinedClan	(MMG_Profile *profile);

	bool AccountInUse		(uint accountId);
	bool ProfileInUse		(uint profileId);

	SvClient* GetClientByProfileId	(uint profileId);
};