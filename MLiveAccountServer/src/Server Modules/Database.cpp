#include "../stdafx.h"

#define DatabaseLog(format, ...) DebugLog(L_INFO, "[Database]: "format, __VA_ARGS__)

namespace Database
{
	MT_Mutex m_Mutex;

	sqlite3 *db_handle;

	bool Initialize()
	{
		// Set it to serialized multithreaded mode
		if (sqlite3_config(SQLITE_CONFIG_SERIALIZED, 1) != SQLITE_OK)
		{
			DatabaseLog("Can't open database: Couldn't set multithreaded configuration");
			return false;
		}

		// Open a handle to the SQLite3 database file
		// Also create if it does not exist
		int err = sqlite3_open_v2("test.db", &db_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

		if (err != SQLITE_OK)
		{
			DatabaseLog("Can't open database: %s\n", sqlite3_errmsg(db_handle));
			return false;
		}

		DatabaseLog("Opened a handle to 'test.db'");
		return true;
	}

	void Unload()
	{
		if (db_handle)
			sqlite3_close_v2(db_handle);

		db_handle = nullptr;
	}

	bool InitializeTables()
	{
		// Go through each table entry
		for(int i = 0; i < ARRAYSIZE(TableValues); i++)
		{
			SQLQuery query(db_handle, TableValues[i]);

			if (query.Step() != SQLITE_OK)
				return false;
		}

		return true;
	}

	size_t JSONDataReceived(void *ptr, size_t size, size_t nmemb, void *userdata)
	{
		curl_userdata *c_userdata = (curl_userdata *)userdata;

		size_t maxSize = sizeof(c_userdata->dataBuffer);
		size_t dataSize = size * nmemb;

		if ((strlen(c_userdata->dataBuffer) + dataSize) > maxSize)
			return dataSize;

		strncat_s(c_userdata->dataBuffer, maxSize, (const char *)ptr, dataSize);

		return dataSize;
	}

	bool FetchJSONUrl(const char *url, JSON *json)
	{
		return false;
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
		char *sql = "INSERT INTO LiveAccUsers (Username, Email, Password, Experience, Rank, ClanID, RankInClan, ChatOptions, OnlineStatus) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

		SQLQuery query(db_handle, sql);

		query.Bind(1, "");									// Username
		query.Bind(2, email);								// Email
		query.BindBlob(3, (void*)password, wcslen(password) * 2);	// Password
		query.Bind(4, 0);									// Experience
		query.Bind(5, 0);									// Rank
		query.Bind(6, 0);									// ClanID
		query.Bind(7, 0);									// RankInClan
		query.Bind(8, 0);									// ChatOptions
		query.Bind(9, 0);									// OnlineStatus

		for(;;)
		{
			int result = query.Step();

			if (result == SQLITE_DONE)
				break;
			else if (result != SQLITE_ROW)
				return WIC_INVALID_ACCOUNT;
		}

		return (uint)sqlite3_last_insert_rowid(db_handle);
	}

	uint AuthUserAccount(const char *email, const wchar_t *password)
	{
		JSON json;
		if (!BackendQuery(&json, GAME_AUTH_ACCOUNT, email, password))
			return WIC_INVALID_ACCOUNT;

		int j_status;
		if (!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("CreateUserAccount: JSON status failed.");
			return WIC_INVALID_ACCOUNT;
		}

		uint j_profileId;
		if (!json.ReadUInt("profileId", &j_profileId))
		{
			DatabaseLog("Error: Server returned invalid profile ID.");
			return WIC_INVALID_ACCOUNT;
		}

		return j_profileId;
	}

	bool QueryUserProfile(const wchar_t *name, MMG_Profile *profile)
	{
		JSON json;
		if (!BackendQuery(&json, GAME_GET_PROFILE_NAME, name))
			return false;

		int j_status;
		if (!json.ReadInt("status", &j_status) || j_status == 0)
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

		if (!bResult)
		{
			DatabaseLog("Error: Server returned invalid profile fields.");
			return false;
		}

		return true;
	}

	bool QueryUserProfile(uint profileId, MMG_Profile *profile)
	{
		JSON json;
		if (!BackendQuery(&json, GAME_GET_PROFILE, profileId))
			return false;

		int j_status;
		if (!json.ReadInt("status", &j_status) || j_status == 0)
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

		if (!bResult)
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

		if (!BackendQuery(&json, GAME_GET_PROFILE, profileId))
			return false;

		int j_status;
		if (!json.ReadInt("status", &j_status) || j_status == 0)
		{
			DatabaseLog("QueryUserOptions: JSON status failed.");
			return false;
		}

		return true;
	}
}