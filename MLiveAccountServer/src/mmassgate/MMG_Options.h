#pragma once

class MMG_Options
{
public:
	bool	m_PccInGame;
	bool	m_ShowMyPcc;
	bool	m_ShowFriendsPcc;
	bool	m_ShowOthersPcc;
	bool	m_ShowClanPccInGame;
	int		m_ReceiveFromFriends;
	int		m_ReceiveFromClanMembers;
	int		m_ReceiveFromAcquaintances;
	int		m_ReceiveFromEveryoneElse;
	int		m_AllowCommunicationInGame;
	int		m_EmailIfOffline;
	int		m_GroupInviteFromFriendsOnly;
	int		m_ClanInviteFromFriendsOnly;
	int		m_ServerInviteFromFriendsOnly;

private:

public:
	MMG_Options()
	{
		this->m_PccInGame					= 0;
		this->m_ShowMyPcc					= 0;
		this->m_ShowFriendsPcc				= 0;
		this->m_ShowOthersPcc				= 0;
		this->m_ShowClanPccInGame			= 0;
		this->m_ReceiveFromFriends			= 0;
		this->m_ReceiveFromClanMembers		= 0;
		this->m_ReceiveFromAcquaintances	= 0;
		this->m_ReceiveFromEveryoneElse		= 0;
		this->m_AllowCommunicationInGame	= 0;
		this->m_EmailIfOffline				= 0;
		this->m_GroupInviteFromFriendsOnly	= 0;
		this->m_ClanInviteFromFriendsOnly	= 0;
		this->m_ServerInviteFromFriendsOnly	= 0;
	}

	uint ToUInt();
	void FromUInt(uint aUInt);
};