#pragma once

enum myClanStrings : uchar
{
	FAIL_INVALID_NAME					= 4,	// The requested name of the clan is invalid, Please try another name.
	FAIL_TAG_TAKEN						= 5,	// Another clan already has the tag you requested, Please choose another tag.
	FAIL_OTHER							= 6,	// Could not create clan. Clan name and tag may be invalid.
	InternalMassgateError				= 7,	// Cannot modify clan: An internal massgate error occured.
	FAIL_MASSGATE						= 12,	// Cannot modify clan: An internal massgate error occured.
	FAIL_ALREADY_IN_CLAN				= 9,	// The player is already in a clan and can not be invited to your clan.
	InviteSent							= 8,	// Invite sent!
	FAIL_DUPLICATE						= 10,	// The player is already invited to a clan.
	FAIL_INVALID_PRIVILIGES				= 11,	// You do not have the right to invite players to your clan.
	MODIFY_FAIL_INVALID_PRIVILIGES		= 14,	// You must be the clan leader to modify the clan
	MODIFY_FAIL_TOO_FEW_MEMBERS			= 15,	// The operation requires more members. If you want to quit the clan - select leave on the clan page instead of kicking yourself from the clan
	MODIFY_FAIL_OTHER					= 16,	// Could not modify clan.
	MODIFY_FAIL_MASSGATE				= 17,	// A massgate error occured while modifying clan. Try again later.
	INVITE_FAIL_PLAYER_IGNORE_MESSAGES	= 13,	// Can not invite player. The player ignores messages and invites from you.
};

class MMG_Clan
{
public:
	class FullInfo
	{
	public:
		uint m_ClanId;
		wchar_t m_FullClanName[WIC_CLANNAME_MAX_LENGTH];
		wchar_t m_ShortClanName[WIC_CLANTAG_MAX_LENGTH];
		wchar_t m_Motto[WIC_MOTTO_MAX_LENGTH];
		wchar_t m_LeaderSays[WIC_MOTD_MAX_LENGTH];
		wchar_t m_Homepage[WIC_HOMEPAGE_MAX_LENGTH];
		uint m_ClanMembers[512];
		uint m_PlayerOfWeek;
	private:

	public:
		FullInfo();
	};

	class Description
	{
	public:
		uint m_ClanId;
		wchar_t m_FullName[WIC_CLANNAME_MAX_LENGTH];
		wchar_t m_ClanTag[WIC_CLANTAG_MAX_LENGTH];
	private:

	public:
		Description();

		void ToStream	(MN_WriteMessage *aMessage);
	};
	
private:

public:
	MMG_Clan();
};