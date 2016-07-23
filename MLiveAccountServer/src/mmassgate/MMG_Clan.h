#pragma once

enum myClanStrings : uint
{
	FAIL_INVALID_NAME					= 4,
	FAIL_TAG_TAKEN						= 5,
	FAIL_OTHER							= 6,
	InternalMassgateError				= 7,		// not a myClanString enum, technically the enum isnt an enum
	FAIL_MASSGATE						= 12,
	FAIL_ALREADY_IN_CLAN				= 9,
	InviteSent							= 8,
	FAIL_DUPLICATE						= 10,
	FAIL_INVALID_PRIVILIGES				= 11,
	MODIFY_FAIL_INVALID_PRIVILIGES		= 14,
	MODIFY_FAIL_TOO_FEW_MEMBERS			= 15,
	MODIFY_FAIL_OTHER					= 16,
	MODIFY_FAIL_MASSGATE				= 17,
	INVITE_FAIL_PLAYER_IGNORE_MESSAGES	= 13,
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
	
private:

public:
	MMG_Clan();
};