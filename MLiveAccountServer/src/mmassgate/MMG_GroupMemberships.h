#pragma once

union MMG_GroupMemberships
{
	uint m_Code;

	enum : int
	{
		presale,
		moderator,
		isRecruitedFriend,
		hasRecruitedFriend,
		isRankedServer,
		isTournamentServer,
		isClanMatchServer,
		reserved8,
		isCafeKey,
		isPublicGuestKey,
		reserved11,
		reserved12,
		reserved13,
		reserved14,
		reserved15,
		reserved16,
		reserved17,
		reserved18,
		reserved19,
		reserved20,
		reserved21,
		reserved22,
		reserved23,
		reserved24,
		reserved25,
		reserved26,
		reserved27,
		reserved28,
		reserved29,
		reserved30,
		reserved31,
		reserved32,
	} memberOf;
};