#include "../stdafx.h"

#define DatabaseLog(format, ...) DebugLog(L_INFO, "[Database]: "format, __VA_ARGS__)

DWORD WINAPI MySQLDatabase::PingThread(LPVOID lpArg)
{
	return ((MySQLDatabase*)lpArg)->PingDatabase();
}

bool MySQLDatabase::EmergencyMassgateDisconnect()
{
	// set maintenance mode, server will most likely need attending if it gets to this point.
	DatabaseLog("connection to %s lost, check the MySQL server.", this->host);

	MMG_AccountProtocol::ourInstance->m_MaintenanceMode = true;
	SvClientManager::ourInstance->EmergencyDisconnectAll();
	return true;
}

bool MySQLDatabase::Initialize()
{
	// check if client version started
	const char *info = mysql_get_client_info();

	if (!info)
		return false;

	DatabaseLog("MySQL client version: %s started", info);

	// read mysql configuration from file
	if (!this->ReadConfig("settings.ini"))
		return false;

	if (this->ConnectDatabase())
	{
		DatabaseLog("Connected to server MySQL %s on '%s'", mysql_get_server_info(this->m_Connection), this->host);
		//this->PrintStatus();

		//create a thread to 'ping' the database every 5 hours, preventing timeout.
		this->m_PingThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PingThread, this, 0, NULL);
	}

	return true;
}

void MySQLDatabase::Unload()
{
	TerminateThread(this->m_PingThreadHandle, 0);
	this->m_PingThreadHandle = NULL;

	mysql_close(this->m_Connection);
	this->m_Connection = NULL;
	this->isConnected = false;
}

bool MySQLDatabase::ReadConfig(const char *filename)
{
	//might need to change working directory in property pages if it cant find the file
	FILE *file;
	file = fopen(filename, "r");

	if(!file)
	{
		DatabaseLog("could not open configuration file '%s'", filename);
		return false;
	}

	const int bufferlen = 64;		// characters read per line
	const int maxoptions = 64;		// max number of options read from the file
	int i = 0;

	char line[bufferlen];
	memset(line, 0, sizeof(line));

	char settings[maxoptions][bufferlen];
	memset(settings, 0, sizeof(settings));

	while (fgets(line, bufferlen, file))
	{
		// remove newline
		if (strchr(line, 10))
			line[strlen(line)-1] = 0;

		// ignoring standard comment characters and header. 91 = '[', 47 = '/'
		if (!strchr(line, 91) && !strchr(line, 47) && strlen(line))
		{
			strncpy(settings[i], line, sizeof(settings[i]));
			i++;
		}

		// reset the line buffer
		memset(line, 0, bufferlen);
	}

	fclose(file);

	strncpy(this->host,	settings[0], sizeof(this->host));
	strncpy(this->user,	settings[1], sizeof(this->user));
	strncpy(this->pass,	settings[2], sizeof(this->pass));
	strncpy(this->db,	settings[3], sizeof(this->db));

	strncpy(this->TABLENAME[UTILS_TABLE],			strstr(settings[4], "=") + 1, sizeof(this->TABLENAME[UTILS_TABLE]));
	strncpy(this->TABLENAME[ACCOUNTS_TABLE],		strstr(settings[5], "=") + 1, sizeof(this->TABLENAME[ACCOUNTS_TABLE]));
	strncpy(this->TABLENAME[CDKEYS_TABLE],			strstr(settings[6], "=") + 1, sizeof(this->TABLENAME[CDKEYS_TABLE]));
	strncpy(this->TABLENAME[PROFILES_TABLE],		strstr(settings[7], "=") + 1, sizeof(this->TABLENAME[PROFILES_TABLE]));
	strncpy(this->TABLENAME[FRIENDS_TABLE],			strstr(settings[8], "=") + 1, sizeof(this->TABLENAME[FRIENDS_TABLE]));
	strncpy(this->TABLENAME[IGNORED_TABLE],			strstr(settings[9], "=") + 1, sizeof(this->TABLENAME[IGNORED_TABLE]));
	strncpy(this->TABLENAME[MESSAGES_TABLE],		strstr(settings[10], "=") + 1, sizeof(this->TABLENAME[MESSAGES_TABLE]));
	strncpy(this->TABLENAME[ABUSEREPORTS_TABLE],	strstr(settings[11], "=") + 1, sizeof(this->TABLENAME[ABUSEREPORTS_TABLE]));
	strncpy(this->TABLENAME[CLANS_TABLE],			strstr(settings[12], "=") + 1, sizeof(this->TABLENAME[CLANS_TABLE]));

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
	mysql_options(this->m_Connection, MYSQL_INIT_COMMAND, "SET autocommit=1");

	// attempt connection to the database
	if (mysql_real_connect(this->m_Connection, this->host, this->user, this->pass, this->db, 0, NULL, CLIENT_REMEMBER_OPTIONS) == NULL)
	{
		DebugLog(L_WARN, "[Database]: %s", mysql_error(this->m_Connection));

		mysql_close(this->m_Connection);
		this->m_Connection = NULL;
		this->isConnected = false;

		return false;
	}

	this->isConnected = true;

	return true;
}

bool MySQLDatabase::HasConnection()
{
	return (this->m_Connection != NULL) && this->isConnected;
}

void MySQLDatabase::PrintStatus()
{
	if (!this->HasConnection())
		return;

	//connection type
	const char *info = mysql_get_host_info(this->m_Connection);

	const char *find = "via ";
	size_t pos = (size_t)(strstr(info, find) - info) + strlen(find);

	const char *hostname = this->host;
	const char *type = info+pos;

	// server version
	const char *version = mysql_get_server_info(this->m_Connection);

	// character set info
	const char *charsetname = mysql_character_set_name(this->m_Connection);

	MY_CHARSET_INFO charsetinfo;
	mysql_get_character_set_info(this->m_Connection, &charsetinfo);

	printf("\n");
	printf("Connection Status\n");
	printf("-----------------------------------\n");
	printf("Hostname:         %s\n", hostname);
	printf("Type:             %s\n", type);
	printf("Version:          MySQL %s\n", version);
	printf("Current Charset:  %s\n", charsetname);
	printf("-----------------------------------\n");
	printf("Default Client Charset\n");
	printf("Charset:          %s\n", charsetinfo.name);
	printf("Collation:        %s\n", charsetinfo.csname);
	printf("-----------------------------------\n");
}

bool MySQLDatabase::TestDatabase()
{
	MYSQL_RES *result = NULL;

	bool error_flag = false;

	if (!this->HasConnection())
		error_flag = true;

	if (!error_flag)
	{
		char SQL[4096];
		memset(SQL, 0, sizeof(SQL));

		// build sql query using table names defined in settings file
		sprintf(SQL, "SELECT poll FROM %s LIMIT 1", TABLENAME[UTILS_TABLE]);

		// execute a standard query on the database
		if (mysql_real_query(this->m_Connection, SQL, strlen(SQL)))
			error_flag = true;

		// must call mysql_store_result after calling mysql_real_query
		if (!error_flag)
			result = mysql_store_result(this->m_Connection);

		if (!result)
			error_flag = true;
		else
		{
			// if there is only one field, query has executed successfully, no need to check the row
			if(mysql_num_fields(result) != 1)
				error_flag = true;

			mysql_free_result(result);
		}
	}

	if (error_flag)
	{
		this->isConnected = false;
		return false;
	}

	this->isConnected = true;
	return true;
}

bool MySQLDatabase::PingDatabase()
{
	while (true)
	{
		Sleep(this->sleep_interval_ms);

		if (mysql_ping(this->m_Connection) == 0)
		{
			this->isConnected = true;
			MMG_AccountProtocol::ourInstance->m_MaintenanceMode = false;
		}
		else
		{
			//connection lost
			//mysql_ping will attempt to reconnect automatically, see notes in header.

			DatabaseLog("connection to %s lost, attempting reconnect", this->host);

			this->isConnected = false;
			MMG_AccountProtocol::ourInstance->m_MaintenanceMode = true;
		}
	}
	
	return true;
}

void MySQLDatabase::BeginTransaction()
{
	mysql_real_query(this->m_Connection, "SET AUTOCOMMIT=0", 16);
	mysql_real_query(this->m_Connection, "START TRANSACTION", 17);
}

void MySQLDatabase::RollbackTransaction()
{
	mysql_real_query(this->m_Connection, "ROLLBACK", 8);
}

void MySQLDatabase::CommitTransaction()
{
	mysql_real_query(this->m_Connection, "COMMIT", 6);
	mysql_real_query(this->m_Connection, "SET AUTOCOMMIT=1", 16);
}

bool MySQLDatabase::CheckIfEmailExists(const char *email, uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id FROM %s WHERE email = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	ulong emailLength = strlen(email);
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], email, &emailLength);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfEmailExists() query failed: %s", email);
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("email %s not found", email);
			*dstId = 0;
		}
		else
		{
			DatabaseLog("email %s found", email);
			*dstId = id;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::CheckIfCDKeyExists(const ulong cipherKeys[], uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id FROM %s WHERE cipherkeys0 = ? AND cipherkeys1 = ? AND cipherkeys2 = ? AND cipherkeys3 = ? LIMIT 1", TABLENAME[CDKEYS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[4], result[1];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint id;

	// bind parameters to prepared statement
	query.Bind(&params[0], &cipherKeys[0]);
	query.Bind(&params[1], &cipherKeys[1]);
	query.Bind(&params[2], &cipherKeys[2]);
	query.Bind(&params[3], &cipherKeys[3]);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(params, result))
	{
		DatabaseLog("CheckIfCDKeyExists() query failed:");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("cipherkeys not found");
			*dstId = 0;
		}
		else
		{
			DatabaseLog("cipherkeys found");
			*dstId = id;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::CreateUserAccount(const char *email, const char *password, const char *country, const uchar *emailgamerelated, const uchar *acceptsemail, const ulong cipherKeys[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	// *Step 1: insert account* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "INSERT INTO %s (email, password, country, emailgamerelated, acceptsemail, activeprofileid, isbanned) VALUES (?, ?, ?, ?, ?, 0, 0)", TABLENAME[ACCOUNTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);

	// prepared statement binding structures
	MYSQL_BIND params1[5];

	// initialize (zero) bind structures
	memset(params1, 0, sizeof(params1));

	// query specific variables
	ulong emailLength = strlen(email);
	ulong passLength = strlen(password);
	ulong countryLength = strlen(country);
	uint account_insert_id;

	// bind parameters to prepared statement
	query1.Bind(&params1[0], email, &emailLength);
	query1.Bind(&params1[1], password, &passLength);
	query1.Bind(&params1[2], country, &countryLength);
	query1.Bind(&params1[3], emailgamerelated);
	query1.Bind(&params1[4], acceptsemail);

	// execute prepared statement
	if (!query1.StmtExecute(params1))
	{
		DatabaseLog("CreateUserAccount() query1 failed: %s", email);
		account_insert_id = 0;
	}
	else
	{
		account_insert_id = (uint)query1.StmtInsertId();
		DatabaseLog("%s created an account id(%u)", email, account_insert_id);
	}

	if (!query1.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 2: insert cdkeys* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "INSERT INTO %s (accountid, cipherkeys0, cipherkeys1, cipherkeys2, cipherkeys3) VALUES (?, ?, ?, ?, ?)", TABLENAME[CDKEYS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND params2[5];

	// initialize (zero) bind structures
	memset(params2, 0, sizeof(params2));

	// bind parameters to prepared statement
	query2.Bind(&params2[0], &account_insert_id);
	query2.Bind(&params2[1], &cipherKeys[0]);
	query2.Bind(&params2[2], &cipherKeys[1]);
	query2.Bind(&params2[3], &cipherKeys[2]);
	query2.Bind(&params2[4], &cipherKeys[3]);

	// execute prepared statement
	if (!query2.StmtExecute(params2))
		DatabaseLog("CreateUserAccount() query2 failed: %s", email);
	else
		DatabaseLog("inserted cipherkeys for %s", email);

	if (!query2.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::AuthUserAccount(const char *email, char *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// *Step 1: retrieve account details* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file // AND password = ?
	sprintf(SQL1, "SELECT id, password, activeprofileid, isbanned FROM %s WHERE email = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);

	// prepared statement binding structures
	MYSQL_BIND param1[1], results1[4];

	// initialize (zero) bind structures
	memset(param1, 0, sizeof(param1));
	memset(results1, 0, sizeof(results1));

	// query specific variables
	ulong emailLength = strlen(email);
	
	char password[WIC_PASSWORDHASH_MAX_LENGTH];
	memset(password, 0, sizeof(password));
	ulong passLength = ARRAYSIZE(password);

	uint id, activeprofileid;
	uchar isbanned;

	// bind parameters to prepared statement
	query1.Bind(&param1[0], email, &emailLength);

	// bind results
	query1.Bind(&results1[0], &id);
	query1.Bind(&results1[1], password, &passLength);
	query1.Bind(&results1[2], &activeprofileid);
	query1.Bind(&results1[3], &isbanned);

	// execute prepared statement
	if(!query1.StmtExecute(param1, results1))
	{
		DatabaseLog("AuthUserAccount() query failed: %s", email);

		authToken->m_AccountId = 0;
		authToken->m_ProfileId = 0;
		//authToken->m_TokenId = 0;
		authToken->m_CDkeyId = 0;
		dstPassword = "";
		*dstIsBanned = 0;
	}
	else
	{
		if (!query1.StmtFetch())
		{
			DatabaseLog("account %s not found", email);

			authToken->m_AccountId = 0;
			authToken->m_ProfileId = 0;
			//authToken->m_TokenId = 0;
			authToken->m_CDkeyId = 0;
			dstPassword = "";
			*dstIsBanned = 0;
		}
		else
		{
			DatabaseLog("accountid(%u) %s found", id, email);

			authToken->m_AccountId = id;
			authToken->m_ProfileId = activeprofileid;
			//authToken->m_TokenId = 0;	// TODO
			authToken->m_CDkeyId = 0;	// fetched below in step 2
			strncpy(dstPassword, password, WIC_PASSWORDHASH_MAX_LENGTH);
			*dstIsBanned = isbanned;
		}
	}

	if (!query1.Success())
		return false;

	// *Step 2: retrieve account cdkeyid* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "SELECT id FROM %s WHERE accountid = ? LIMIT 1", TABLENAME[CDKEYS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND param2[1], result2[1];

	// initialize (zero) bind structures
	memset(param2, 0, sizeof(param2));
	memset(result2, 0, sizeof(result2));

	// query specific variables
	uint cdkeyid;

	// bind parameters to prepared statement
	query2.Bind(&param2[0], &id);

	// bind results
	query2.Bind(&result2[0], &cdkeyid);

	// execute prepared statement
	if(!query2.StmtExecute(param2, result2))
	{
		DatabaseLog("AuthUserAccount() query2 failed: %s", email);
		authToken->m_CDkeyId = 0;
	}
	else
	{
		if (!query2.StmtFetch())
		{
			DatabaseLog("cdkeyid %s not found", email);
			authToken->m_CDkeyId = 0;
		}
		else
		{
			DatabaseLog("cdkeyid(%u) %s found", id, email);
			authToken->m_CDkeyId = cdkeyid;
		}
	}

	if (!query2.Success())
		return false;

	return true;
}

bool MySQLDatabase::CheckIfProfileExists(const wchar_t* name, uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id FROM %s WHERE name = ? AND isdeleted = 0 LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	ulong nameLength = wcslen(name);
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], name, &nameLength);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfProfileExists() query failed: %ws", name);
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile %ws not found", name);
			*dstId = 0;
		}
		else
		{
			DatabaseLog("profile %ws already exists", name);
			*dstId = id;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::CreateUserProfile(const uint accountId, const wchar_t* name, const char* email)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();
	
	// *Step 1: create the profile as usual* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "INSERT INTO %s (accountid, name, rank, clanid, rankinclan, isdeleted, commoptions, lastlogindate, motto, homepage) VALUES (?, ?, 0, 0, 0, 0, 992, 0, NULL, NULL)", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);
	
	// prepared statement binding structures
	MYSQL_BIND params1[2];

	// initialize (zero) bind structures
	memset(params1, 0, sizeof(params1));

	// query specific variables
	ulong nameLength = wcslen(name);
	uint profile_insert_id;

	// bind parameters to prepared statement
	query1.Bind(&params1[0], &accountId);
	query1.Bind(&params1[1], name, &nameLength);

	// execute prepared statement
	if (!query1.StmtExecute(params1))
	{
		DatabaseLog("%s create profile '%ws' failed for account id %u", email, name, accountId);
		profile_insert_id = 0;
	}
	else
	{
		DatabaseLog("%s created profile %ws", email, name);
		profile_insert_id = (uint)query1.StmtInsertId();
	}

	if (!query1.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 2: get profile count for current account* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "SELECT id FROM %s WHERE accountid = ? AND isdeleted = 0 ORDER BY lastlogindate DESC, id ASC LIMIT 5", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND param2[1], result2[1];

	// initialize (zero) bind structures
	memset(param2, 0, sizeof(param2));
	memset(result2, 0, sizeof(result2));

	// query specific variables
	uint id;
	ulong profileCount;

	// bind parameters to prepared statement
	query2.Bind(&param2[0], &accountId);

	// bind results
	query2.Bind(&result2[0], &id);

	// execute prepared statement
	if(!query2.StmtExecute(param2, result2))
	{
		DatabaseLog("CreateUserProfile() query2 failed: %s", email);
		profileCount = 0;
	}
	else
	{
		profileCount = (ulong)query2.StmtNumRows();
		DatabaseLog("%s found %u profiles", email, profileCount);
	}

	if (!query2.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 3: if this is the only profile for the account, set it as the active profile* //

	if (profileCount == 1)
	{
		char SQL3[4096];
		memset(SQL3, 0, sizeof(SQL3));

		// build sql query using table names defined in settings file
		sprintf(SQL3, "UPDATE %s SET activeprofileid = ? WHERE id = ?", TABLENAME[ACCOUNTS_TABLE]);

		// prepared statement wrapper object
		MySQLQuery query3(this->m_Connection, SQL3);

		// prepared statement binding structures
		MYSQL_BIND params3[2];

		// initialize (zero) bind structures
		memset(params3, 0, sizeof(params3));

		// bind parameters to prepared statement
		query3.Bind(&params3[0], &profile_insert_id);
		query3.Bind(&params3[1], &accountId);
		
		// execute prepared statement
		if(!query3.StmtExecute(params3))
			DatabaseLog("%s set active profile failed", email);
		else
			DatabaseLog("%s set active profile to profileid(%u)", email, profile_insert_id);

		if (!query3.Success())
		{
			this->RollbackTransaction();
			return false;
		}
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::DeleteUserProfile(const uint accountId, const uint profileId, const char* email)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	// *Step 1: set the isdeleted field to 1, dis-associating the profile from the account* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "UPDATE %s SET isdeleted = 1 WHERE id = ?", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);

	// prepared statement binding structures
	MYSQL_BIND param1[1];

	// initialize (zero) bind structures
	memset(param1, 0, sizeof(param1));

	// bind parameters to prepared statement
	query1.Bind(&param1[0], &profileId);

	// execute prepared statement
	if (!query1.StmtExecute(param1))
		DatabaseLog("DeleteUserProfile(query1) failed: %s, profile id(%u)", email, profileId);
	else
		DatabaseLog("%s deleted profile id(%u)", email, profileId);

	if (!query1.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 2: get profile count for current account* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "SELECT id FROM %s WHERE accountid = ? AND isdeleted = 0 ORDER BY lastlogindate DESC, id ASC LIMIT 5", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND param2[1], result2[1];

	// initialize (zero) bind structures
	memset(param2, 0, sizeof(param2));
	memset(result2, 0, sizeof(result2));

	// query specific variables
	uint id;
	ulong profileCount;

	// bind parameters to prepared statement
	query2.Bind(&param2[0], &accountId);

	// bind results
	query2.Bind(&result2[0], &id);

	// execute prepared statement
	if(!query2.StmtExecute(param2, result2))
	{
		DatabaseLog("DeleteUserProfile(query2) failed: %s", email);
		profileCount = 0;
	}
	else
	{
		profileCount = (ulong)query2.StmtNumRows();
		DatabaseLog("%s found %u profiles", email, profileCount);
	}

	if (!query2.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 3: set active profile to last used profile id, if there are no profiles set active profile id to 0* //
	uint activeprofile;

	if(profileCount > 0)
	{
		//fetch first row from query2, this is the last used id, as it is sorted by date
		query2.StmtFetch();
		activeprofile = id;
	}
	else
		activeprofile = 0;

	char SQL3[4096];
	memset(SQL3, 0, sizeof(SQL3));

	// build sql query using table names defined in settings file
	sprintf(SQL3, "UPDATE %s SET activeprofileid = ? WHERE id = ?", TABLENAME[ACCOUNTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query3(this->m_Connection, SQL3);

	// prepared statement binding structures
	MYSQL_BIND params3[2];

	// initialize (zero) bind structures
	memset(params3, 0, sizeof(params3));

	// bind parameters to prepared statement
	query3.Bind(&params3[0], &activeprofile);
	query3.Bind(&params3[1], &accountId);
		
	// execute prepared statement
	if(!query3.StmtExecute(params3))
		DatabaseLog("DeleteUserProfile(query3) failed: account id(%u), profile id(%u)", email, accountId, activeprofile);
	else
		DatabaseLog("%s set active profile id(%u)", email, activeprofile);

	if (!query3.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 4: delete friends list for profile id and delete profile id from all friends lists* //

	char SQL4[4096];
	memset(SQL4, 0, sizeof(SQL4));

	// build sql query using table names defined in settings file
	sprintf(SQL4, "DELETE FROM %s WHERE profileid = ? OR friendprofileid = ?", TABLENAME[FRIENDS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query4(this->m_Connection, SQL4);

	// prepared statement binding structures
	MYSQL_BIND params4[2];

	// initialize (zero) bind structures
	memset(params4, 0, sizeof(params4));

	// bind parameters to prepared statement
	query4.Bind(&params4[0], &profileId);
	query4.Bind(&params4[1], &profileId);

	// execute prepared statement
	if (!query4.StmtExecute(params4))
		DatabaseLog("DeleteUserProfile(query4) failed: profile id(%u)", email, profileId);
	else
		DatabaseLog("%s deleted friends list for profile id(%u)", email, profileId);

	if (!query4.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 5: delete ignore list for profile id and delete profile id from all ignore lists* //

	char SQL5[4096];
	memset(SQL5, 0, sizeof(SQL5));

	// build sql query using table names defined in settings file
	sprintf(SQL5, "DELETE FROM %s WHERE profileid = ? OR ignoredprofileid = ?", TABLENAME[IGNORED_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query5(this->m_Connection, SQL5);

	// prepared statement binding structures
	MYSQL_BIND params5[2];

	// initialize (zero) bind structures
	memset(params5, 0, sizeof(params5));

	// bind parameters to prepared statement
	query5.Bind(&params5[0], &profileId);
	query5.Bind(&params5[1], &profileId);

	// execute prepared statement
	if (!query5.StmtExecute(params5))
		DatabaseLog("DeleteUserProfile(query5) failed: profile id(%u)", email, profileId);
	else
		DatabaseLog("%s deleted ignore list for profile id(%u)", email, profileId);

	if (!query5.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 6: delete pending and sent messages for profile

	char SQL6[4096];
	memset(SQL6, 0, sizeof(SQL6));

	// build sql query using table names defined in settings file
	sprintf(SQL6, "DELETE FROM %s WHERE senderprofileid = ? OR recipientprofileid = ?", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query6(this->m_Connection, SQL6);

	// prepared statement binding structures
	MYSQL_BIND params6[2];

	// initialize (zero) bind structures
	memset(params6, 0, sizeof(params6));

	// bind parameters to prepared statement
	query6.Bind(&params6[0], &profileId);
	query6.Bind(&params6[1], &profileId);

	// execute prepared statement
	if (!query6.StmtExecute(params6))
		DatabaseLog("DeleteUserProfile(query6) failed: profile id(%u)", email, profileId);
	else
		DatabaseLog("%s deleted all messages for/from profile id(%u)", email, profileId);

	if (!query6.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryUserProfile(const uint accountId, const uint profileId, MMG_Profile *profile)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	// *Step 1: get the requested profile* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "SELECT id, name, rank, clanid, rankinclan FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);
	
	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);

	// prepared statement binding structures
	MYSQL_BIND param1[1], results1[5];

	// initialize (zero) bind structures
	memset(param1, 0, sizeof(param1));
	memset(results1, 0, sizeof(results1));

	// query specific variables
	wchar_t name[WIC_NAME_MAX_LENGTH];
	memset(name, 0, sizeof(name));
	ulong nameLength = ARRAYSIZE(name);

	uint id, clanid;
	uchar rank, rankinclan;

	// bind parameters to prepared statement
	query1.Bind(&param1[0], &profileId);

	// bind results
	query1.Bind(&results1[0], &id);
	query1.Bind(&results1[1], name, &nameLength);
	query1.Bind(&results1[2], &rank);
	query1.Bind(&results1[3], &clanid);
	query1.Bind(&results1[4], &rankinclan);

	// execute prepared statement
	if(!query1.StmtExecute(param1, results1))
	{
		DatabaseLog("QueryUserProfile(query1) failed: accountid(%u), profileid(%u)", accountId, profileId);

		profile->m_ProfileId = 0;
		wcsncpy(profile->m_Name, L"", WIC_NAME_MAX_LENGTH);
		profile->m_Rank = 0;
		profile->m_ClanId = 0;
		profile->m_RankInClan = 0;
		profile->m_OnlineStatus = 0;
	}
	else
	{
		if (!query1.StmtFetch())
		{
			DatabaseLog("profile id(%u) not found", profileId);

			profile->m_ProfileId = 0;
			wcsncpy(profile->m_Name, L"", WIC_NAME_MAX_LENGTH);
			profile->m_Rank = 0;
			profile->m_ClanId = 0;
			profile->m_RankInClan = 0;
			profile->m_OnlineStatus = 0;
		}
		else
		{
			DatabaseLog("profile id(%u) %ws found", profileId, name);

			profile->m_ProfileId = id;
			wcsncpy(profile->m_Name, name, WIC_NAME_MAX_LENGTH);
			profile->m_Rank = rank;
			profile->m_ClanId = clanid;
			profile->m_RankInClan = rankinclan;
			profile->m_OnlineStatus = 0;
		}
	}

	if (!query1.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 2: update last login date for the requested profile* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "UPDATE %s SET lastlogindate = ? WHERE id = ?", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND param2[2];

	// initialize (zero) bind structures
	memset(param2, 0, sizeof(param2));

	// query specific variables
	time_t local_timestamp = time(NULL);
	//struct tm* gtime = gmtime(&local_timestamp);
	//time_t utc_timestamp = mktime(gtime);

	uint logintime = local_timestamp;

	// bind parameters to prepared statement
	query2.Bind(&param2[0], &logintime);
	query2.Bind(&param2[1], &profile->m_ProfileId);
		
	// execute prepared statement
	if(!query2.StmtExecute(param2))
		DatabaseLog("QueryUserProfile(query2) failed: accountid(%u), profileid(%u)", accountId, profile->m_ProfileId);
	else
		DatabaseLog("profile id(%u) %ws lastlogindate updated", profile->m_ProfileId, profile->m_Name);

	if (!query2.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 3: set new requested profile as active for the account* //

	char SQL3[4096];
	memset(SQL3, 0, sizeof(SQL3));

	// build sql query using table names defined in settings file
	sprintf(SQL3, "UPDATE %s SET activeprofileid = ? WHERE id = ?", TABLENAME[ACCOUNTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query3(this->m_Connection, SQL3);

	// prepared statement binding structures
	MYSQL_BIND params3[2];

	// initialize (zero) bind structures
	memset(params3, 0, sizeof(params3));

	// bind parameters to prepared statement
	query3.Bind(&params3[0], &profile->m_ProfileId);
	query3.Bind(&params3[1], &accountId);
		
	// execute prepared statement
	if(!query3.StmtExecute(params3))
		DatabaseLog("QueryUserProfile(query3) failed: accountid(%u), profileid(%u)", accountId, profile->m_ProfileId);
	else
		DatabaseLog("account id(%u) set active profile id(%u) %ws", accountId, profile->m_ProfileId, profile->m_Name);

	if (!query3.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::RetrieveUserProfiles(const uint accountId, ulong *dstProfileCount, MMG_Profile *profiles[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id, name, rank, clanid, rankinclan FROM %s WHERE accountid = ? AND isdeleted = 0 ORDER BY lastlogindate DESC, id ASC LIMIT 5", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], results[5];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	wchar_t name[WIC_NAME_MAX_LENGTH];
	memset(name, 0, sizeof(name));
	ulong nameLength = ARRAYSIZE(name);

	uint id, clanid;
	uchar rank, rankinclan;

	// bind parameters to prepared statement
	query.Bind(&param[0], &accountId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], name, &nameLength);
	query.Bind(&results[2], &rank);
	query.Bind(&results[3], &clanid);
	query.Bind(&results[4], &rankinclan);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("RetrieveUserProfiles() failed: accountId(%u)", accountId);

		MMG_Profile *tmp = new MMG_Profile[1];

		tmp[0].m_ProfileId = 0;
		wcsncpy(tmp[0].m_Name, L"", WIC_NAME_MAX_LENGTH);
		tmp[0].m_Rank = 0;
		tmp[0].m_ClanId = 0;
		tmp[0].m_RankInClan = 0;
		tmp[0].m_OnlineStatus = 0;

		*profiles = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong profileCount = (ulong)query.StmtNumRows();
		DatabaseLog("accountid(%u): %u profiles found", accountId, profileCount);

		if (profileCount < 1)
		{
			MMG_Profile *tmp = new MMG_Profile[1];

			tmp[0].m_ProfileId = 0;
			wcsncpy(tmp[0].m_Name, L"", WIC_NAME_MAX_LENGTH);
			tmp[0].m_Rank = 0;
			tmp[0].m_ClanId = 0;
			tmp[0].m_RankInClan = 0;
			tmp[0].m_OnlineStatus = 0;

			*profiles = tmp;
			*dstProfileCount = 0;
		}
		else
		{
			MMG_Profile *tmp = new MMG_Profile[profileCount];
			int i = 0;

			while(query.StmtFetch())
			{
				tmp[i].m_ProfileId = id;
				wcsncpy(tmp[i].m_Name, name, WIC_NAME_MAX_LENGTH);
				tmp[i].m_Rank = rank;
				tmp[i].m_ClanId = clanid;
				tmp[i].m_RankInClan = rankinclan;
				tmp[i].m_OnlineStatus = 0;

				i++;
			}

			*profiles = tmp;
			*dstProfileCount = profileCount;
		}
	}
	
	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryUserOptions(const uint profileId, uint *options)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT commoptions FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], results[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	uint commoptions;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], &commoptions);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryUserOptions() failed: profileid(%u)", profileId);
		*options = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("options not found for profile id(%u)", profileId);
			*options = 0;
		}
		else
		{
			DatabaseLog("options found for profile id(%u)", profileId);
			*options = commoptions;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::SaveUserOptions(const uint profileId, const uint options)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "UPDATE %s SET commoptions = ? WHERE id = ?", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &options);
	query.Bind(&params[1], &profileId);

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("SaveUserOptions() failed: profileid(%u), options(%u)", profileId, options);
	else
		DatabaseLog("profile id(%u) set commoptions(%u)", profileId, options);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryFriends(const uint profileId, uint *dstProfileCount, uint *friendIds[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT friendprofileid FROM %s WHERE profileid = ? LIMIT 64", TABLENAME[FRIENDS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryFriends() failed: profileid(%u)", profileId);

		uint *tmp = new uint[1];

		tmp[0] = 0;

		*friendIds = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("profile id(%u), %u friends found", profileId, count);

		if (count < 1)
		{
			uint *tmp = new uint[1];

			tmp[0] = 0;

			*friendIds = tmp;
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

			*friendIds = tmp;
			*dstProfileCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AddFriend(const uint profileId, uint friendProfileId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "INSERT INTO %s (profileid, friendprofileid) VALUES (?, ?)", TABLENAME[FRIENDS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &friendProfileId);

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("AddFriend() failed: profileid(%u)", profileId);
	else
		DatabaseLog("profileid (%u) added friendprofileid(%u)", profileId, friendProfileId);
	
	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::RemoveFriend(const uint profileId, uint friendProfileId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "DELETE FROM %s WHERE profileid = ? AND friendprofileid = ?", TABLENAME[FRIENDS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &friendProfileId);

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("RemoveFriend() failed: profileid(%u)", profileId);
	else
		DatabaseLog("profileid (%u) remove friendprofileid(%u)", profileId, friendProfileId);
	
	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryAcquaintances(const uint profileId, uint *dstProfileCount, uint *acquaintanceIds[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// TODO:
	// this function is temporary, must fix when stats are implemented
	// currently only retrieving profiles that have logged in within the last 7 days

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file, AND lastlogindate > (UNIX_TIMESTAMP(UTC_TIMESTAMP())
	sprintf(SQL, "SELECT id FROM %s WHERE id <> ? AND isdeleted = 0 AND lastlogindate > (UNIX_TIMESTAMP(NOW()) - 604800) LIMIT 64", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];
	
	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryAcquaintances() failed: profileid(%u)", profileId);

		uint *tmp = new uint[1];

		tmp[0] = 0;

		*acquaintanceIds = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("profile id(%u), %u acquaintances found", profileId, count);

		if (count < 1)
		{
			uint *tmp = new uint[1];

			tmp[0] = 0;

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

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryIgnoredProfiles(const uint profileId, uint *dstProfileCount, uint *ignoredIds[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT ignoredprofileid FROM %s WHERE profileid = ? LIMIT 64", TABLENAME[IGNORED_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));
	
	// query specific variables
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryIgnoredProfiles() failed: profileid(%u)", profileId);

		uint *tmp = new uint[1];

		tmp[0] = 0;

		*ignoredIds = tmp;
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("profile id(%u), %u ignored profiles found", profileId, count);

		if (count < 1)
		{
			uint *tmp = new uint[1];

			tmp[0] = 0;

			*ignoredIds = tmp;
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

			*ignoredIds = tmp;
			*dstProfileCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AddIgnoredProfile(const uint profileId, uint ignoredProfileId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "INSERT INTO %s (profileid, ignoredprofileid) VALUES (?, ?)", TABLENAME[IGNORED_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &ignoredProfileId);

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("AddIgnoredProfile() failed: profileid(%u)", profileId);
	else
		DatabaseLog("profileid (%u) added ingoredprofileid(%u)", profileId, ignoredProfileId);
	
	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::RemoveIgnoredProfile(const uint profileId, uint ignoredProfileId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "DELETE FROM %s WHERE profileid = ? AND ignoredprofileid = ?", TABLENAME[IGNORED_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &ignoredProfileId);

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("RemoveIgnoredProfile() failed: profileid(%u)", profileId);
	else
		DatabaseLog("profileid (%u) remove ignoredprofileid(%u)", profileId, ignoredProfileId);
	
	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryProfileName(const uint profileId, MMG_Profile *profile)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id, name, rank, clanid, rankinclan FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], results[5];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	wchar_t name[WIC_NAME_MAX_LENGTH];
	memset(name, 0, sizeof(name));
	ulong nameLength = ARRAYSIZE(name);

	uint id, clanid;
	uchar rank, rankinclan;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], name, &nameLength);
	query.Bind(&results[2], &rank);
	query.Bind(&results[3], &clanid);
	query.Bind(&results[4], &rankinclan);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryProfileName() failed: profileid(%u)", profileId);

		profile->m_ProfileId = 0;
		wcsncpy(profile->m_Name, L"", WIC_NAME_MAX_LENGTH);
		profile->m_Rank = 0;
		profile->m_ClanId = 0;
		profile->m_RankInClan = 0;
		profile->m_OnlineStatus = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile id(%u) not found", profileId);

			profile->m_ProfileId = 0;
			wcsncpy(profile->m_Name, L"", WIC_NAME_MAX_LENGTH);
			profile->m_Rank = 0;
			profile->m_ClanId = 0;
			profile->m_RankInClan = 0;
			profile->m_OnlineStatus = 0;
		}
		else
		{
			DatabaseLog("profile id(%u) %ws found", profileId, name);

			profile->m_ProfileId = id;
			wcsncpy(profile->m_Name, name, WIC_NAME_MAX_LENGTH);
			profile->m_Rank = rank;
			profile->m_ClanId = clanid;
			profile->m_RankInClan = rankinclan;
			profile->m_OnlineStatus = 0;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileList(const size_t Count, const uint *profileIds, MMG_Profile *profiles)
{
	for (uint i = 0; i < Count; i++)
	{
		if (!this->QueryProfileName(profileIds[i], &profiles[i]))
			return false;
	}

	return true;
}

bool MySQLDatabase::QueryEditableVariables(const uint profileId, wchar_t *dstMotto, wchar_t *dstHomepage)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT motto, homepage FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[1], results[2];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	wchar_t motto[WIC_MOTTO_MAX_LENGTH], homepage[WIC_HOMEPAGE_MAX_LENGTH];
	memset(motto, 0, sizeof(motto));
	memset(homepage, 0, sizeof(homepage));

	ulong mottoLength = ARRAYSIZE(motto);
	ulong homepageLength = ARRAYSIZE(homepage);

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], motto, &mottoLength);
	query.Bind(&results[1], homepage, &homepageLength);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryEditableVariables() failed: profileid(%u)", profileId);

		dstMotto = L"";
		dstHomepage = L"";
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile id(%u) not found", profileId);

			dstMotto = L"";
			dstHomepage = L"";
		}
		else
		{
			DatabaseLog("profile id(%u) found", profileId);

			wcsncpy(dstMotto, motto, WIC_MOTTO_MAX_LENGTH);
			wcsncpy(dstHomepage, homepage, WIC_HOMEPAGE_MAX_LENGTH);
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::SaveEditableVariables(const uint profileId, const wchar_t *motto, const wchar_t *homepage)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "UPDATE %s SET motto = ?, homepage = ? WHERE id = ?", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[3];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	ulong mottoLength = wcslen(motto);
	ulong homepageLength = wcslen(homepage);

	// bind parameters to prepared statement
	// if either string is empty BindNull
	if (mottoLength)
		query.Bind(&params[0], motto, &mottoLength);
	else
		query.BindNull(&params[0]);

	if (homepageLength)
		query.Bind(&params[1], homepage, &homepageLength);
	else
		query.BindNull(&params[1]);

	query.Bind(&params[2], &profileId);

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("SaveEditableVariables() failed: profileid(%u)", profileId);
	else
		DatabaseLog("SaveEditableVariables() success: profileid(%u)", profileId);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryPendingMessages(const uint profileId, uint *dstMessageCount, MMG_InstantMessageListener::InstantMessage *messages[])
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));
	
	// TODO temporarily limiting returned pending messages to 20, real limit for the client may be 128

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id, writtenat, senderprofileid, recipientprofileid, message FROM %s WHERE recipientprofileid = ? ORDER BY writtenat ASC LIMIT 20", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], results[5];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));
	
	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));
	ulong msgLength = ARRAYSIZE(message);

	uint id, senderid, recipientid;
	uint writtenat;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], &writtenat);
	query.Bind(&results[2], &senderid);
	query.Bind(&results[3], &recipientid);
	query.Bind(&results[4], message, &msgLength);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryPendingMessages() failed: profileid(%u)", profileId);

		//messages = NULL;
		*dstMessageCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("found %u pending messages for profile id(%u)", count, profileId);

		*dstMessageCount = 0;

		if (count > 0)
		{
			MMG_InstantMessageListener::InstantMessage *tmp = new MMG_InstantMessageListener::InstantMessage[count];
			int i = 0;

			while(query.StmtFetch())
			{
				tmp[i].m_MessageId = id;
				tmp[i].m_WrittenAt = writtenat;
				this->QueryProfileName(senderid, &tmp[i].m_SenderProfile);
				tmp[i].m_RecipientProfile = recipientid;
				wcsncpy(tmp[i].m_Message, message, WIC_INSTANTMSG_MAX_LENGTH);

				i++;
			}

			*messages = tmp;
			*dstMessageCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AddInstantMessage(const uint profileId, MMG_InstantMessageListener::InstantMessage *message)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "INSERT INTO %s (writtenat, senderprofileid, recipientprofileid, message) VALUES (?, ?, ?, ?)", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[4];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	ulong msgLength = wcslen(message->m_Message);

	time_t local_timestamp = time(NULL);
	//struct tm* gtime = gmtime(&local_timestamp);
	//time_t utc_timestamp = mktime(gtime);

	message->m_WrittenAt = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &message->m_WrittenAt);
	query.Bind(&params[1], &message->m_SenderProfile.m_ProfileId);
	query.Bind(&params[2], &message->m_RecipientProfile);
	query.Bind(&params[3], message->m_Message, &msgLength);

	// execute prepared statement
	if (!query.StmtExecute(params))
	{
		DatabaseLog("AddInstantMessage() failed: profileid(%u)", profileId);
		message->m_MessageId = 0;
	}
	else
	{
		message->m_MessageId = (uint)query.StmtInsertId();
		//DatabaseLog("message id(%u) queued:", message->m_MessageId);
	}

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::RemoveInstantMessage(const uint profileId, uint messageId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "DELETE FROM %s WHERE id = ? AND recipientprofileid = ?", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[2];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));

	// bind parameters to prepared statement
	query.Bind(&param[0], &messageId);
	query.Bind(&param[1], &profileId);

	// execute prepared statement
	if (!query.StmtExecute(param))
		DatabaseLog("RemoveInstantMessage() failed: profileid(%u)", profileId);
	//else
	//	DatabaseLog("message id(%u) acked", messageId);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::AddAbuseReport(const uint profileId, const uint flaggedProfile, const wchar_t *report)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "INSERT INTO %s (senderprofileid, reportedprofileid, report, datereported) VALUES (?, ?, ?, ?)", TABLENAME[ABUSEREPORTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[4];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	ulong reportLength = wcslen(report);

	time_t local_timestamp = time(NULL);
	//struct tm* gtime = gmtime(&local_timestamp);
	//time_t utc_timestamp = mktime(gtime);

	uint datereported = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &flaggedProfile);
	query.Bind(&params[2], report, &reportLength);
	query.Bind(&params[3], &datereported);

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("AddAbuseReport() failed: profileid(%u)", profileId);
	else
		DatabaseLog("player id(%u) reported player id(%u):", profileId, flaggedProfile);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}