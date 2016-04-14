#pragma once

namespace MySQLDatabase
{
	bool	Initialize			();
	void	Unload				();
	bool	ReadConfig			();
	bool	InitializeSchema	();
	
	//accounts
	bool	CheckIfEmailExists	(const char *email, uint *dstId);
	bool	CheckIfCDKeyExists	(uint *dstId);
	bool	CreateUserAccount	(const char *email, const wchar_t *password, const char *country, const uchar *emailme, const uchar *acceptsemail);
	bool	AuthUserAccount		(const char *email, wchar_t *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken);
	
	//profiles
	bool	CheckIfProfileExists(const wchar_t* name, uint *dstId);
	bool	CreateUserProfile	(const uint accountId, const wchar_t* name, const char* email);
	bool	DeleteUserProfile	(const uint accountId, const uint profileId, const char* email);
	bool	QueryUserProfile	(const uint accountId, const uint profileId, MMG_Profile *profile);
	bool	RetrieveUserProfiles	(const char *email, const wchar_t *password, ulong *dstProfileCount, MMG_Profile *profiles[]);

	bool	QueryUserOptions	(const wchar_t *name, int *options);
	bool	QueryUserOptions	(const uint profileId, int *options);
}