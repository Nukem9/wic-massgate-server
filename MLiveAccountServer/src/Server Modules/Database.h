#pragma once

#include "JSON.h"

namespace Database
{
	enum PHPGameQueries
	{
		// AccountProtocol
		GAME_CREATE_ACCOUNT = 0,
		GAME_AUTH_ACCOUNT = 1,

		// Messaging
		GAME_GET_PROFILE = 10,
		GAME_GET_PROFILE_NAME = 11,
	};

	struct curl_userdata
	{
		char dataBuffer[8192];
	};

	bool	Initialize			();
	void	Unload				();

	size_t	JSONDataReceived	(void *ptr, size_t size, size_t nmemb, void *userdata);
	bool	FetchJSONUrl		(const char *url, JSON *json);

	bool	BackendQuery		(JSON *json, PHPGameQueries queryType, ...);

	uint	CreateUserAccount	(const char *email, const wchar_t *password);
	uint	AuthUserAccount		(const char *email, const wchar_t *password);

	bool	QueryUserProfile	(const char *name, MMG_Profile *profile);
	bool	QueryUserProfile	(uint profileId, MMG_Profile *profile);
	bool	QueryUserOptions	(uint profileId, int *options);
}