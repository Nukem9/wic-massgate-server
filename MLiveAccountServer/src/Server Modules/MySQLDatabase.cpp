#include "../stdafx.h"

#include <fstream>
#include <string>

#define DatabaseLog(format, ...) DebugLog(L_INFO, "[Database]: "format, __VA_ARGS__)

DWORD WINAPI MySQLDatabase::PingThread(LPVOID lpArg)
{
	return ((MySQLDatabase*)lpArg)->PingDatabase();
}

bool MySQLDatabase::Initialize()
{
	if (!ReadConfig())
		return false;

	if (!ConnectDatabase())
		return false;

	//create a thread to 'ping' the database every 5 hours, preventing timeout.
	this->m_PingThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PingThread, this, 0, NULL);

	return true;
}

void MySQLDatabase::Unload()
{
	TerminateThread(this->m_PingThreadHandle, 0);
	this->m_PingThreadHandle = nullptr;

	mysql_close(this->m_Connection);
	mysql_thread_end();
	this->m_Connection = nullptr;

	if(this->host)
	{
		free((char*)this->host);
		free((char*)this->user);
		free((char*)this->pass);
		free((char*)this->db);

		this->host = nullptr;
		this->user = nullptr;
		this->pass = nullptr;
		this->db = nullptr;
	}
}

bool MySQLDatabase::ReadConfig()
{
	//might need to change working directory in property pages if it cant find the file
	std::ifstream file("settings.ini", std::ios::in);
	std::string line;
	std::string settings[4];

	int i=0;

	if(file.is_open())
	{
		while(getline(file, line))
		{
			if(line.substr(0, 1)!="[" && line != "")
			{
				settings[i] = line;
				i++;
			}
		}
	} else {
		DatabaseLog("could not open configuration file 'settings.ini'");
		return false;
	}

	file.close();

	host = (char*)malloc(settings[0].size() + 1);
	memcpy((char*)host, settings[0].c_str(), settings[0].size() + 1);

	user = (char*)malloc(settings[1].size() + 1);
	memcpy((char*)user, settings[1].c_str(), settings[1].size() + 1);

	pass = (char*)malloc(settings[2].size() + 1);
	memcpy((char*)pass, settings[2].c_str(), settings[2].size() + 1);

	db = (char*)malloc(settings[3].size() + 1);
	memcpy((char*)db, settings[3].c_str(), settings[3].size() + 1);

	return true;
}

bool MySQLDatabase::ConnectDatabase()
{
	//create the connection object
	this->m_Connection = mysql_init(NULL);

	//set the connection options
	mysql_options(this->m_Connection, MYSQL_OPT_RECONNECT, &this->reconnect);
	mysql_options(this->m_Connection, MYSQL_OPT_BIND, this->bind_interface);
	mysql_options(this->m_Connection, MYSQL_SET_CHARSET_NAME, this->charset_name);

	// attempt connection to the database
	if (mysql_real_connect(this->m_Connection, host, user, pass, db, 0, NULL, 0) == NULL)
	{
		DebugLog(L_ERROR, "%s", mysql_error(this->m_Connection));
		mysql_close(this->m_Connection);
		mysql_thread_end();

		return false;
	}

	DatabaseLog("client version: %s started", mysql_get_client_info());
	DatabaseLog("connection to %s ok", host);

	return true;
}

bool MySQLDatabase::TestDatabase()
{
	MYSQL_RES *result;
	bool error_flag = false;

	// execute a standard query on the database
	if (mysql_query(this->m_Connection, "SELECT poll FROM mg_utils LIMIT 1"))
		error_flag = true;

	// must call mysql_store_result after calling mysql_query
	result = mysql_store_result(this->m_Connection);

	if (!result)
		error_flag = true;
	else
	{
		// if there is only one field, query has executed successfully, no need to check the row
		if(mysql_num_fields(result) != 1)
			error_flag = true;
	}

	// disconnect everyone
	if (error_flag)
	{
		mysql_free_result(result);

		// set maintenance mode, server will most likely need attending if it gets to this point.
		DatabaseLog("connection to %s lost, check the MySQL server.", host);

		MMG_AccountProtocol::ourInstance->m_MaintenanceMode = true;
		SvClientManager::ourInstance->EmergencyDisconnectAll();

		return false;
	}

	mysql_free_result(result);

	return true;
}

bool MySQLDatabase::PingDatabase()
{
	while (true)
	{
		Sleep(this->sleep_interval_ms);

		if (mysql_ping(this->m_Connection) == 0)
		{
			MMG_AccountProtocol::ourInstance->m_MaintenanceMode = false;
		}
		else
		{
			//connection lost
			//mysql_ping will attempt to reconnect automatically, see notes in header.

			DatabaseLog("connection to %s lost, attempting reconnect", host);
			MMG_AccountProtocol::ourInstance->m_MaintenanceMode = true;
			SvClientManager::ourInstance->EmergencyDisconnectAll();
		}
	}
	
	return true;
}

bool MySQLDatabase::InitializeSchema()
{
	// Go through each table entry
	for(int i = 0; i < ARRAYSIZE(MySQLTableValues); i++)
	{
		MySQLQuery query(this->m_Connection, MySQLTableValues[i]);
		
		if (!query.Step())
			return false;
	}

	DatabaseLog("database tables created OK");

	return true;
}

bool MySQLDatabase::CheckIfEmailExists(const char *email, uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT id FROM mg_accounts WHERE email = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], result[1];
	ulong email_len;
	uint id;
	bool querySuccess;

	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	query.Bind(&param[0], email, &email_len);	//bind 'email' to the first/only parameter

	query.Bind(&result[0], &id);				//mg_accounts.id

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfEmailExists() query failed: %s", email);
		querySuccess = false;
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("email %s not found", email);
			querySuccess = true;
			*dstId = 0;
		}
		else
		{
			DatabaseLog("email %s found", email);
			querySuccess = true;
			*dstId = id;
		}
	}

	return querySuccess;
}

bool MySQLDatabase::CheckIfCDKeyExists(const ulong cipherKeys[], uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT id FROM mg_cdkeys WHERE cipherkeys0 = ? AND cipherkeys1 = ? AND cipherkeys2 = ? AND cipherkeys3 = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND params[4], result[1];
	uint id;
	bool querySuccess;

	memset(params, 0, sizeof(params));
	memset(result, 0, sizeof(result));

	query.Bind(&params[0], &cipherKeys[0]);
	query.Bind(&params[1], &cipherKeys[1]);
	query.Bind(&params[2], &cipherKeys[2]);
	query.Bind(&params[3], &cipherKeys[3]);

	query.Bind(&result[0], &id);			//mg_cdkeys.id

	if(!query.StmtExecute(params, result))
	{
		DatabaseLog("CheckIfCDKeyExists() query failed:");
		querySuccess = false;
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("cipherkeys not found");
			querySuccess = true;
			*dstId = 0;
		}
		else
		{
			DatabaseLog("cipherkeys found");
			querySuccess = true;
			*dstId = id;
		}
	}

	return querySuccess;
}

bool MySQLDatabase::CreateUserAccount(const char *email, const wchar_t *password, const char *country, const uchar *emailgamerelated, const uchar *acceptsemail, const ulong cipherKeys[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	//step 1: insert account
	char *sql = "INSERT INTO mg_accounts (email, password, country, emailgamerelated, acceptsemail, activeprofileid, isbanned) VALUES (?, ?, ?, ?, ?, 0, 0)";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND params[5];
	ulong email_len, pass_len, country_len;
	uint account_insert_id, cdkey_insert_id;
	bool query1Success, query2Success;

	memset(params, 0, sizeof(params));

	query.Bind(&params[0], email, &email_len);			//email
	query.Bind(&params[1], password, &pass_len);		//password
	query.Bind(&params[2], country, &country_len);		//country
	query.Bind(&params[3], emailgamerelated);			//emailgamerelated (game related news and events)
	query.Bind(&params[4], acceptsemail);				//acceptsemail (from ubisoft (pfft) rename this field)

	if (!query.StmtExecute(params))
	{
		DatabaseLog("CreateUserAccount() query failed: %s", email);
		account_insert_id = 0;
		query1Success = false;
	}
	else
	{
		DatabaseLog("created account: %s", email);
		account_insert_id = (uint)mysql_insert_id(this->m_Connection);
		query1Success = true;
	}

	//step 2: insert cdkeys
	if(query1Success)
	{
		char *sql2 = "INSERT INTO mg_cdkeys (accountid, cipherkeys0, cipherkeys1, cipherkeys2, cipherkeys3) VALUES (?, ?, ?, ?, ?)";

		MySQLQuery query2(this->m_Connection, sql2);
		MYSQL_BIND params2[5];

		memset(params2, 0, sizeof(params2));

		query2.Bind(&params2[0], &account_insert_id);	
		query2.Bind(&params2[1], &cipherKeys[0]);
		query2.Bind(&params2[2], &cipherKeys[1]);
		query2.Bind(&params2[3], &cipherKeys[2]);
		query2.Bind(&params2[4], &cipherKeys[3]);

		if (!query2.StmtExecute(params2))
		{
			DatabaseLog("CreateUserAccount() query2 failed: %s", email);
			cdkey_insert_id = 0;
			query2Success = false;
		}
		else
		{
			DatabaseLog("inserted cipherkeys: %s", email);
			cdkey_insert_id = (uint)mysql_insert_id(this->m_Connection);
			query2Success = true;
		}
	}

	return query1Success && query2Success;
}

bool MySQLDatabase::AuthUserAccount(const char *email, wchar_t *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT mg_accounts.id, mg_accounts.password, mg_accounts.activeprofileid, mg_accounts.isbanned, mg_cdkeys.id AS cdkeyid "
				"FROM mg_accounts "
				"JOIN mg_cdkeys "
				"ON mg_accounts.id = mg_cdkeys.accountid "
				"WHERE mg_accounts.email = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], results[5];
	wchar_t password[WIC_PASSWORD_MAX_LENGTH];
	uint id, activeprofileid;
	uchar isbanned;
	uint cdkeyid;
	ulong email_len, pass_len;
	bool querySuccess;

	memset(password, 0, sizeof(password));

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));
		
	query.Bind(&param[0], email, &email_len);		//email

	query.Bind(&results[0], &id);					//mg_accounts.id
	query.Bind(&results[1], password, &pass_len);	//mg_accounts.password
	query.Bind(&results[2], &activeprofileid);		//mg_accounts.activeprofileid
	query.Bind(&results[3], &isbanned);				//mg_accounts.isbanned
	query.Bind(&results[4], &cdkeyid);				//mg_cdkeys.id AS cdkeyid
		
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("AuthUserAccount() query failed: %s", email);
		querySuccess = false;

		authToken->m_AccountId = 0;
		authToken->m_ProfileId = 0;
		//authToken->m_TokenId = 0;
		authToken->m_CDkeyId = 0;
		dstPassword = L"";
		*dstIsBanned = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("account %s not found", email);
			querySuccess = true;

			authToken->m_AccountId = 0;
			authToken->m_ProfileId = 0;
			//authToken->m_TokenId = 0;
			authToken->m_CDkeyId = 0;
			dstPassword = L"";
			*dstIsBanned = 0;
		}
		else
		{
			DatabaseLog("account %s found", email);
			querySuccess = true;

			authToken->m_AccountId = id;
			authToken->m_ProfileId = activeprofileid;
			//authToken->m_TokenId = 0;
			authToken->m_CDkeyId = cdkeyid;
			wcscpy_s(dstPassword, ARRAYSIZE(password), password);
			*dstIsBanned = isbanned;
		}
	}

	return querySuccess;
}

bool MySQLDatabase::CheckIfProfileExists(const wchar_t* name, uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT id FROM mg_profiles WHERE name = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], result[1];
	ulong name_len;
	uint id;
	bool querySuccess;

	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	query.Bind(&param[0], name, &name_len);

	query.Bind(&result[0], &id);		//mg_profiles.id

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfProfileExists() query failed: %ws", name);
		querySuccess = false;
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile %ws not found", name);
			querySuccess = true;
			*dstId = 0;
		}
		else
		{
			DatabaseLog("profile %ws found", name);
			querySuccess = true;
			*dstId = id;
		}
	}

	return querySuccess;
}

bool MySQLDatabase::CreateUserProfile(const uint accountId, const wchar_t* name, const char* email)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	//Step 1: create the profile as usual
	char *sql = "INSERT INTO mg_profiles (accountid, name, rank, clanid, rankinclan, commoptions, onlinestatus, lastlogindate, motto, homepage) VALUES (?, ?, 0, 0, 0, 992, 0, ?, NULL, NULL)";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND params[3];
	ulong name_len, email_len;
	MYSQL_TIME datetime;
	ulong count;
	uint id, last_id;
	bool query1Success, query2Success, query3Success;

	datetime.year = 1970;
	datetime.month = 1;
	datetime.day = 1;
	datetime.hour = 0;
	datetime.minute = 0;
	datetime.second = 0;

	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &accountId);			//account id
	query.Bind(&params[1], name, &name_len);	//profile name
	query.BindDateTime(&params[2], &datetime);	//last login date, set to 1/1/1970 for new accounts

	if (!query.StmtExecute(params))
	{
		DatabaseLog("%s create profile '%ws' failed for account id %d", email, name, accountId);
		query1Success = false;
		last_id = 0;
	}
	else
	{
		DatabaseLog("%s created profile %ws", email, name);
		query1Success = true;
		last_id = (uint)mysql_insert_id(this->m_Connection);
	}

	//Step 2: get profile count for current account
	if(query1Success)
	{
		char *sql2 = "SELECT mg_profiles.id "
					"FROM mg_profiles "
					"JOIN mg_accounts "
					"ON mg_profiles.accountid = mg_accounts.id "
					"WHERE mg_accounts.email = ? "
					"ORDER BY lastlogindate DESC, id ASC";

		MySQLQuery query2(this->m_Connection, sql2);
		MYSQL_BIND params2[1], results2[1];
			
		memset(params2, 0, sizeof(params2));
		memset(results2, 0, sizeof(results2));

		query2.Bind(&params2[0], email, &email_len);	//email

		query2.Bind(&results2[0], &id);					//mg_profiles.id

		if(!query2.StmtExecute(params2, results2))
		{
			DatabaseLog("CreateUserProfile() failed:");
			query2Success = false;
			count = 0;
		}
		else
		{
			query2Success = true;
			count = (ulong)mysql_stmt_num_rows(query2.GetStatement());
			DatabaseLog("found %d profiles", count);
		}
	}

	//Step 3: if there is only one profile for the account, set the only profile as the active profile
	if(count == 1)
	{
		char *sql3 = "UPDATE mg_accounts SET activeprofileid = ? WHERE id = ?";

		MySQLQuery query3(this->m_Connection, sql3);
		MYSQL_BIND params3[2];

		query3.Bind(&params3[0], &last_id);			//profileid
		query3.Bind(&params3[1], &accountId);		//account id
		
		if(!query3.StmtExecute(params3))
		{
			DatabaseLog("set active profile failed");
			query3Success = false;
			last_id = 0;
		}
		else
		{
			DatabaseLog("set active profile to %d OK", last_id);
			query3Success = true;
		}
	}

	return query1Success && query2Success && query3Success;
}

bool MySQLDatabase::DeleteUserProfile(const uint accountId, const uint profileId, const char* email)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	//TODO: (when forum is implemented) 
	//dont delete the profile, just rename the profile name
	//set an 'isdeleted' flag to 1
	//disassociate the profile with the account

	//Step 1: just delete the profile
	char *sql = "DELETE FROM mg_profiles WHERE id = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1];
	ulong email_len;
	uint id;
	ulong count;
	bool query1Success, query2Success, query3Success;

	memset(param, 0, sizeof(param));

	query.Bind(&param[0], &profileId);		//profile id

	if (!query.StmtExecute(param))
	{
		DatabaseLog("DeleteUserProfile(query1) failed: profile id(%d)", email, profileId);
		query1Success = false;
	}
	else
	{
		DatabaseLog("%s deleted profile id(%d)", email, profileId);
		query1Success = true;
	}

	//Step 2: get profile count for current account
	//if(query1Success)
	//{
		char *sql2 = "SELECT mg_profiles.id "
					"FROM mg_profiles "
					"JOIN mg_accounts "
					"ON mg_profiles.accountid = mg_accounts.id "
					"WHERE mg_accounts.email = ? "
					"ORDER BY lastlogindate DESC, id ASC";

		MySQLQuery query2(this->m_Connection, sql2);
		MYSQL_BIND params2[1], results2[1];

		memset(params2, 0, sizeof(params2));
		memset(results2, 0, sizeof(results2));

		query2.Bind(&params2[0], email, &email_len);	//email

		query2.Bind(&results2[0], &id);					//mg_profiles.id

		if(!query2.StmtExecute(params2, results2))
		{
			DatabaseLog("DeleteUserProfile(query2) failed: account id(%d), profile id(%d)", accountId, profileId);
			query2Success = false;
			count = 0;
		}
		else
		{
			query2Success = true;
			count = (ulong)mysql_stmt_num_rows(query2.GetStatement());
		}
	//}

	//Step 3: set active profile to last used profile id, if there are no profiles set active profile id to 0
	if(count > 0)
	{

		//fetch first row from query2, this is the last used id, as it is sorted by date
		query2.StmtFetch();

		char *sql3 = "UPDATE mg_accounts SET activeprofileid = ? WHERE id = ?";

		MySQLQuery query3(this->m_Connection, sql3);
		MYSQL_BIND params3[2];

		query3.Bind(&params3[0], &id);				//last used id
		query3.Bind(&params3[1], &accountId);		//account id
		
		if(!query3.StmtExecute(params3))
		{
			DatabaseLog("DeleteUserProfile(query3) failed: account id(%d), profile id(%d)", accountId, profileId);
			query3Success = false;
		}
		else
		{
			DatabaseLog("account id(%d) set active profile id(0)", accountId);
			query3Success = true;
		}
	}
	else
	{
		char *sql3 = "UPDATE mg_accounts SET activeprofileid = 0 WHERE id = ?";

		MySQLQuery query3(this->m_Connection, sql3);
		MYSQL_BIND params3[1];

		query3.Bind(&params3[0], &accountId);		//account id
		
		if(!query3.StmtExecute(params3))
		{
			DatabaseLog("DeleteUserProfile(query3) failed: account id(%d), profile id(%d)", accountId, profileId);
			query3Success = false;
		}
		else
		{
			DatabaseLog("account id(%d) set active profile id(0)", accountId);
			query3Success = true;
		}
	}

	return query1Success && query2Success && query3Success;
}

bool MySQLDatabase::QueryUserProfile(const uint accountId, const uint profileId, MMG_Profile *profile, MMG_Options *options)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	//Step 1: get the requested profile
	char *sql = "SELECT id, name, rank, clanid, rankinclan, commoptions FROM mg_profiles WHERE id = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], results[6];
	uint id, clanid, commoptions;
	uchar rank, rankinclan;
	ulong name_len;
	wchar_t name[WIC_NAME_MAX_LENGTH];

	bool query1Success, query2Success, query3Success;

	memset(name, 0, sizeof(name));

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], &profileId);			//profile id

	query.Bind(&results[0], &id);				//mg_profiles.id
	query.Bind(&results[1], name, &name_len);	//mg_profiles.name
	query.Bind(&results[2], &rank);				//mg_profiles.rank
	query.Bind(&results[3], &clanid);			//mg_profiles.clanid
	query.Bind(&results[4], &rankinclan);		//mg_profiles.rankinclan
	query.Bind(&results[5], &commoptions);		//mg_profiles.commoptions

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryUserProfile(query) failed: profile id(%d)", profileId);
		query1Success = false;

		//profile
		profile->m_ProfileId = 0;
		wcscpy_s(profile->m_Name, L"");
		profile->m_Rank = 0;
		profile->m_ClanId = 0;
		profile->m_RankInClan = 0;

		//communication options
		options->FromUInt(0);
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile id(%d) not found", profileId);
			query1Success = true;

			//profile
			profile->m_ProfileId = 0;
			wcscpy_s(profile->m_Name, L"");
			profile->m_Rank = 0;
			profile->m_ClanId = 0;
			profile->m_RankInClan = 0;

			//communication options
			options->FromUInt(0);
		}
		else
		{
			DatabaseLog("profile id(%d) %ws found", profileId, name);
			query1Success = true;

			//profile
			profile->m_ProfileId = id;
			wcscpy_s(profile->m_Name, name);
			profile->m_Rank = rank;
			profile->m_ClanId = clanid;
			profile->m_RankInClan = rankinclan;

			//communication options
			options->FromUInt(commoptions);
		}
	}

	//Step 2: update last login date for the requested profile
	if(query1Success)
	{
		char *sql2 = "UPDATE mg_profiles SET lastlogindate = NOW() WHERE id = ?";

		MySQLQuery query2(this->m_Connection, sql2);
		MYSQL_BIND param2[1];

		query2.Bind(&param2[0], &profile->m_ProfileId);		//profile id
		
		if(!query2.StmtExecute(param2))
		{
			DatabaseLog("QueryUserProfile(query2) failed: profile id(%d)", profile->m_ProfileId);
			query2Success = false;
		}
		else
		{
			DatabaseLog("profile id(%d) %ws lastlogindate updated", profile->m_ProfileId, profile->m_Name);
			query2Success = true;
		}
	}

	//Step 3: set new requested profile as active for the account
	if(query2Success)
	{
		char *sql3 = "UPDATE mg_accounts SET activeprofileid = ? WHERE id = ?";

		MySQLQuery query3(this->m_Connection, sql3);
		MYSQL_BIND params3[2];

		query3.Bind(&params3[0], &profile->m_ProfileId);		//profileid
		query3.Bind(&params3[1], &accountId);					//account id
		
		if(!query3.StmtExecute(params3))
		{
			DatabaseLog("QueryUserProfile(query3) failed: account id(%d), profile id(%d)", accountId, profile->m_ProfileId);
			query3Success = false;
		}
		else
		{
			DatabaseLog("account id(%d) set active profile id(%d) %ws", accountId, profile->m_ProfileId, profile->m_Name);
			query3Success = true;
		}
	}

	return query1Success && query2Success && query3Success;
}

bool MySQLDatabase::RetrieveUserProfiles(const char *email, const wchar_t *password, ulong *dstProfileCount, MMG_Profile *profiles[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT mg_profiles.id, mg_profiles.name, mg_profiles.rank, mg_profiles.clanid, mg_profiles.rankinclan, mg_profiles.onlinestatus "
				"FROM mg_profiles "
				"JOIN mg_accounts "
				"ON mg_profiles.accountid = mg_accounts.id "
				"WHERE mg_accounts.email = ? "
				"ORDER BY lastlogindate DESC, id ASC";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], results[6];
	ulong name_len, email_len;
	uint id, clanid, onlinestatus;
	uchar rank, rankinclan;
	wchar_t name[WIC_NAME_MAX_LENGTH];
	bool querySuccess;

	memset(name, 0, sizeof(name));

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], email, &email_len);		//email

	query.Bind(&results[0], &id);					//mg_profiles.id
	query.Bind(&results[1], name, &name_len);		//mg_profiles.name
	query.Bind(&results[2], &rank);					//mg_profiles.rank
	query.Bind(&results[3], &clanid);				//mg_profiles.clanid
	query.Bind(&results[4], &rankinclan);			//mg_profiles.rankinclan
	query.Bind(&results[5], &onlinestatus);			//mg_profiles.onlinestatus, do we need this in the database?

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("RetrieveUserProfiles() query failed: %s", email);
		querySuccess = false;

		MMG_Profile *tmp = new MMG_Profile();

		tmp->m_ProfileId = 0;
		wcscpy_s(tmp->m_Name, L"");
		tmp->m_Rank = 0;
		tmp->m_ClanId = 0;
		tmp->m_RankInClan = 0;
		tmp->m_OnlineStatus = 0;

		*profiles = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)mysql_stmt_num_rows(query.GetStatement());
		DatabaseLog("%s: %d profiles found", email, count);
		querySuccess = true;

		if (count < 1)
		{
			MMG_Profile *tmp = new MMG_Profile();

			tmp->m_ProfileId = 0;
			wcscpy_s(tmp->m_Name, L"");
			tmp->m_Rank = 0;
			tmp->m_ClanId = 0;
			tmp->m_RankInClan = 0;
			tmp->m_OnlineStatus = 0;

			*profiles = tmp;
			*dstProfileCount = 0;
		}
		else
		{
			MMG_Profile *tmp = new MMG_Profile[count];
			int i = 0;

			while(query.StmtFetch())
			{
				tmp[i].m_ProfileId = id;
				wcscpy_s(tmp[i].m_Name, name);
				tmp[i].m_Rank = rank;
				tmp[i].m_ClanId = clanid;
				tmp[i].m_RankInClan = rankinclan;
				tmp[i].m_OnlineStatus = onlinestatus;

				i++;
			}

			*profiles = tmp;
			*dstProfileCount = count;
		}
	}
	
	return querySuccess;
}

bool MySQLDatabase::QueryUserOptions(const uint profileId, int *options)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	//TODO
	return true;
}

bool MySQLDatabase::SaveUserOptions(const uint profileId, const int options)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "UPDATE mg_profiles SET commoptions = ? WHERE id = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND params[2];
	bool querySuccess;

	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &options);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
	{
		DatabaseLog("SaveUserOptions() failed: profile id(%d), options(%d)", profileId, options);
		querySuccess = false;
	}
	else
	{
		DatabaseLog("profile id(%d) set commoptions(%d)", profileId, options);
		querySuccess = true;
	}

	return querySuccess;
}

bool MySQLDatabase::QueryFriends(const uint profileId, uint *dstProfileCount, uint *friendIds[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT friendprofileid from mg_friends WHERE profileid = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], results[2];
	uint friendid;
	bool querySuccess;

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], &profileId);		//profileid

	query.Bind(&results[0], &friendid);		//mg_friends.friendprofileid

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryFriends() query failed: %s", profileId);
		querySuccess = false;

		uint *tmp = new uint();

		*tmp = 0;

		*friendIds = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)mysql_stmt_num_rows(query.GetStatement());
		DatabaseLog("profile id(%d), %d friends found", profileId, count);
		querySuccess = true;

		if (count < 1)
		{
			uint *tmp = new uint();

			*tmp = 0;

			*friendIds = tmp;
			*dstProfileCount = 0;
		}
		else
		{
			uint *tmp = new uint[count];
			int i = 0;

			while(query.StmtFetch())
			{
				tmp[i] = friendid;
				i++;
			}

			*friendIds = tmp;
			*dstProfileCount = count;
		}
	}

	return querySuccess;
}

bool MySQLDatabase::QueryAcquaintances(const uint profileId, uint *dstProfileCount, uint *acquaintanceIds[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT id from mg_profiles WHERE id <> ? LIMIT 100";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], results[1];
	uint id;
	bool querySuccess;

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], &profileId);			//temporary, will fix when acquaitances are implemented

	query.Bind(&results[0], &id);			//mg_profiles.id

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryAcquaintances() query failed: %s", profileId);
		querySuccess = false;

		uint *tmp = new uint();

		*tmp = 0;

		*acquaintanceIds = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)mysql_stmt_num_rows(query.GetStatement());
		DatabaseLog("profile id(%d), %d acquaintances found", profileId, count);
		querySuccess = true;

		if (count < 1)
		{
			uint *tmp = new uint();

			*tmp = 0;

			*acquaintanceIds = tmp;
			*dstProfileCount = 0;
		}
		else
		{
			uint *tmp = new uint[count];
			int i = 0;

			while(query.StmtFetch())
			{
				tmp[i] = id;
				i++;
			}

			*acquaintanceIds = tmp;
			*dstProfileCount = count;
		}
	}

	return querySuccess;
}

bool MySQLDatabase::QueryIgnoredProfiles(const uint profileId, uint *dstProfileCount, uint *ignoredIds[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileName (const uint profileId, MMG_Profile *profile)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
		return false;

	char *sql = "SELECT id, name, rank, clanid, rankinclan, onlinestatus FROM mg_profiles WHERE id = ?";

	MySQLQuery query(this->m_Connection, sql);
	MYSQL_BIND param[1], results[6];
	uint id, clanid, onlinestatus;
	uchar rank, rankinclan;
	ulong name_len;
	wchar_t name[WIC_NAME_MAX_LENGTH];

	bool querySuccess;

	memset(name, 0, sizeof(name));

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], &profileId);			//profile id

	query.Bind(&results[0], &id);				//mg_profiles.id
	query.Bind(&results[1], name, &name_len);	//mg_profiles.name
	query.Bind(&results[2], &rank);				//mg_profiles.rank
	query.Bind(&results[3], &clanid);			//mg_profiles.clanid
	query.Bind(&results[4], &rankinclan);		//mg_profiles.rankinclan
	query.Bind(&results[5], &onlinestatus);		//mg_profiles.onlinestatus

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryProfileName() failed: profile id(%d)", profileId);
		querySuccess = false;

		profile->m_ProfileId = 0;
		wcscpy_s(profile->m_Name, L"");
		profile->m_Rank = 0;
		profile->m_ClanId = 0;
		profile->m_RankInClan = 0;
		profile->m_OnlineStatus = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile id(%d) not found", profileId);
			querySuccess = true;

			profile->m_ProfileId = 0;
			wcscpy_s(profile->m_Name, L"");
			profile->m_Rank = 0;
			profile->m_ClanId = 0;
			profile->m_RankInClan = 0;
			profile->m_OnlineStatus = 0;
		}
		else
		{
			DatabaseLog("profile id(%d) %ws found", profileId, name);
			querySuccess = true;

			//profile
			profile->m_ProfileId = id;
			wcscpy_s(profile->m_Name, name);
			profile->m_Rank = rank;
			profile->m_ClanId = clanid;
			profile->m_RankInClan = rankinclan;
			profile->m_OnlineStatus = onlinestatus;
		}
	}

	return querySuccess;
}