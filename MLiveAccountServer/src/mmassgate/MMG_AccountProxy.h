#pragma once

CLASS_SINGLE(MMG_AccountProxy)
{
public:

	// map of pointers to client objects. key == accountid <SvClient->MMG_AuthToken->m_AccountId, SvClient>
	std::map<uint, SvClient*> m_PlayerList;

	MMG_AccountProxy();

	bool SetClientOffline	(SvClient *aClient);
	bool SetClientOnline	(SvClient *aClient);
	bool SetClientPlaying	(SvClient *aClient);

	bool SetProfileOnlineStatus		(SvClient *aClient, uint aStatus);
	void CheckProfileOnlineStatus	(MMG_Profile *profile);

	bool AccountInUse		(MMG_AuthToken *authtoken);
	bool ProfileInUse		(MMG_Profile *profile);
};