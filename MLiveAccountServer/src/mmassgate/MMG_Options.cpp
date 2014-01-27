#include "../stdafx.h"

uint MMG_Options::ToUInt()
{
	#define BIT_SET(var, offset) if (var){ aRetVal |= (1 << offset); }

	uint aRetVal = 0;

	BIT_SET(this->m_PccInGame, 0);
	BIT_SET(this->m_ShowMyPcc, 1);
	BIT_SET(this->m_ShowFriendsPcc, 2);
	BIT_SET(this->m_ShowOthersPcc, 3);
	BIT_SET(this->m_ShowClanPccInGame, 4);
	BIT_SET(this->m_ReceiveFromFriends, 5);
	BIT_SET(this->m_ReceiveFromClanMembers, 6);
	BIT_SET(this->m_ReceiveFromAcquaintances, 7);
	BIT_SET(this->m_ReceiveFromEveryoneElse, 8);
	BIT_SET(this->m_AllowCommunicationInGame, 9);
	BIT_SET(this->m_EmailIfOffline, 10);
	BIT_SET(this->m_GroupInviteFromFriendsOnly, 11);
	BIT_SET(this->m_ClanInviteFromFriendsOnly, 12);
	BIT_SET(this->m_ServerInviteFromFriendsOnly, 13);

	#undef BIT_SET

	return aRetVal;
}

void MMG_Options::FromUInt(uint aUInt)
{
	#define BIT_READ(offset) !!((aUInt) & (1 << (offset)))

	this->m_PccInGame					= BIT_READ(0);
	this->m_ShowMyPcc					= BIT_READ(1);
	this->m_ShowFriendsPcc				= BIT_READ(2);
	this->m_ShowOthersPcc				= BIT_READ(3);
	this->m_ShowClanPccInGame			= BIT_READ(4);
	this->m_ReceiveFromFriends			= BIT_READ(5);
	this->m_ReceiveFromClanMembers		= BIT_READ(6);
	this->m_ReceiveFromAcquaintances	= BIT_READ(7);
	this->m_ReceiveFromEveryoneElse		= BIT_READ(8);
	this->m_AllowCommunicationInGame	= BIT_READ(9);
	this->m_EmailIfOffline				= BIT_READ(10);
	this->m_GroupInviteFromFriendsOnly	= BIT_READ(11);
	this->m_ClanInviteFromFriendsOnly	= BIT_READ(12);
	this->m_ServerInviteFromFriendsOnly	= BIT_READ(13);

	#undef BIT_READ
}