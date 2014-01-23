#include "../stdafx.h"

#define DatabaseLog(format, ...) DebugLog(L_INFO, "[Database]: "format, __VA_ARGS__)

namespace Database
{
	sqlite3 *db_handle;

	bool Initialize()
	{
		// Open a handle to the SQLite3 database file
		int err = sqlite3_open("test.db", &db_handle);

		if(err)
		{
			DatabaseLog("Can't open database: %s\n", sqlite3_errmsg(db_handle));
			return false;
		}

		DatabaseLog("Opened a handle to 'test.db'");
		return true;
	}

	void Unload()
	{
		if(db_handle)
			sqlite3_close(db_handle);

		db_handle = nullptr;
	}

	int SQLCallback(void *userdata, int argc, char **argv, char **ColNames)
	{
		for(int i = 0; i < argc; i++)
		{
			printf("%s = %s\n", ColNames[i], argv[i] ? argv[i] : "NULL");
		}

		printf("\n");
		return 0;
	}

	size_t JSONDataReceived(void *ptr, size_t size, size_t nmemb, void *userdata)
	{
		curl_userdata *c_userdata = (curl_userdata *)userdata;

		size_t maxSize = sizeof(c_userdata->dataBuffer);
		size_t dataSize = size * nmemb;

		if((strlen(c_userdata->dataBuffer) + dataSize) > maxSize)
			return dataSize;

		strncat_s(c_userdata->dataBuffer, maxSize, (const char *)ptr, dataSize);

		return dataSize;
	}

	bool FetchJSONUrl(const char *url, JSON *json)
	{
		CURL *curl = curl_easy_init();

		if(!curl)
		{
			DatabaseLog("Error: Failed to load CURL.");
			return false;
		}

		curl_userdata userdata;
		memset(&userdata, 0, sizeof(userdata));

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, JSONDataReceived);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userdata);

		CURLcode code = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if(code != CURLE_OK)
		{
			DatabaseLog("Error: CURL returned an error: %i", code);
			return false;
		}

		if(!json->LoadJSON(userdata.dataBuffer))
		{
			DatabaseLog("Error: Failed to load JSON parser. Check line %i\nData: %s\n", json->error.line, userdata.dataBuffer);
			return false;
		}

		return true;
	}

	bool BackendQuery(JSON *json, PHPGameQueries queryType, ...)
	{
		char urlbuffer[2048];
		memset(urlbuffer, 0, sizeof(urlbuffer));

		const char *baseUrl = "http://192.168.0.200/massgate/db_query.php?query=game";

		// used for parsing each variable parameter passed
		va_list va;
		va_start(va, queryType);

		switch(queryType)
		{
			// Parameters:
			// [user] Email address
			// [pass] Password
			case GAME_CREATE_ACCOUNT:
			case GAME_AUTH_ACCOUNT:
			{
				const char *email_addr	= va_arg(va, const char *);
				const wchar_t *password = va_arg(va, const wchar_t *);

				sprintf_s(urlbuffer, "%s&sub=%i&user=%s&pass=%ws", baseUrl, queryType, email_addr, password);
			}
			break;

			// Parameters:
			// [profileId] User profile ID
			case GAME_GET_PROFILE:
			{
				uint profile_id = va_arg(va, uint);

				sprintf_s(urlbuffer, "%s&sub=%i&profileId=%i", baseUrl, queryType, profile_id);
			}
			break;

			// Parameters:
			// [profileName] User profile nickname
			case GAME_GET_PROFILE_NAME:
			{
				const char *profile_name = va_arg(va, const char *);

				sprintf_s(urlbuffer, "%s&sub=%i&profileName=%s", baseUrl, queryType, profile_name);
			}
			break;
		}

		va_end(va);

		return FetchJSONUrl(urlbuffer, json);
	}

	uint CreateUserAccount(const char *email, const wchar_t *password)
	{
		JSON json;
		if(!BackendQuery(&json, GAME_CREATE_ACCOUNT, email, password))
			return WIC_INVALID_ACCOUNT;

		int j_status;
		if(!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("CreateUserAccount: JSON status failed.");
			return WIC_INVALID_ACCOUNT;
		}

		uint j_profileId;
		if(!json.ReadUInt("profileId", &j_profileId))
		{
			DatabaseLog("Error: Server returned invalid profile ID.");
			return WIC_INVALID_ACCOUNT;
		}

		return j_profileId;
	}

	uint AuthUserAccount(const char *email, const wchar_t *password)
	{
		JSON json;
		if(!BackendQuery(&json, GAME_AUTH_ACCOUNT, email, password))
			return WIC_INVALID_ACCOUNT;

		int j_status;
		if(!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("CreateUserAccount: JSON status failed.");
			return WIC_INVALID_ACCOUNT;
		}

		uint j_profileId;
		if(!json.ReadUInt("profileId", &j_profileId))
		{
			DatabaseLog("Error: Server returned invalid profile ID.");
			return WIC_INVALID_ACCOUNT;
		}

		return j_profileId;
	}

	bool QueryUserProfile(const wchar_t *name, MMG_Profile *profile)
	{
		JSON json;
		if(!BackendQuery(&json, GAME_GET_PROFILE_NAME, name))
			return false;

		int j_status;
		if(!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("QueryUserProfile: JSON status failed.");
			return false;
		}

		bool bResult = json.ReadString("name", profile->m_Name, sizeof(profile->m_Name));
		bResult = bResult && json.ReadUInt("profileId", &profile->m_ProfileId);
		bResult = bResult && json.ReadUInt("clanId", &profile->m_ClanId);
		bResult = bResult && json.ReadUInt("onlineStatus", &profile->m_OnlineStatus);
		bResult = bResult && json.ReadUChar("rank", &profile->m_Rank);
		bResult = bResult && json.ReadUChar("clanRank", &profile->m_RankInClan);

		if(!bResult)
		{
			DatabaseLog("Error: Server returned invalid profile fields.");
			return false;
		}

		return true;
	}

	bool QueryUserProfile(uint profileId, MMG_Profile *profile)
	{
		JSON json;
		if(!BackendQuery(&json, GAME_GET_PROFILE, profileId))
			return false;

		int j_status;
		if(!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("QueryUserProfile: JSON status failed.");
			return false;
		}

		bool bResult = json.ReadString("name", profile->m_Name, sizeof(profile->m_Name));
		bResult = bResult && json.ReadUInt("profileId", &profile->m_ProfileId);
		bResult = bResult && json.ReadUInt("clanId", &profile->m_ClanId);
		bResult = bResult && json.ReadUInt("onlineStatus", &profile->m_OnlineStatus);
		bResult = bResult && json.ReadUChar("rank", &profile->m_Rank);
		bResult = bResult && json.ReadUChar("clanRank", &profile->m_RankInClan);

		if(!bResult)
		{
			DatabaseLog("Error: Server returned invalid profile fields.");
			return false;
		}

		return true;
	}

	bool QueryUserOptions(uint profileId, int *options)
	{
		*options = 0;
		return true;

		JSON json;

		if(!BackendQuery(&json, GAME_GET_PROFILE, profileId))
			return false;

		int j_status;
		if(!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("QueryUserOptions: JSON status failed.");
			return false;
		}

		return true;
	}
}