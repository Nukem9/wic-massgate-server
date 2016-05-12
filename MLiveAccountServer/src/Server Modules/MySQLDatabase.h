#pragma once

#ifndef USING_MYSQL_DATABASE
//#define USING_MYSQL_DATABASE
#endif

namespace MySQLDatabase
{
	bool	Initialize			();
	void	Unload				();
	bool	ReadConfig			();
	bool	InitializeSchema	();
	
	//accounts
	bool	CheckIfEmailExists	(const char *email, uint *dstId);
	bool	CheckIfCDKeyExists	(const ulong cipherKeys[], uint *dstId);
	bool	CreateUserAccount	(const char *email, const wchar_t *password, const char *country, const uchar *emailgamerelated, const uchar *acceptsemail, const ulong cipherKeys[]);
	bool	AuthUserAccount		(const char *email, wchar_t *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken);
	
	//profiles (logins)
	bool	CheckIfProfileExists(const wchar_t* name, uint *dstId);
	bool	CreateUserProfile	(const uint accountId, const wchar_t* name, const char* email);
	bool	DeleteUserProfile	(const uint accountId, const uint profileId, const char* email);
	bool	QueryUserProfile	(const uint accountId, const uint profileId, MMG_Profile *profile, MMG_Options *options);
	bool	RetrieveUserProfiles	(const char *email, const wchar_t *password, ulong *dstProfileCount, MMG_Profile *profiles[]);

	//messaging
	bool	QueryUserOptions	(const uint profileId, int *options);	//TODO
	bool	SaveUserOptions		(const uint profileId, const int options);
	bool	QueryFriends		(const uint profileId, uint *dstProfileCount, uint *friendIds[]);
	bool	QueryAcquaintances	(const uint profileId, uint *dstProfileCount, uint *acquaintanceIds[]);
	bool	QueryIgnoredProfiles	(const uint profileId, uint *dstProfileCount, uint *ignoredIds[]);	//TODO
	bool	QueryProfileName	(const uint profileId, MMG_Profile *profile);
}