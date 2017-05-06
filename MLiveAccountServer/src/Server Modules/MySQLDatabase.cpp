#define _CRT_SECURE_NO_WARNINGS
#include "../stdafx.h"

#define DatabaseLog(format, ...) DebugLog(L_INFO, "[Database]: "format, __VA_ARGS__)

MySQLDatabase::MySQLDatabase()
{
	memset(this->TABLENAME, 0, sizeof(this->TABLENAME));
	memset(this->host, 0, sizeof(this->host));
	memset(this->user, 0, sizeof(this->user));
	memset(this->pass, 0, sizeof(this->pass));
	memset(this->db, 0, sizeof(this->db));
	memset(this->charset_name, 0, sizeof(this->charset_name));
	memset(this->bind_interface, 0, sizeof(this->bind_interface));
	this->auto_reconnect = 0;

	this->m_Connection = NULL;
	this->isConnected = false;

	// set default connection options in case they were left out of config
	strncpy(this->charset_name, "latin1", 6);		// latin1 default for mysql, translates to cp-1252
	strncpy(this->bind_interface, "0.0.0.0", 7);	// 0.0.0.0 binds port to all interfaces

	this->sleep_interval_h = 5; // connection timeout for a default mysql install is 28800 seconds (8 hours), so ping every 5 to keep it alive
	this->sleep_interval_m = this->sleep_interval_h * 60;
	this->sleep_interval_s = this->sleep_interval_m * 60;
	this->sleep_interval_ms = this->sleep_interval_s * 1000;

	this->m_PingThreadHandle = NULL;
}

MySQLDatabase::~MySQLDatabase()
{
	this->Unload();
}

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
	if (!ReadConfig("settings.ini"))
		return false;

	if (!ConnectDatabase())
		return false;

	DatabaseLog("Connected to server MySQL %s on '%s:%d'", mysql_get_server_info(this->m_Connection), this->host, this->m_Connection->port);

	if (!ReadDatabaseSchema())
		return false;

	if (!TestDatabaseTables())
		return false;

	if (!PrintDatabaseInfo())
		return false;

	if (!ResetOnlineStatus())
		return false;

	// create a thread to 'ping' the database every 5 hours, preventing timeout.
	this->m_PingThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) PingThread, this, 0, NULL);

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

	int i = 0, j = 0;

	char line[MAX_CONFIG_BUFFER_LENGTH];
	memset(line, 0, sizeof(line));

	char settings[MAX_CONFIG_OPTIONS*2][MAX_CONFIG_BUFFER_LENGTH];
	memset(settings, 0, sizeof(settings));

	while (fgets(line, MAX_CONFIG_BUFFER_LENGTH, file))
	{
		// remove newline
		if (strchr(line, 10))
			line[strlen(line)-1] = 0;

		// ignoring standard comment characters. 91 = '[', 47 = '/'
		if (!strchr(line, 47) && strlen(line))
		{
			strncpy(settings[i], line, sizeof(settings[i]));
			i++;
		}

		// reset the line buffer
		memset(line, 0, MAX_CONFIG_BUFFER_LENGTH);
	}

	fclose(file);

	while (j < i)
	{
		if (strstr(settings[j], "[hostname]") && !strchr(settings[j+1], 91))
			strncpy(this->host,	settings[++j], sizeof(this->host));

		if (strstr(settings[j], "[username]") && !strchr(settings[j+1], 91))
			strncpy(this->user,	settings[++j], sizeof(this->user));
		
		if (strstr(settings[j], "[password]") && !strchr(settings[j+1], 91))
			strncpy(this->pass,	settings[++j], sizeof(this->pass));
		
		if (strstr(settings[j], "[database]") && !strchr(settings[j+1], 91))
			strncpy(this->db,	settings[++j], sizeof(this->db));

		if (strstr(settings[j], "[charset]") && !strchr(settings[j+1], 91))
		{
			memset(this->charset_name, 0, sizeof(this->charset_name));
			strncpy(this->charset_name,	settings[++j], sizeof(this->charset_name));
		}

		if (strstr(settings[j], "[bind_interface]") && !strchr(settings[j+1], 91))
		{
			memset(this->bind_interface, 0, sizeof(this->bind_interface));
			strncpy(this->bind_interface, settings[++j], sizeof(this->bind_interface));
		}

		if (strstr(settings[j], "[auto_reconnect]") && !strchr(settings[j+1], 91))
		{
			if (strstr(settings[++j], "true"))
				this->auto_reconnect = 1;
			else
				this->auto_reconnect = 0;
		}

		j++;
	}

	return true;
}

bool MySQLDatabase::ConnectDatabase()
{
	//create the connection object
	this->m_Connection = mysql_init(NULL);

	//set the connection options
	mysql_options(this->m_Connection, MYSQL_SET_CHARSET_NAME, this->charset_name);
	mysql_options(this->m_Connection, MYSQL_OPT_BIND, this->bind_interface);
	mysql_options(this->m_Connection, MYSQL_OPT_RECONNECT, &this->auto_reconnect);
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

bool MySQLDatabase::ReadDatabaseSchema()
{
	if (!HasConnection())
		return false;

	MYSQL_RES *res = mysql_list_tables(this->m_Connection, NULL);
	int num_fields = mysql_num_fields(res);
	int i = 0;

	MYSQL_ROW row;
	while (row = mysql_fetch_row(res))
	{
		for (int j = 0; j < num_fields; j++)
			strncpy(this->TABLENAME[i], row[j], strlen(row[j]));
		i++;
	}

	mysql_free_result(res);

	return true;
}

bool MySQLDatabase::HasConnection()
{
	return (this->m_Connection != NULL) && this->isConnected;
}

bool MySQLDatabase::TestDatabaseTables()
{
	if (!HasConnection())
		return false;

	for (int i = 0; i < TOTAL_TABLES; i++)
	{
		char SQL[4096];
		memset(SQL, 0, sizeof(SQL));

		sprintf(SQL, "SELECT 1 FROM %s LIMIT 1", TABLENAME[i]);

		MySQLQuery query(this->m_Connection, SQL);

		if(!query.StmtExecute())
			DatabaseLog("TestDatabaseTables() failed:");
	
		if (!query.Success())
			return false;
	}

	return true;
}

bool MySQLDatabase::PrintDatabaseInfo()
{
	if (!HasConnection())
		return false;

	//connection type
	const char *info = mysql_get_host_info(this->m_Connection);

	const char *find = "via ";
	size_t pos = (size_t)(strstr(info, find) - info) + strlen(find);

	const char *hostname = this->m_Connection->host;
	const char *type = info+pos;

	// server version
	const char *version = mysql_get_server_info(this->m_Connection);

	// character set info
	const char *charsetname = mysql_character_set_name(this->m_Connection);

	MY_CHARSET_INFO charsetinfo;
	mysql_get_character_set_info(this->m_Connection, &charsetinfo);

	DatabaseLog("Client Charset:   %s", charsetinfo.name);
	DatabaseLog("Collation:        %s", charsetinfo.csname);
	DatabaseLog("");
	DatabaseLog("Server Info");
	DatabaseLog("-----------------------------------");
	DatabaseLog("Hostname:         %s:%d", hostname, this->m_Connection->port);
	DatabaseLog("Version:          MySQL %s", this->m_Connection->server_version);
	DatabaseLog("Type:             %s", type);
	DatabaseLog("Current Charset:  %s", charsetname);
	DatabaseLog("User:             %s", this->m_Connection->user);
	DatabaseLog("Database:         %s", this->m_Connection->db);
	DatabaseLog("");
	DatabaseLog("Tables in %s", this->m_Connection->db);

	for (int i = 0; i < TOTAL_TABLES; i++)
		DatabaseLog("%s", this->TABLENAME[i]);

	DatabaseLog("-----------------------------------");

	return true;
}

bool MySQLDatabase::ResetOnlineStatus()
{
	if (!HasConnection())
		return false;

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET onlinestatus = 0 WHERE onlinestatus <> 0", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);

	if(!query.StmtExecute())
		DatabaseLog("ResetOnlineStatus() failed:");
	
	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::TestDatabase()
{
	MYSQL_RES *result = NULL;

	bool error_flag = false;

	if (!this->HasConnection())
		error_flag = true;

	if (!error_flag)
	{
		// execute a standard query on the database
		if (mysql_real_query(this->m_Connection, "SELECT 1", 8))
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
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], email, strlen(email));

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

bool MySQLDatabase::CheckIfCDKeyExists(const uint sequenceNum, uint *dstId)
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
	sprintf(SQL, "SELECT id FROM %s WHERE sequencenum = ? LIMIT 1", TABLENAME[CDKEYS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[1], result[1];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint id;

	// bind parameters to prepared statement
	query.Bind(&params[0], &sequenceNum);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(params, result))
	{
		DatabaseLog("CheckIfCDKeyExists() failed:");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("sequence number not found");
			*dstId = 0;
		}
		else
		{
			DatabaseLog("sequence number found");
			*dstId = id;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::InsertUserAccount(const char *email, const char *password, const char *country, const char *realcountry, const uchar *emailgamerelated, const uchar *acceptsemail, uint *accountInsertId)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "INSERT INTO %s (email, password, country, realcountry, emailgamerelated, acceptsemail, membersince) VALUES (?, ?, ?, ?, ?, ?, ?)", TABLENAME[ACCOUNTS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[7];
	memset(params, 0, sizeof(params));

	time_t local_timestamp = time(NULL);
	uint membersince = local_timestamp;

	query.Bind(&params[0], email, strlen(email));
	query.Bind(&params[1], password, strlen(password));
	query.Bind(&params[2], country, strlen(country));
	query.Bind(&params[3], realcountry, strlen(realcountry));
	query.Bind(&params[4], emailgamerelated);
	query.Bind(&params[5], acceptsemail);
	query.Bind(&params[6], &membersince);

	if (!query.StmtExecute(params))
	{
		DatabaseLog("InsertUserAccount() failed: %s", email);
		*accountInsertId = 0;
	}
	else
	{
		*accountInsertId = (uint)query.StmtInsertId();
		DatabaseLog("%s created an account id(%u)", email, *accountInsertId);
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::InsertUserCDKeyInfo(const uint accountId, const uint sequenceNum, const ulong cipherKeys[])
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "INSERT INTO %s (accountid, sequencenum, cipherkeys0, cipherkeys1, cipherkeys2, cipherkeys3) VALUES (?, ?, ?, ?, ?, ?)", TABLENAME[CDKEYS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[6];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &accountId);
	query.Bind(&params[1], &sequenceNum);
	query.Bind(&params[2], &cipherKeys[0]);
	query.Bind(&params[3], &cipherKeys[1]);
	query.Bind(&params[4], &cipherKeys[2]);
	query.Bind(&params[5], &cipherKeys[3]);

	if (!query.StmtExecute(params))
		DatabaseLog("InsertUserCDKeyInfo() failed:");
	else
		DatabaseLog("account id(%u) inserted cipherkeys", accountId);

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::CreateUserAccount(const char *email, const char *password, const char *country, const char *realcountry, const uchar *emailgamerelated, const uchar *acceptsemail, const uint sequenceNum, const ulong cipherKeys[])
{
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	BeginTransaction();

	uint accountInsertId = 0;

	if (!InsertUserAccount(email, password, country, realcountry, emailgamerelated, acceptsemail, &accountInsertId))
	{
		RollbackTransaction();
		return false;
	}

	if (!InsertUserCDKeyInfo(accountInsertId, sequenceNum, cipherKeys))
	{
		RollbackTransaction();
		return false;
	}

	CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryUserAccount(const char *email, char *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// WHERE email = ? AND password = ?
	sprintf(SQL, "SELECT id, password, activeprofileid, isbanned FROM %s WHERE email = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], results[4];
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	char password[WIC_PASSWORDHASH_MAX_LENGTH];
	memset(password, 0, sizeof(password));

	uint id, activeprofileid;
	uchar isbanned;

	query.Bind(&param[0], email, strlen(email));

	query.Bind(&results[0], &id);
	query.Bind(&results[1], password, ARRAYSIZE(password));
	query.Bind(&results[2], &activeprofileid);
	query.Bind(&results[3], &isbanned);

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryUserAccount() failed: %s", email);

		authToken->m_AccountId = 0;
		authToken->m_ProfileId = 0;
		//authToken->m_TokenId = 0;
		authToken->m_CDkeyId = 0;
		dstPassword = "";
		*dstIsBanned = 0;
	}
	else
	{
		if (!query.StmtFetch())
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
			authToken->m_CDkeyId = 0;
			strncpy(dstPassword, password, WIC_PASSWORDHASH_MAX_LENGTH);
			*dstIsBanned = isbanned;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryUserCDKeyId(const uint accountId, MMG_AuthToken *authToken)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT id FROM %s WHERE accountid = ? LIMIT 1", TABLENAME[CDKEYS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], result[1];
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	uint cdkeyid;

	query.Bind(&param[0], &accountId);

	query.Bind(&result[0], &cdkeyid);

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryUserCDKeyId() failed:");
		authToken->m_CDkeyId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("cdkeyid not found");
			authToken->m_CDkeyId = 0;
		}
		else
		{
			DatabaseLog("cdkeyid(%u) found", cdkeyid);
			authToken->m_CDkeyId = cdkeyid;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AuthUserAccount(const char *email, char *dstPassword, uchar *dstIsBanned, MMG_AuthToken *authToken)
{
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	if (!QueryUserAccount(email, dstPassword, dstIsBanned, authToken))
		return false;

	if (!QueryUserCDKeyId(authToken->m_AccountId, authToken))
		return false;

	return true;
}

bool MySQLDatabase::UpdateRealCountry(const uint accountId, const char *realcountry)
{
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET realcountry = IF (realcountry = '', ?, realcountry) WHERE id = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[2];
	memset(param, 0, sizeof(param));

	query.Bind(&param[0], realcountry, strlen(realcountry));
	query.Bind(&param[1], &accountId);

	if(!query.StmtExecute(param))
		DatabaseLog("UpdateRealCountry() failed:");

	if (!query.Success())
	{
		RollbackTransaction();
		return false;
	}

	CommitTransaction();

	return true;
}

bool MySQLDatabase::UpdateCDKeyInfo(const uint accountId, const uint sequenceNum, const ulong cipherKeys[])
{
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET "
				 "sequencenum = IF (sequencenum = 0, ?, sequencenum), "
				 "cipherkeys0 = IF (? = sequencenum AND cipherkeys0 = 0, ?, cipherkeys0), "
				 "cipherkeys1 = IF (? = sequencenum AND cipherkeys1 = 0, ?, cipherkeys1), "
				 "cipherkeys2 = IF (? = sequencenum AND cipherkeys2 = 0, ?, cipherkeys2), "
				 "cipherkeys3 = IF (? = sequencenum AND cipherkeys3 = 0, ?, cipherkeys3) "
				 "WHERE accountid = ? LIMIT 1", TABLENAME[CDKEYS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[10];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &sequenceNum);
	query.Bind(&params[1], &sequenceNum);
	query.Bind(&params[2], &cipherKeys[0]);
	query.Bind(&params[3], &sequenceNum);
	query.Bind(&params[4], &cipherKeys[1]);
	query.Bind(&params[5], &sequenceNum);
	query.Bind(&params[6], &cipherKeys[2]);
	query.Bind(&params[7], &sequenceNum);
	query.Bind(&params[8], &cipherKeys[3]);
	query.Bind(&params[9], &accountId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateCDKeyInfo() failed:");

	if (!query.Success())
	{
		RollbackTransaction();
		return false;
	}

	CommitTransaction();

	return true;
}

bool MySQLDatabase::UpdatePassword(const uint accountId, const char *password)
{
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET password = ? WHERE id = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[2];
	memset(param, 0, sizeof(param));

	query.Bind(&param[0], password, strlen(password));
	query.Bind(&param[1], &accountId);

	if(!query.StmtExecute(param))
		DatabaseLog("UpdatePassword() failed:");

	if (!query.Success())
	{
		RollbackTransaction();
		return false;
	}

	CommitTransaction();

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
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], name, wcslen(name));

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfProfileExists() query failed:");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("profile not found");
			*dstId = 0;
		}
		else
		{
			DatabaseLog("profile already exists");
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
	sprintf(SQL1, "INSERT INTO %s (accountid, name, membersince, medaldata, badgedata) VALUES (?, ?, ?, ?, ?)", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);
	
	// prepared statement binding structures
	MYSQL_BIND params1[5];

	// initialize (zero) bind structures
	memset(params1, 0, sizeof(params1));

	// query specific variables
	uint profile_insert_id;
	time_t local_timestamp = time(NULL);
	uint membersince = local_timestamp;
	uint medalbuffer[256], badgebuffer[256];
	memset(medalbuffer, 0, sizeof(medalbuffer));
	memset(badgebuffer, 0, sizeof(badgebuffer));

	{
		MMG_Stats::Badge preorderBadge, recruitBadge;
		uchar isPreorder = 0;
		uint numFriendsRecruited = 0;

		if (!QueryPreorderNumRecruited(accountId, &isPreorder, &numFriendsRecruited))
		{
			RollbackTransaction();
			return false;
		}

		preorderBadge.level = isPreorder > 0 ? 1 : 0;
		preorderBadge.stars = isPreorder > 1 ? 1 : 0;
		preorderBadge.stars = isPreorder > 2 ? 2 : preorderBadge.stars;
		preorderBadge.stars = isPreorder > 3 ? 3 : preorderBadge.stars;

		recruitBadge.level = numFriendsRecruited > 0 ? 1 : 0;
		recruitBadge.level = numFriendsRecruited > 4 ? 2 : recruitBadge.level;
		recruitBadge.level = numFriendsRecruited > 9 ? 3 : recruitBadge.level;
		recruitBadge.stars = 0;

		MMG_BitWriter<unsigned int> writer(badgebuffer, sizeof(badgebuffer) * 8);

		for (int i = 0; i < 12; i++)
		{
			writer.WriteBits(0, 2);
			writer.WriteBits(0, 2);
		}

		writer.WriteBits(preorderBadge.level, 2);
		writer.WriteBits(preorderBadge.stars, 2);

		writer.WriteBits(recruitBadge.level, 2);
		writer.WriteBits(recruitBadge.stars, 2);
	}

	// bind parameters to prepared statement
	query1.Bind(&params1[0], &accountId);
	query1.Bind(&params1[1], name, wcslen(name));
	query1.Bind(&params1[2], &membersince);
	query1.BindBlob(&params1[3], medalbuffer, sizeof(medalbuffer));
	query1.BindBlob(&params1[4], badgebuffer, sizeof(badgebuffer));

	// execute prepared statement
	if (!query1.StmtExecute(params1))
	{
		DatabaseLog("%s create profile failed for account id %u", email, accountId);
		profile_insert_id = 0;
	}
	else
	{
		DatabaseLog("%s created new profile", email);
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
		sprintf(SQL3, "UPDATE %s SET activeprofileid = ? WHERE id = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

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

	// *Step 1: "delete" the profile (set the isdeleted field to 1) * //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "UPDATE %s SET isdeleted = 1 WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

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
		DatabaseLog("DeleteUserProfile() query1 failed: %s", email);
	else
		DatabaseLog("%s deleted profileid(%u)", email, profileId);

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
	if (!query2.StmtExecute(param2, result2))
	{
		DatabaseLog("DeleteUserProfile() query2 failed: %s", email);
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

	if (profileCount > 0)
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
	sprintf(SQL3, "UPDATE %s SET activeprofileid = ? WHERE id = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

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
	if (!query3.StmtExecute(params3))
		DatabaseLog("DeleteUserProfile() query3 failed: %s", email);
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
		DatabaseLog("DeleteUserProfile() query4 failed: %s", email);
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
		DatabaseLog("DeleteUserProfile() query5 failed: %s", email);
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
		DatabaseLog("DeleteUserProfile() query6 failed: %s", email);
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

bool MySQLDatabase::QueryProfileCreationDate(const uint profileId, uint *membersince)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT membersince FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], result[1];
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	query.Bind(&param[0], &profileId);

	query.Bind(&result[0], membersince);

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryProfileCreationDate() failed:");
		*membersince = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("QueryProfileCreationDate() account not found");
			*membersince = 0;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryPreorderNumRecruited(const uint accountId, uchar *isPreorder, uint *numFriendsRecruited)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT ispreorder, numfriendsrecruited FROM %s WHERE id = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], results[2];
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], &accountId);

	query.Bind(&results[0], isPreorder);
	query.Bind(&results[1], numFriendsRecruited);

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryPreorderNumRecruited() failed:");
		*isPreorder = 0;
		*numFriendsRecruited = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("QueryPreorderNumRecruited() account not found");
			*isPreorder = 0;
			*numFriendsRecruited = 0;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::UpdateMembershipBadges(const uint accountId, const uint profileId)
{
	MMG_Stats::Badge badges[14];
	uint membersince = 0;
	uchar isPreorder = 0;
	uint numFriendsRecruited = 0;
	uint readbuffer[256];
	memset(readbuffer, 0, sizeof(readbuffer));

	if (!QueryProfileBadgesRawData(profileId, readbuffer, sizeof(readbuffer)))
		return false;

	if (!QueryProfileCreationDate(profileId, &membersince))
		return false;

	if (!QueryPreorderNumRecruited(accountId, &isPreorder, &numFriendsRecruited))
		return false;

	MMG_BitReader<unsigned int> reader(readbuffer, sizeof(readbuffer) * 8);

	for (uint i = 0; i < 14; i++)
	{
		badges[i].level = reader.ReadBits(2);
		badges[i].stars = reader.ReadBits(2);
	}

	uint todaysdate = ROUND_TIME(time(NULL));
	uint signupdate = ROUND_TIME(membersince);

	double days = difftime(todaysdate, signupdate) / 86400;

	if (badges[7].level < 3)
	{
		badges[7].level = days >= 30 ? 1 : 0;
		badges[7].stars = days >= 60 ? 1 : 0;
		badges[7].stars = days >= 90 ? 0 : badges[7].stars;

		badges[7].level = days >= 90 ? 2 : badges[7].level;
		badges[7].stars = days >= 180 ? 1 : badges[7].stars;
		badges[7].stars = days >= 270 ? 2 : badges[7].stars;
		badges[7].stars = days >= 360 ? 3 : badges[7].stars;

		badges[7].level = days >= 365 ? 3 : badges[7].level;
		badges[7].stars = days >= 365 ? 0 : badges[7].stars;
	}

	if (badges[12].level < 1 || badges[12].stars < 3)
	{
		badges[12].level = isPreorder > 0 ? 1 : 0;
		badges[12].stars = isPreorder > 1 ? 1 : 0;
		badges[12].stars = isPreorder > 2 ? 2 : badges[12].stars;
		badges[12].stars = isPreorder > 3 ? 3 : badges[12].stars;
	}

	if (badges[13].level < 3)
	{
		badges[13].level = numFriendsRecruited > 0 ? 1 : 0;
		badges[13].level = numFriendsRecruited > 4 ? 2 : badges[13].level;
		badges[13].level = numFriendsRecruited > 9 ? 3 : badges[13].level;
		badges[13].stars = 0;
	}

	uint writebuffer[256];
	memset(writebuffer, 0, sizeof(writebuffer));

	MMG_BitWriter<unsigned int> writer(writebuffer, sizeof(writebuffer) * 8);

	for (uint i = 0; i < 14; i++)
	{
		writer.WriteBits(badges[i].level, 2);
		writer.WriteBits(badges[i].stars, 2);
	}

	if (memcmp(readbuffer, writebuffer, sizeof(writebuffer)))
	{
		if (!UpdateProfileBadgesRawData(profileId, writebuffer, sizeof(writebuffer)))
			return false;
	}

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

	uint id, clanid;
	uchar rank, rankinclan;

	// bind parameters to prepared statement
	query1.Bind(&param1[0], &profileId);

	// bind results
	query1.Bind(&results1[0], &id);
	query1.Bind(&results1[1], name, ARRAYSIZE(name));
	query1.Bind(&results1[2], &rank);
	query1.Bind(&results1[3], &clanid);
	query1.Bind(&results1[4], &rankinclan);

	// execute prepared statement
	if(!query1.StmtExecute(param1, results1))
	{
		DatabaseLog("QueryUserProfile() query1 failed: accountid(%u), profileid(%u)", accountId, profileId);

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
			DatabaseLog("profile id(%u) found", profileId);

			profile->m_ProfileId = id;
			wcsncpy(profile->m_Name, name, WIC_NAME_MAX_LENGTH);
			profile->m_Rank = rank;
			profile->m_ClanId = clanid;
			profile->m_RankInClan = rankinclan;
			profile->m_OnlineStatus = 0;

			if (clanid > 0)
				AppendClanTag(profile);
		}
	}

	if (!query1.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	if (!UpdateMembershipBadges(accountId, profileId))
	{
		RollbackTransaction();
		return false;
	}

	// *Step 2: update last login date for the requested profile* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "UPDATE %s SET lastlogindate = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

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
		DatabaseLog("QueryUserProfile() query2 failed: accountid(%u), profileid(%u)", accountId, profileId);
	else
		DatabaseLog("profile id(%u) lastlogindate updated", profileId);

	if (!query2.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 3: set new requested profile as active for the account* //

	char SQL3[4096];
	memset(SQL3, 0, sizeof(SQL3));

	// build sql query using table names defined in settings file
	sprintf(SQL3, "UPDATE %s SET activeprofileid = ? WHERE id = ? LIMIT 1", TABLENAME[ACCOUNTS_TABLE]);

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
		DatabaseLog("QueryUserProfile() query3 failed: accountid(%u), profileid(%u)", accountId, profileId);
	else
		DatabaseLog("account id(%u) set active profile id(%u)", accountId, profileId);

	if (!query3.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::RetrieveUserProfiles(const uint accountId, ulong *dstProfileCount, MMG_Profile *profiles)
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

	uint id, clanid;
	uchar rank, rankinclan;

	// bind parameters to prepared statement
	query.Bind(&param[0], &accountId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], name, ARRAYSIZE(name));
	query.Bind(&results[2], &rank);
	query.Bind(&results[3], &clanid);
	query.Bind(&results[4], &rankinclan);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("RetrieveUserProfiles() failed: accountId(%u)", accountId);
		*dstProfileCount = 0;
	}
	else
	{
		ulong profileCount = (ulong)query.StmtNumRows();
		DatabaseLog("accountid(%u): %u profiles found", accountId, profileCount);
		*dstProfileCount = 0;

		if (profileCount > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				profiles[i].m_ProfileId = id;
				wcsncpy(profiles[i].m_Name, name, WIC_NAME_MAX_LENGTH);
				profiles[i].m_Rank = rank;
				profiles[i].m_ClanId = clanid;
				profiles[i].m_RankInClan = rankinclan;
				profiles[i].m_OnlineStatus = 0;

				if (clanid > 0)
					AppendClanTag(&profiles[i]);

				memset(name, 0, sizeof(name));
				i++;
			}

			*dstProfileCount = profileCount;
		}
	}
	
	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AppendClanTag(MMG_Profile *profile)
{
	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	char disptag[WIC_CLANTAG_MAX_LENGTH];
	memset(clantag, 0, sizeof(clantag));
	memset(disptag, 0, sizeof(disptag));

	if (!QueryClanTag(profile->m_ClanId, clantag, disptag))
		return false;

	if (wcslen(clantag) < 0 || strlen(disptag) < 0)
		return false;

	wchar_t buffer[WIC_NAME_MAX_LENGTH];
	memset(buffer, 0, sizeof(buffer));

	wcsncpy(buffer, profile->m_Name, WIC_NAME_MAX_LENGTH);
	memset(profile->m_Name, 0, sizeof(profile->m_Name));

	if (strcmp(disptag, "[C]P") == 0)
		swprintf(profile->m_Name, WIC_NAME_MAX_LENGTH, L"[%ws]%ws", clantag, buffer);
	else if (strcmp(disptag, "P[C]") == 0)
		swprintf(profile->m_Name, WIC_NAME_MAX_LENGTH, L"%ws[%ws]", buffer, clantag);
	else if (strcmp(disptag, "C^P") == 0)
		swprintf(profile->m_Name, WIC_NAME_MAX_LENGTH, L"%ws^%ws", clantag, buffer);
	else if (strcmp(disptag, "-=C=-P") == 0)
		swprintf(profile->m_Name, WIC_NAME_MAX_LENGTH, L"-=%ws=-%ws", clantag, buffer);
	else
		swprintf(profile->m_Name, WIC_NAME_MAX_LENGTH, L"[%ws]%ws", clantag, buffer);

	return true;
}

bool MySQLDatabase::QueryClanTag(const uint clanId, wchar_t *shortclanname, char *displaytag)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT shortclanname, displaytag FROM %s WHERE id = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], results[2];
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	char disptag[WIC_CLANTAG_MAX_LENGTH];
	memset(clantag, 0, sizeof(clantag));
	memset(disptag, 0, sizeof(disptag));

	query.Bind(&param[0], &clanId);
	
	query.Bind(&results[0], clantag, ARRAYSIZE(clantag));
	query.Bind(&results[1], disptag, ARRAYSIZE(disptag));

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryClanTag() failed: clanid(%u)", clanId);

		shortclanname = L"";
		displaytag = "";
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("clan id (%u) clan tag not found.", clanId);

			shortclanname = L"";
			displaytag = "";
		}
		else
		{
			wcsncpy(shortclanname, clantag, WIC_CLANTAG_MAX_LENGTH);
			strncpy(displaytag, disptag, WIC_CLANTAG_MAX_LENGTH);
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileName(const uint profileId, MMG_Profile *profile)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT id, name, onlinestatus, rank, clanid, rankinclan FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], results[6];
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	wchar_t name[WIC_NAME_MAX_LENGTH];
	memset(name, 0, sizeof(name));

	uint id, onlinestatus, clanid;
	uchar rank, rankinclan;

	query.Bind(&param[0], &profileId);

	query.Bind(&results[0], &id);
	query.Bind(&results[1], name, ARRAYSIZE(name));
	query.Bind(&results[2], &onlinestatus);
	query.Bind(&results[3], &rank);
	query.Bind(&results[4], &clanid);
	query.Bind(&results[5], &rankinclan);

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryProfileName() failed: profileid(%u)", profileId);

		profile->m_ProfileId = 0;
		memset(profile->m_Name, 0, sizeof(profile->m_Name));
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
			memset(profile->m_Name, 0, sizeof(profile->m_Name));
			profile->m_Rank = 0;
			profile->m_ClanId = 0;
			profile->m_RankInClan = 0;
			profile->m_OnlineStatus = 0;
		}
		else
		{
			profile->m_ProfileId = id;
			wcsncpy(profile->m_Name, name, WIC_NAME_MAX_LENGTH);
			profile->m_Rank = rank;
			profile->m_ClanId = clanid;
			profile->m_RankInClan = rankinclan;
			profile->m_OnlineStatus = onlinestatus;

			if (clanid > 0)
				AppendClanTag(profile);
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileList(const size_t Count, const uint profileIds[], MMG_Profile profiles[])
{
	for (uint i = 0; i < Count; i++)
	{
		if (!QueryProfileName(profileIds[i], &profiles[i]))
			return false;
	}

	return true;
}

bool MySQLDatabase::UpdateProfileOnlineStatus(const uint profileId, const uint onlinestatus)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET onlinestatus = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &onlinestatus);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateProfileOnlineStatus() failed:");
	
	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileRank(const uint profileId, const uchar rank)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET rank = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &rank);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateProfileRank() failed:");
	
	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileClanId(const uint profileId, const uint clanId)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET clanid = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &clanId);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateProfileClanId() failed:");
	
	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileClanRank(const uint profileId, const uchar rankInClan)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET rankinclan = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &rankInClan);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateProfileClanRank() failed:");

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
			*options = 0;
		else
			*options = commoptions;
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
	sprintf(SQL, "UPDATE %s SET commoptions = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

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

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryFriends(const uint profileId, uint *dstProfileCount, uint friendIds[])
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT friendprofileid FROM %s WHERE profileid = ? ORDER BY friendprofileid ASC LIMIT 100", TABLENAME[FRIENDS_TABLE]);

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
		DatabaseLog("QueryFriends() failed:");
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstProfileCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				friendIds[i] = id;
				i++;
			}

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
	sprintf(SQL, "INSERT INTO %s (profileid, friendprofileid) VALUES (?, ?) ON DUPLICATE KEY UPDATE profileid = profileid, friendprofileid = friendprofileid", TABLENAME[FRIENDS_TABLE]);

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

bool MySQLDatabase::QueryAcquaintances(const uint profileId, uint *dstCount, Acquaintance acquaintances[])
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// AND numberoftimesplayed >= 10
	sprintf(SQL, "SELECT acquaintanceprofileid, numberoftimesplayed FROM %s WHERE dateplayedwith >= ? - 1814400 AND profileid = ? ORDER BY acquaintanceprofileid ASC LIMIT 512", TABLENAME[ACQUAINTANCES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[2], result[2];
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	uint acquaintanceprofileid = 0;
	uint numberoftimesplayed = 0;
	uint local_timestamp = time(NULL);
	uint dateplayedwith = ROUND_TIME(local_timestamp);

	query.Bind(&param[0], &dateplayedwith);
	query.Bind(&param[1], &profileId);

	query.Bind(&result[0], &acquaintanceprofileid);
	query.Bind(&result[1], &numberoftimesplayed);

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryAcquaintances() failed:");
		*dstCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				acquaintances[i].m_ProfileId = acquaintanceprofileid;
				acquaintances[i].m_NumTimesPlayed = numberoftimesplayed;
				i++;
			}

			*dstCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryIgnoredProfiles(const uint profileId, uint *dstProfileCount, uint ignoredIds[])
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT ignoredprofileid FROM %s WHERE profileid = ? ORDER BY ignoredprofileid ASC LIMIT 64", TABLENAME[IGNORED_TABLE]);

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
		DatabaseLog("QueryIgnoredProfiles() failed:");
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstProfileCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				ignoredIds[i] = id;
				i++;
			}

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
	sprintf(SQL, "INSERT INTO %s (profileid, ignoredprofileid) VALUES (?, ?) ON DUPLICATE KEY UPDATE profileid = profileid, ignoredprofileid = ignoredprofileid", TABLENAME[IGNORED_TABLE]);

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

bool MySQLDatabase::QueryPlayerSearch(const wchar_t* const name, const uint maxResults, uint *dstCount, uint profileIds[])
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id FROM %s WHERE isdeleted = 0 AND name LIKE CONCAT('%%',?,'%%') ORDER BY name ASC, id ASC LIMIT ?", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[2], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], name, wcslen(name));
	query.Bind(&param[1], &maxResults);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryPlayerSearch() failed:");
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();

		*dstCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				profileIds[i] = id;
				i++;
			}

			*dstCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryClanSearch(const wchar_t* const name, const uint maxResults, uint *dstCount, MMG_Clan::Description clans[])
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id, clanname, clantag FROM %s WHERE clanname LIKE CONCAT('%%',?,'%%') OR shortclanname LIKE CONCAT('%%',?,'%%') ORDER BY clanname ASC, clantag ASC, id ASC LIMIT ?", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[3], results[3];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	uint id;
	wchar_t clanname[WIC_CLANNAME_MAX_LENGTH];
	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	memset(clanname, 0, sizeof(clanname));
	memset(clantag, 0, sizeof(clantag));

	// bind parameters to prepared statement
	query.Bind(&param[0], name, wcslen(name));
	query.Bind(&param[1], name, wcslen(name));
	query.Bind(&param[2], &maxResults);
	
	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], clanname, ARRAYSIZE(clanname));
	query.Bind(&results[2], clantag, ARRAYSIZE(clantag));

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryClanSearch() failed:");
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();

		*dstCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				clans[i].m_ClanId = id;
				wcsncpy(clans[i].m_FullName, clanname, WIC_CLANNAME_MAX_LENGTH);
				wcsncpy(clans[i].m_ClanTag, clantag, WIC_CLANTAG_MAX_LENGTH);

				i++;
			}

			*dstCount = count;
		}
	}

	if (!query.Success())
		return false;

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

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], motto, ARRAYSIZE(motto));
	query.Bind(&results[1], homepage, ARRAYSIZE(homepage));

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
	sprintf(SQL, "UPDATE %s SET motto = ?, homepage = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[3];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], motto, wcslen(motto));
	query.Bind(&params[1], homepage, wcslen(homepage));
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

bool MySQLDatabase::QueryPendingMessages(const uint profileId, uint *dstMessageCount, MMG_InstantMessageListener::InstantMessage *messages)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));
	
	// TODO temporarily limiting returned pending messages to 20, real limit for the client is unknown

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

	uint id, senderid, recipientid;
	uint writtenat;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], &writtenat);
	query.Bind(&results[2], &senderid);
	query.Bind(&results[3], &recipientid);
	query.Bind(&results[4], message, ARRAYSIZE(message));

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryPendingMessages() failed: profileid(%u)", profileId);
		*dstMessageCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstMessageCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				messages[i].m_MessageId = id;
				messages[i].m_WrittenAt = writtenat;
				this->QueryProfileName(senderid, &messages[i].m_SenderProfile);
				messages[i].m_RecipientProfile = recipientid;
				wcsncpy(messages[i].m_Message, message, WIC_INSTANTMSG_MAX_LENGTH);

				memset(message, 0, sizeof(message));
				i++;
			}

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

	time_t local_timestamp = time(NULL);
	//struct tm* gtime = gmtime(&local_timestamp);
	//time_t utc_timestamp = mktime(gtime);

	if (message->m_WrittenAt == 0)
		message->m_WrittenAt = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &message->m_WrittenAt);
	query.Bind(&params[1], &message->m_SenderProfile.m_ProfileId);
	query.Bind(&params[2], &message->m_RecipientProfile);
	query.Bind(&params[3], message->m_Message, wcslen(message->m_Message));

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

bool MySQLDatabase::AddAbuseReport(const uint profileId, const MMG_Profile senderProfile, const uint flaggedProfileId, const wchar_t *report)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	// get all the ids.
	uint senderAccountId, flaggedAccountId;
	MMG_Profile flaggedProfile;
	QueryProfileName(flaggedProfileId, &flaggedProfile);
	QueryProfileAccountId(profileId, &senderAccountId);
	QueryProfileAccountId(flaggedProfileId, &flaggedAccountId);

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "INSERT INTO %s (senderaccountid, senderprofileid, sendername, reportedaccountid, reportedprofileid, reportedname, report, datereported) VALUES (?, ?, ?, ?, ?, ?, ?, ?)", TABLENAME[ABUSEREPORTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[8];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	time_t local_timestamp = time(NULL);
	//struct tm* gtime = gmtime(&local_timestamp);
	//time_t utc_timestamp = mktime(gtime);

	uint datereported = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &senderAccountId);
	query.Bind(&params[1], &senderProfile.m_ProfileId);
	query.Bind(&params[2], senderProfile.m_Name, wcslen(senderProfile.m_Name));
	query.Bind(&params[3], &flaggedAccountId);
	query.Bind(&params[4], &flaggedProfile.m_ProfileId);
	query.Bind(&params[5], flaggedProfile.m_Name, wcslen(flaggedProfile.m_Name));
	query.Bind(&params[6], report, wcslen(report));
	query.Bind(&params[7], &datereported);

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("AddAbuseReport() failed: accountid(%u)", senderAccountId);
	else
		DatabaseLog("account id(%u) reported account id(%u):", senderAccountId, flaggedAccountId);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryProfileAccountId(const uint profileId, uint *dstAccountId)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT accountid FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint accountid;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&result[0], &accountid);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryProfileAccountId() query failed: profileid(%u)", profileId);
		*dstAccountId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("accountid not found");
			*dstAccountId = 0;
		}
		else
		{
			DatabaseLog("accountid found");
			*dstAccountId = accountid;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileGuestbook(const uint profileId, uint *dstEntryCount, MMG_ProfileGuestBookProtocol::GetRsp *guestbook)
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
	sprintf(SQL, "(SELECT id, profileid, timestamp, requestid, posterid, message FROM %s WHERE profileid = ? ORDER BY id DESC LIMIT 32) ORDER BY timestamp ASC, requestid ASC", TABLENAME[PROFILE_GB_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], results[6];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));
	
	// query specific variables
	wchar_t message[WIC_GUESTBOOK_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	uint id, profileid, posterid;
	uint timestamp, requestid;

	// bind parameters to prepared statement
	query.Bind(&param[0], &profileId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], &profileid);
	query.Bind(&results[2], &timestamp);
	query.Bind(&results[3], &requestid);
	query.Bind(&results[4], &posterid);
	query.Bind(&results[5], message, ARRAYSIZE(message));

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryProfileGuestbook() query failed: profileid(%u)", profileId);
		*dstEntryCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstEntryCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				guestbook->m_Entries[i].m_MessageId = id;
				guestbook->m_Entries[i].m_Timestamp = timestamp;
				guestbook->m_Entries[i].m_ProfileId = posterid;
				wcsncpy(guestbook->m_Entries[i].m_Message, message, WIC_GUESTBOOK_MAX_LENGTH);

				memset(message, 0, sizeof(message));
				i++;
			}

			*dstEntryCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AddProfileGuestbookEntry(const uint profileId, const uint requestId, MMG_ProfileGuestBookProtocol::GetRsp::GuestbookEntry *entry)
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
	sprintf(SQL, "INSERT INTO %s (profileid, timestamp, requestid, posterid, message) VALUES (?, ?, ?, ?, ?)", TABLENAME[PROFILE_GB_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND params[5];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	time_t local_timestamp = time(NULL);
	
	entry->m_Timestamp = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &entry->m_Timestamp);
	query.Bind(&params[2], &requestId);
	query.Bind(&params[3], &entry->m_ProfileId);
	query.Bind(&params[4], entry->m_Message, wcslen(entry->m_Message));
	
	// execute prepared statement
	if (!query.StmtExecute(params))
	{
		DatabaseLog("AddProfileGuestbookEntry() query failed: profileid(%u)", entry->m_ProfileId);
	}
	else
	{
		//DatabaseLog("");
		entry->m_MessageId = (uint)query.StmtInsertId();
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

bool MySQLDatabase::DeleteProfileGuestbookEntry(const uint profileId, const uint messageId, const uchar deleteAllByProfile)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	if (deleteAllByProfile)
	{
		//get poster id by message id

		char SQL1[4096];
		memset(SQL1, 0, sizeof(SQL1));

		// build sql query using table names defined in settings file
		sprintf(SQL1, "SELECT posterid FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILE_GB_TABLE]);

		// prepared statement wrapper object
		MySQLQuery query1(this->m_Connection, SQL1);
	
		// prepared statement binding structures
		MYSQL_BIND param1[1], result1[1];

		// initialize (zero) bind structures
		memset(param1, 0, sizeof(param1));
		memset(result1, 0, sizeof(result1));

		// query specific variables
		uint posterid;

		// bind parameters to prepared statement
		query1.Bind(&param1[0], &messageId);

		// bind results
		query1.Bind(&result1[0], &posterid);

		// execute prepared statement
		if(!query1.StmtExecute(param1, result1))
		{
			DatabaseLog("DeleteProfileGuestbookEntry() query1 failed: profileid(%u)", profileId);
		}
		else
		{
			query1.StmtFetch();
		}

		if (!query1.Success())
			return false;

		// delete all poster ids posts

		char SQL2[4096];
		memset(SQL2, 0, sizeof(SQL2));

		// build sql query using table names defined in settings file
		sprintf(SQL2, "DELETE FROM %s WHERE profileid = ? AND posterid = ?", TABLENAME[PROFILE_GB_TABLE]);

		// prepared statement wrapper object
		MySQLQuery query2(this->m_Connection, SQL2);

		// prepared statement binding structures
		MYSQL_BIND param2[2];

		// initialize (zero) bind structures
		memset(param2, 0, sizeof(param2));

		// bind parameters to prepared statement
		query2.Bind(&param2[0], &profileId);
		query2.Bind(&param2[1], &posterid);

		// execute prepared statement
		if (!query2.StmtExecute(param2))
			DatabaseLog("DeleteProfileGuestbookEntry() query2 failed: profileid(%u)", profileId);
		//else
		//	DatabaseLog("");

		if (!query2.Success())
		{
			this->RollbackTransaction();
			return false;
		}
	}
	else
	{
		char SQL[4096];
		memset(SQL, 0, sizeof(SQL));

		// build sql query using table names defined in settings file
		sprintf(SQL, "DELETE FROM %s WHERE id = ? AND profileid = ?", TABLENAME[PROFILE_GB_TABLE]);

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
			DatabaseLog("DeleteProfileGuestbookEntry() query failed: profileid(%u)", profileId);
		//else
		//	DatabaseLog("");

		if (!query.Success())
		{
			this->RollbackTransaction();
			return false;
		}
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::CheckIfClanNameExists(const wchar_t* clanname, uint *dstId)
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
	sprintf(SQL, "SELECT id FROM %s WHERE clanname = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

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
	query.Bind(&param[0], clanname, wcslen(clanname));
	
	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfClanNameExists() query failed.");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("clan name not found");
			*dstId = 0;
		}
		else
		{
			DatabaseLog("clan name found");
			*dstId = id;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::CheckIfClanTagExists(const wchar_t* clantag, uint *dstId)
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
	sprintf(SQL, "SELECT id FROM %s WHERE shortclanname = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

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
	query.Bind(&param[0], clantag, wcslen(clantag));
	
	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("CheckIfClanTagExists() query failed.");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("clantag not found");
			*dstId = 0;
		}
		else
		{
			DatabaseLog("clantag found");
			*dstId = id;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::CreateClan(const uint profileId, const wchar_t* clanname, const wchar_t* clantag, const char* displayTag, uint *dstId)
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

	sprintf(SQL, "INSERT INTO %s (clanname, clantag, shortclanname, displaytag) VALUES (?, ?, ?, ?)", TABLENAME[CLANS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[4];
	memset(params, 0, sizeof(params));

	wchar_t temptags[WIC_CLANTAG_MAX_LENGTH];
	memset(temptags, 0, sizeof(temptags));

	if (strcmp(displayTag, "[C]P") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"[%ws]", clantag);
	else if (strcmp(displayTag, "P[C]") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"[%ws]", clantag);
	else if (strcmp(displayTag, "C^P") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"%ws^", clantag);
	else if (strcmp(displayTag, "-=C=-P") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"-=%ws=-", clantag);
	else
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"[%ws]", clantag);

	query.Bind(&params[0], clanname, wcslen(clanname));
	query.Bind(&params[1], temptags, wcslen(temptags));
	query.Bind(&params[2], clantag, wcslen(clantag));
	query.Bind(&params[3], displayTag, strlen(displayTag));

	if (!query.StmtExecute(params))
	{
		DatabaseLog("CreateClan() failed:");
		*dstId = 0;
	}
	else
	{
		DatabaseLog("Created new clan.");
		*dstId = (uint)query.StmtInsertId();
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

bool MySQLDatabase::DeleteProfileClanInvites(const uint profileId, const uint clanId)
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
	sprintf(SQL, "DELETE FROM %s WHERE (senderprofileid = ? OR recipientprofileid = ?) AND message LIKE CONCAT(?, '%%')", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[3];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|%u", clanId);

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &profileId);
	query.Bind(&params[2], message, wcslen(message));

	// execute prepared statement
	if (!query.StmtExecute(params))
		DatabaseLog("DeleteProfileClanInvites() failed:");
	else
		DatabaseLog("DeleteProfileClanInvites() success:");

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::DeleteProfileClanMessages(const uint profileId)
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
	sprintf(SQL, "DELETE FROM %s WHERE (senderprofileid = ? OR recipientprofileid = ?) AND message LIKE CONCAT(?, '%%')", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[3];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clms|");

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &profileId);
	query.Bind(&params[2], message, wcslen(message));

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("DeleteProfileClanMessages() failed:");
	else
		DatabaseLog("DeleteProfileClanMessages() success:");

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::UpdateClanPlayerOfWeek(const uint clanId, const uint profileId)
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
	sprintf(SQL, "UPDATE %s SET playeroftheweekid = ? WHERE id = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &clanId);

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("UpdateClanPlayerOfWeek() failed: clanid(%u)", profileId);
	else
		DatabaseLog("UpdateClanPlayerOfWeek() success: clanid(%u)", profileId);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::DeleteClan(const uint clanId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	// *Step 1: delete clan* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "DELETE FROM %s WHERE id = ?", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);

	// prepared statement binding structures
	MYSQL_BIND param1[1];

	// initialize (zero) bind structures
	memset(param1, 0, sizeof(param1));

	// bind parameters to prepared statement
	query1.Bind(&param1[0], &clanId);

	// execute prepared statement
	if (!query1.StmtExecute(param1))
		DatabaseLog("DeleteClan() query1 failed: clan id(%u)", clanId);
	else
		DatabaseLog("deleted clanid(%u)", clanId);

	if (!query1.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// *Step 2: delete clan invites* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "DELETE FROM %s WHERE message LIKE CONCAT(?, '%%')", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND params2[1];

	// initialize (zero) bind structures
	memset(params2, 0, sizeof(params2));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|%u", clanId);

	// bind parameters to prepared statement
	query2.Bind(&params2[0], message, wcslen(message));

	// execute prepared statement
	if (!query2.StmtExecute(params2))
		DatabaseLog("DeleteClan() query2 failed: clan id(%u)", clanId);
	else
		DatabaseLog("deleted clanid(%u) pending invites", clanId);

	if (!query2.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryClanFullInfo(const uint clanId, uint *dstMemberCount, MMG_Clan::FullInfo *fullinfo)
{
	// *Step 1: get the requested clan* //

	char SQL1[4096];
	memset(SQL1, 0, sizeof(SQL1));

	// build sql query using table names defined in settings file
	sprintf(SQL1, "SELECT id, clanname, clantag, playeroftheweekid, motto, motd, homepage FROM %s WHERE id = ? LIMIT 1", TABLENAME[CLANS_TABLE]);
	
	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);

	// prepared statement binding structures
	MYSQL_BIND param1[1], results1[7];

	// initialize (zero) bind structures
	memset(param1, 0, sizeof(param1));
	memset(results1, 0, sizeof(results1));

	// query specific variables
	wchar_t clanname[WIC_CLANNAME_MAX_LENGTH];
	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	memset(clanname, 0, sizeof(clanname));
	memset(clantag, 0, sizeof(clantag));
	
	uint id, playeroftheweekid;

	wchar_t clanmotto[WIC_MOTTO_MAX_LENGTH];
	wchar_t clanmotd[WIC_MOTD_MAX_LENGTH];
	wchar_t clanhomepage[WIC_HOMEPAGE_MAX_LENGTH];
	memset(clanmotto, 0, sizeof(clanmotto));
	memset(clanmotd, 0, sizeof(clanmotd));
	memset(clanhomepage, 0, sizeof(clanhomepage));

	// bind parameters to prepared statement
	query1.Bind(&param1[0], &clanId);

	// bind results
	query1.Bind(&results1[0], &id);
	query1.Bind(&results1[1], clanname, ARRAYSIZE(clanname));
	query1.Bind(&results1[2], clantag, ARRAYSIZE(clantag));
	query1.Bind(&results1[3], &playeroftheweekid);
	query1.Bind(&results1[4], clanmotto, ARRAYSIZE(clanmotto));
	query1.Bind(&results1[5], clanmotd, ARRAYSIZE(clanmotd));
	query1.Bind(&results1[6], clanhomepage, ARRAYSIZE(clanhomepage));

	// execute prepared statement
	if(!query1.StmtExecute(param1, results1))
	{
		DatabaseLog("QueryClanFullInfo(q1) failed: clanid(%u)", clanId);

		fullinfo->m_ClanId = 0;
		wcsncpy(fullinfo->m_FullClanName, L"", WIC_CLANNAME_MAX_LENGTH);
		wcsncpy(fullinfo->m_ShortClanName, L"", WIC_CLANTAG_MAX_LENGTH);
		wcsncpy(fullinfo->m_Motto, L"", WIC_MOTTO_MAX_LENGTH);
		wcsncpy(fullinfo->m_LeaderSays, L"", WIC_MOTD_MAX_LENGTH);
		wcsncpy(fullinfo->m_Homepage, L"", WIC_HOMEPAGE_MAX_LENGTH);
		// clanmembers below
		fullinfo->m_PlayerOfWeek = 0;
	}
	else
	{
		if (!query1.StmtFetch())
		{
			DatabaseLog("clanid(%u) not found", clanId);

			fullinfo->m_ClanId = 0;
			wcsncpy(fullinfo->m_FullClanName, L"", WIC_CLANNAME_MAX_LENGTH);
			wcsncpy(fullinfo->m_ShortClanName, L"", WIC_CLANTAG_MAX_LENGTH);
			wcsncpy(fullinfo->m_Motto, L"", WIC_MOTTO_MAX_LENGTH);
			wcsncpy(fullinfo->m_LeaderSays, L"", WIC_MOTD_MAX_LENGTH);
			wcsncpy(fullinfo->m_Homepage, L"", WIC_HOMEPAGE_MAX_LENGTH);
			// clanmembers below
			fullinfo->m_PlayerOfWeek = 0;
		}
		else
		{
			DatabaseLog("clanid(%u) found", clanId);

			fullinfo->m_ClanId = id;
			wcsncpy(fullinfo->m_FullClanName, clanname, WIC_CLANNAME_MAX_LENGTH);
			wcsncpy(fullinfo->m_ShortClanName, clantag, WIC_CLANTAG_MAX_LENGTH);
			wcsncpy(fullinfo->m_Motto, clanmotto, WIC_MOTTO_MAX_LENGTH);
			wcsncpy(fullinfo->m_LeaderSays, clanmotd, WIC_MOTD_MAX_LENGTH);
			wcsncpy(fullinfo->m_Homepage, clanhomepage, WIC_HOMEPAGE_MAX_LENGTH);
			// clanmembers below
			fullinfo->m_PlayerOfWeek = playeroftheweekid;
		}
	}

	if (!query1.Success())
		return false;

	// *Step 2: get the clan members* //

	char SQL2[4096];
	memset(SQL2, 0, sizeof(SQL2));

	// build sql query using table names defined in settings file
	sprintf(SQL2, "SELECT id FROM %s WHERE clanid = ? ORDER BY id ASC LIMIT 512", TABLENAME[PROFILES_TABLE]);
	
	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND param2[1], results2[1];

	// initialize (zero) bind structures
	memset(param2, 0, sizeof(param2));
	memset(results2, 0, sizeof(results2));

	// query specific variables
	uint playerid;
	
	// bind parameters to prepared statement
	query2.Bind(&param2[0], &clanId);

	// bind results
	query2.Bind(&results2[0], &playerid);
	
	// execute prepared statement
	if(!query2.StmtExecute(param2, results2))
	{
		DatabaseLog("QueryClanProfile(q2) failed: clanid(%u)", clanId);
		
		memset(fullinfo->m_ClanMembers, 0, sizeof(fullinfo->m_ClanMembers));
		*dstMemberCount = 0;
	}
	else
	{
		ulong profileCount = (ulong)query2.StmtNumRows();
		DatabaseLog("clanid(%u): %u profiles found", clanId, profileCount);

		*dstMemberCount = 0;

		if (profileCount > 0)
		{
			int i = 0;

			while(query2.StmtFetch())
			{
				fullinfo->m_ClanMembers[i] = playerid;
				i++;
			}

			*dstMemberCount = profileCount;
		}
	}

	if (!query2.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryClanDescription(const uint clanId, MMG_Clan::Description *description)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id, clanname, clantag FROM %s WHERE id = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[1], results[3];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	uint id;
	wchar_t clanname[WIC_CLANNAME_MAX_LENGTH];
	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	memset(clanname, 0, sizeof(clanname));
	memset(clantag, 0, sizeof(clantag));

	// bind parameters to prepared statement
	query.Bind(&param[0], &clanId);
	
	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], clanname, ARRAYSIZE(clanname));
	query.Bind(&results[2], clantag, ARRAYSIZE(clantag));

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryClanDescription() failed: clanid(%u)", clanId);

		description->m_ClanId = 0;
		wcsncpy(description->m_FullName, L"", WIC_CLANNAME_MAX_LENGTH);
		wcsncpy(description->m_ClanTag, L"", WIC_CLANTAG_MAX_LENGTH);
	}
	else
	{
		if (!query.StmtFetch())
		{
			//DatabaseLog("");

			description->m_ClanId = 0;
			wcsncpy(description->m_FullName, L"", WIC_CLANNAME_MAX_LENGTH);
			wcsncpy(description->m_ClanTag, L"", WIC_CLANTAG_MAX_LENGTH);
		}
		else
		{
			//DatabaseLog("");
			
			description->m_ClanId = id;
			wcsncpy(description->m_FullName, clanname, WIC_CLANNAME_MAX_LENGTH);
			wcsncpy(description->m_ClanTag, clantag, WIC_CLANTAG_MAX_LENGTH);
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::SaveClanEditableVariables(const uint clanId, const uint profileId, const wchar_t *motto, const wchar_t *motd, const wchar_t *homepage)
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
	sprintf(SQL, "UPDATE %s SET playeroftheweekid = ?, motto = ?, motd = ?, homepage = ? WHERE id = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[5];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], motto, wcslen(motto));
	query.Bind(&params[2], motd, wcslen(motd));
	query.Bind(&params[3], homepage, wcslen(homepage));
	query.Bind(&params[4], &clanId);

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("SaveClanEditableVariables() failed: clanid(%u)", clanId);
	else
		DatabaseLog("SaveClanEditableVariables() success: clanid(%u)", clanId);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::CheckIfInvitedAlready(const uint clanId, const uint inviteeProfileId, uint *dstId)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT id FROM %s WHERE recipientprofileid = ? AND message LIKE CONCAT(?, '%%') LIMIT 1", TABLENAME[MESSAGES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2], result[1];
	memset(params, 0, sizeof(params));
	memset(result, 0, sizeof(result));

	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|");

	uint id;

	query.Bind(&params[0], &inviteeProfileId);
	query.Bind(&params[1], message, wcslen(message));
	
	query.Bind(&result[0], &id);

	if(!query.StmtExecute(params, result))
	{
		DatabaseLog("CheckIfInvitedAlready() failed:");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
			*dstId = 0;
		else
			*dstId = id;
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryClanGuestbook(const uint clanId, uint *dstEntryCount, MMG_ClanGuestbookProtocol::GetRsp *guestbook)
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
	sprintf(SQL, "(SELECT id, clanid, timestamp, requestid, posterid, message FROM %s WHERE clanid = ? ORDER BY id DESC LIMIT 32) ORDER BY timestamp ASC, requestid ASC", TABLENAME[CLAN_GB_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], results[6];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));
	
	// query specific variables
	wchar_t message[WIC_GUESTBOOK_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	uint id, clanid, posterid;
	uint timestamp, requestid;

	// bind parameters to prepared statement
	query.Bind(&param[0], &clanId);

	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], &clanid);
	query.Bind(&results[2], &timestamp);
	query.Bind(&results[3], &requestid);
	query.Bind(&results[4], &posterid);
	query.Bind(&results[5], message, ARRAYSIZE(message));

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryClanGuestbook() query failed: clanid(%u)", clanId);
		*dstEntryCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstEntryCount = 0;

		if (count > 0)
		{
			int i = 0;

			while(query.StmtFetch())
			{
				guestbook->m_Entries[i].m_MessageId = id;
				guestbook->m_Entries[i].m_Timestamp = timestamp;
				guestbook->m_Entries[i].m_ProfileId = posterid;
				wcsncpy(guestbook->m_Entries[i].m_Message, message, WIC_GUESTBOOK_MAX_LENGTH);

				memset(message, 0, sizeof(message));
				i++;
			}

			*dstEntryCount = count;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::AddClanGuestbookEntry(const uint clanId, const uint requestId, MMG_ClanGuestbookProtocol::GetRsp::GuestbookEntry *entry)
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
	sprintf(SQL, "INSERT INTO %s (clanid, timestamp, requestid, posterid, message) VALUES (?, ?, ?, ?, ?)", TABLENAME[CLAN_GB_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND params1[5];

	// initialize (zero) bind structures
	memset(params1, 0, sizeof(params1));

	// query specific variables
	time_t local_timestamp = time(NULL);
	
	entry->m_Timestamp = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params1[0], &clanId);
	query.Bind(&params1[1], &entry->m_Timestamp);
	query.Bind(&params1[2], &requestId);
	query.Bind(&params1[3], &entry->m_ProfileId);
	query.Bind(&params1[4], entry->m_Message, wcslen(entry->m_Message));
	
	// execute prepared statement
	if (!query.StmtExecute(params1))
	{
		DatabaseLog("AddClanGuestbookEntry() query failed: profileid(%u)", entry->m_ProfileId);
	}
	else
	{
		//DatabaseLog("");
		entry->m_MessageId = (uint)query.StmtInsertId();
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

bool MySQLDatabase::DeleteClanGuestbookEntry(const uint clanId, const uint messageId, const uchar deleteAllByProfile)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	// begin an sql transaction
	this->BeginTransaction();

	if (deleteAllByProfile)
	{
		//get poster id by message id

		char SQL1[4096];
		memset(SQL1, 0, sizeof(SQL1));

		// build sql query using table names defined in settings file
		sprintf(SQL1, "SELECT posterid FROM %s WHERE id = ? LIMIT 1", TABLENAME[CLAN_GB_TABLE]);

		// prepared statement wrapper object
		MySQLQuery query1(this->m_Connection, SQL1);
	
		// prepared statement binding structures
		MYSQL_BIND param1[1], result1[1];

		// initialize (zero) bind structures
		memset(param1, 0, sizeof(param1));
		memset(result1, 0, sizeof(result1));

		// query specific variables
		uint posterid;

		// bind parameters to prepared statement
		query1.Bind(&param1[0], &messageId);

		// bind results
		query1.Bind(&result1[0], &posterid);

		// execute prepared statement
		if(!query1.StmtExecute(param1, result1))
		{
			DatabaseLog("DeleteClanGuestbookEntry() query1 failed: clanid(%u)", clanId);
		}
		else
		{
			query1.StmtFetch();
		}

		if (!query1.Success())
			return false;

		// delete all poster ids posts

		char SQL2[4096];
		memset(SQL2, 0, sizeof(SQL2));

		// build sql query using table names defined in settings file
		sprintf(SQL2, "DELETE FROM %s WHERE clanid = ? AND posterid = ?", TABLENAME[CLAN_GB_TABLE]);

		// prepared statement wrapper object
		MySQLQuery query2(this->m_Connection, SQL2);

		// prepared statement binding structures
		MYSQL_BIND param2[2];

		// initialize (zero) bind structures
		memset(param2, 0, sizeof(param2));

		// bind parameters to prepared statement
		query2.Bind(&param2[0], &clanId);
		query2.Bind(&param2[1], &posterid);

		// execute prepared statement
		if (!query2.StmtExecute(param2))
			DatabaseLog("DeleteClanGuestbookEntry() query2 failed: clanid(%u)", clanId);
		//else
		//	DatabaseLog("");

		if (!query2.Success())
		{
			this->RollbackTransaction();
			return false;
		}
	}
	else
	{
		char SQL[4096];
		memset(SQL, 0, sizeof(SQL));

		// build sql query using table names defined in settings file
		sprintf(SQL, "DELETE FROM %s WHERE id = ? AND clanid = ?", TABLENAME[CLAN_GB_TABLE]);

		// prepared statement wrapper object
		MySQLQuery query(this->m_Connection, SQL);

		// prepared statement binding structures
		MYSQL_BIND param[2];

		// initialize (zero) bind structures
		memset(param, 0, sizeof(param));

		// bind parameters to prepared statement
		query.Bind(&param[0], &messageId);
		query.Bind(&param[1], &clanId);

		// execute prepared statement
		if (!query.StmtExecute(param))
			DatabaseLog("DeleteClanGuestbookEntry() query failed: clanid(%u)", clanId);
		//else
		//	DatabaseLog("");

		if (!query.Success())
		{
			this->RollbackTransaction();
			return false;
		}
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::QueryProfileLadderPosition(const uint profileId, uint *position)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT id FROM %s WHERE profileid = ? LIMIT 1", TABLENAME[PLAYER_LADDER_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], result[1];
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	uint id = 0;

	query.Bind(&param[0], &profileId);

	query.Bind(&result[0], &id);

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryProfileLadderPosition() failed:");
		*position = 0;
	}
	else
	{
		if (!query.StmtFetch())
			*position = 0;
		else
			*position = id;
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryPlayerLadderCount(uint *dstTotalCount)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT COUNT(id) FROM %s USE INDEX(PRIMARY)", TABLENAME[PLAYER_LADDER_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND result[1];
	memset(result, 0, sizeof(result));

	uint count = 0;

	query.Bind(&result[0], &count);
	mysql_stmt_bind_result(query.GetStatement(), result);
	mysql_stmt_store_result(query.GetStatement());

	if(!query.StmtExecute())
	{
		DatabaseLog("QueryPlayerLadderCount() failed:");
		*dstTotalCount = 0;
	}
	else
	{
		if (!query.StmtFetch())
			*dstTotalCount = 0;
		else
			*dstTotalCount = count;
	}

	mysql_stmt_free_result(query.GetStatement());

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryPlayerLadder(const uint startPos, const uint maxResults, uint *dstFoundItems, MMG_LadderProtocol::LadderRsp *ladder)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT profileid, ladderscore FROM %s WHERE id >= ? ORDER BY id ASC LIMIT ?", TABLENAME[PLAYER_LADDER_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[2], result[2];

	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	query.Bind(&param[0], &startPos);
	query.Bind(&param[1], &maxResults);

	uint profileid;
	uint ladderscore;

	query.Bind(&result[0], &profileid);
	query.Bind(&result[1], &ladderscore);

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryPlayerLadder() failed:");
		*dstFoundItems = 0;
		ladder->startPos = 0;
		ladder->ladderSize = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		*dstFoundItems = 0;
		ladder->startPos = 0;
		ladder->ladderSize = 0;

		if (startPos > 0 && count > 0)
		{
			int i = 0;
			while (query.StmtFetch())
			{
				QueryProfileName(profileid, &ladder->ladderItems[i].profile);
				ladder->ladderItems[i].score = ladderscore;
				i++;
			}

			*dstFoundItems = count;
			ladder->startPos = startPos;
		}

		QueryPlayerLadderCount(&ladder->ladderSize);
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryPlayerLadder(const uint profileId, uint *dstFoundItems, MMG_LadderProtocol::LadderRsp *ladder)
{
	uint thePosition = 0;

	if (!QueryProfileLadderPosition(profileId, &thePosition))
		return false;

	if (!QueryPlayerLadder(thePosition, 1, dstFoundItems, ladder))
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileMedals(const uint profileId, const size_t Count, MMG_Stats::Medal medals[])
{
	uint buffer[256];
	memset(buffer, 0, sizeof(buffer));

	if (!QueryProfileMedalsRawData(profileId, buffer, sizeof(buffer)))
		return false;

	MMG_BitReader<unsigned int> reader(buffer, sizeof(buffer) * 8);

	for (uint i = 0; i < Count; i++)
	{
		medals[i].level = reader.ReadBits(2);
		medals[i].stars = reader.ReadBits(2);
	}

	return true;
}

bool MySQLDatabase::QueryProfileBadges(const uint profileId, const size_t Count, MMG_Stats::Badge badges[])
{
	uint buffer[256];
	memset(buffer, 0, sizeof(buffer));

	if (!QueryProfileBadgesRawData(profileId, buffer, sizeof(buffer)))
		return false;

	MMG_BitReader<unsigned int> reader(buffer, sizeof(buffer) * 8);

	for (uint i = 0; i < Count; i++)
	{
		badges[i].level = reader.ReadBits(2);
		badges[i].stars = reader.ReadBits(2);
	}

	return true;
}

bool MySQLDatabase::QueryProfileMedalsRawData(const uint profileId, voidptr_t Data, ulong Length)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT medaldata FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], result[1];

	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	query.Bind(&param[0], &profileId);

	query.BindBlob(&result[0], Data, Length);

	if (!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryMedalsRawData() failed:");
	}
	else
	{
		if (!query.StmtFetch())
			DatabaseLog("medal data not found");
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileBadgesRawData(const uint profileId, voidptr_t Data, ulong Length)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT badgedata FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], result[1];

	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	query.Bind(&param[0], &profileId);

	query.BindBlob(&result[0], Data, Length);

	if (!query.StmtExecute(param, result))
	{
		DatabaseLog("QueryBadgesRawData() failed:");
	}
	else
	{
		if (!query.StmtFetch())
			DatabaseLog("badge data not found");
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileStats(const uint profileId, MMG_Stats::PlayerStatsRsp *playerstats)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT lastmatchplayed, scoretotal, scoreasinfantry, highscoreasinfantry, scoreassupport, highscoreassupport, scoreasarmor, highscoreasarmor, scoreasair, highscoreasair, scorebydamagingenemies, scorebyusingtacticalaid, scorebycapturingcommandpoints, scorebyrepairing, scorebyfortifying, scorelostbykillingfriendly, highestscore, timetotalmatchlength, timeplayedasusa, timeplayedasussr, timeplayedasnato, timeplayedasinfantry, timeplayedassupport, timeplayedasarmor, timeplayedasair, numberofmatches, numberofmatcheswon, numberofmatcheslost, numberofassaultmatches, numberofassaultmatcheswon, numberofdominationmatches, numberofdominationmatcheswon, numberoftugofwarmatches, numberoftugofwarmatcheswon, currentwinningstreak, bestwinningstreak, numberofmatcheswonbytotaldomination, numberofperfectdefendsinassaultmatch, numberofperfectpushesintugofwarmatch, numberofunitskilled, numberofunitslost, numberofreinforcementpointsspent, numberoftacticalaidpointsspent, numberofnukesdeployed, numberoftacticalaidcriticalhits FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], results[45];

	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	uint scoreTotal=0, scoreLost=0;

	query.Bind(&param[0], &profileId);

	query.Bind(&results[0], &playerstats->m_LastMatchPlayed);
	query.Bind(&results[1], &playerstats->m_ScoreTotal);
	query.Bind(&results[2], &playerstats->m_ScoreAsInfantry);
	query.Bind(&results[3], &playerstats->m_HighScoreAsInfantry);
	query.Bind(&results[4], &playerstats->m_ScoreAsSupport);
	query.Bind(&results[5], &playerstats->m_HighScoreAsSupport);
	query.Bind(&results[6], &playerstats->m_ScoreAsArmor);
	query.Bind(&results[7], &playerstats->m_HighScoreAsArmor);
	query.Bind(&results[8], &playerstats->m_ScoreAsAir);
	query.Bind(&results[9], &playerstats->m_HighScoreAsAir);
	query.Bind(&results[10], &playerstats->m_ScoreByDamagingEnemies);
	query.Bind(&results[11], &playerstats->m_ScoreByUsingTacticalAid);
	query.Bind(&results[12], &playerstats->m_ScoreByCapturingCommandPoints);
	query.Bind(&results[13], &playerstats->m_ScoreByRepairing);
	query.Bind(&results[14], &playerstats->m_ScoreByFortifying);
	query.Bind(&results[15], &scoreLost);
	query.Bind(&results[16], &playerstats->m_HighestScore);
	query.Bind(&results[17], &playerstats->m_TimeTotalMatchLength);
	query.Bind(&results[18], &playerstats->m_TimePlayedAsUSA);
	query.Bind(&results[19], &playerstats->m_TimePlayedAsUSSR);
	query.Bind(&results[20], &playerstats->m_TimePlayedAsNATO);
	query.Bind(&results[21], &playerstats->m_TimePlayedAsInfantry);
	query.Bind(&results[22], &playerstats->m_TimePlayedAsSupport);
	query.Bind(&results[23], &playerstats->m_TimePlayedAsArmor);
	query.Bind(&results[24], &playerstats->m_TimePlayedAsAir);
	query.Bind(&results[25], &playerstats->m_NumberOfMatches);
	query.Bind(&results[26], &playerstats->m_NumberOfMatchesWon);
	query.Bind(&results[27], &playerstats->m_NumberOfMatchesLost);
	query.Bind(&results[28], &playerstats->m_NumberOfAssaultMatches);
	query.Bind(&results[29], &playerstats->m_NumberOfAssaultMatchesWon);
	query.Bind(&results[30], &playerstats->m_NumberOfDominationMatches);
	query.Bind(&results[31], &playerstats->m_NumberOfDominationMatchesWon);
	query.Bind(&results[32], &playerstats->m_NumberOfTugOfWarMatches);
	query.Bind(&results[33], &playerstats->m_NumberOfTugOfWarMatchesWon);
	query.Bind(&results[34], &playerstats->m_CurrentWinningStreak);
	query.Bind(&results[35], &playerstats->m_BestWinningStreak);
	query.Bind(&results[36], &playerstats->m_NumberOfMatchesWonByTotalDomination);
	query.Bind(&results[37], &playerstats->m_NumberOfPerfectDefendsInAssaultMatch);
	query.Bind(&results[38], &playerstats->m_NumberOfPerfectPushesInTugOfWarMatch);
	query.Bind(&results[39], &playerstats->m_NumberOfUnitsKilled);
	query.Bind(&results[40], &playerstats->m_NumberOfUnitsLost);
	query.Bind(&results[41], &playerstats->m_NumberOfReinforcementPointsSpent);
	query.Bind(&results[42], &playerstats->m_NumberOfTacticalAidPointsSpent);
	query.Bind(&results[43], &playerstats->m_NumberOfNukesDeployed);
	query.Bind(&results[44], &playerstats->m_NumberOfTacticalAidCriticalHits);

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryProfileStats() query failed:");
	}
	else
	{
		if (!query.StmtFetch())
		{
			DatabaseLog("stats not found");
		}
		else
		{
			//DatabaseLog("stats found");

			playerstats->m_ProfileId = profileId;
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::QueryProfileExtraStats(const uint profileId, MMG_Stats::ExtraPlayerStats *extrastats)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT currentassaultwinstreak, currentdominationwinstreak, currenttugofwarwinstreak, currentdominationperfectstreak, currentassaultperfectstreak, currenttugofwarperfectstreak, currentnukesdeployedstreak, numberoftimesbestplayer, currentbestplayerstreak, numberoftimesbestinfantry, currentbestinfantrystreak, numberoftimesbestsupport, currentbestsupportstreak, numberoftimesbestair, currentbestairstreak, numberoftimesbestarmor, currentbestarmorstreak, numberofcommandpointcaptures FROM %s WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], results[18];
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	query.Bind(&param[0], &profileId);

	query.Bind(&results[0], &extrastats->m_CurrentAssaultWinStreak);
	query.Bind(&results[1], &extrastats->m_CurrentDominationWinStreak);
	query.Bind(&results[2], &extrastats->m_CurrentTugOfWarWinStreak);
	query.Bind(&results[3], &extrastats->m_CurrentDominationPerfectStreak);
	query.Bind(&results[4], &extrastats->m_CurrentAssaultPerfectStreak);
	query.Bind(&results[5], &extrastats->m_CurrentTugOfWarPerfectStreak);
	query.Bind(&results[6], &extrastats->m_CurrentNukesDeployedStreak);
	query.Bind(&results[7], &extrastats->m_NumberOfTimesBestPlayer);
	query.Bind(&results[8], &extrastats->m_CurrentBestPlayerStreak);
	query.Bind(&results[9], &extrastats->m_NumberOfTimesBestInfantry);
	query.Bind(&results[10], &extrastats->m_CurrentBestInfantryStreak);
	query.Bind(&results[11], &extrastats->m_NumberOfTimesBestSupport);
	query.Bind(&results[12], &extrastats->m_CurrentBestSupportStreak);
	query.Bind(&results[13], &extrastats->m_NumberOfTimesBestAir);
	query.Bind(&results[14], &extrastats->m_CurrentBestAirStreak);
	query.Bind(&results[15], &extrastats->m_NumberOfTimesBestArmor);
	query.Bind(&results[16], &extrastats->m_CurrentBestArmorStreak);
	query.Bind(&results[17], &extrastats->m_NumberOfCommandPointCaptures);

	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("QueryProfileExtraStats() failed:");
	}
	else
	{
		if (!query.StmtFetch())
			DatabaseLog("extra stats not found");
		//else
		//	DatabaseLog("extra stats found");
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::VerifyServerKey(const uint sequenceNum, uint *dstId)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT id FROM %s WHERE sequencenum = ? LIMIT 1", TABLENAME[SERVERKEYS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND param[1], result[1];
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	uint id;

	query.Bind(&param[0], &sequenceNum);
	query.Bind(&result[0], &id);

	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("VerifyServerKey() failed:");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
			*dstId = 0;
		else
			*dstId = id;
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::InsertPlayerMatchStats(const uint datematchplayed, const MMG_Stats::PlayerMatchStats playerstats)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "INSERT INTO %s (datematchplayed, profileid, timeplayedasusa, timeplayedasussr, timeplayedasnato, timeplayedasinfantry, timeplayedasair, timeplayedasarmor, timeplayedassupport, scoretotal, scoreasinfantry, scoreassupport, scoreasarmor, scoreasair, scorebydamagingenemies, scorebyrepairing, scorebyusingtacticalaid, scorebycommandpointcaptures, scorebyfortifying, scorelostbykillingfriendly, numberofunitskilled, numberofunitslost, numberofcommandpointcaptures, numberofreinforcementpointsspent, numberoftacticalaidpointsspent, numberofnukesdeployed, numberoftacticalaidcriticalhits, numberofrolechanges, timetotalmatchlength, matchtype, matchwon, matchlost, matchwasflawlessvictory, bestdata) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", TABLENAME[PLAYER_MATCHSTATS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);

	MYSQL_BIND params[34];
	memset(params, 0, sizeof(params));
	
	// NOTE: saving buffer_type short to column type INT
	query.Bind(&params[0], &datematchplayed);
	query.Bind(&params[1], &playerstats.m_ProfileId);
	query.Bind(&params[2], &playerstats.m_TimePlayedAsUSA);
	query.Bind(&params[3], &playerstats.m_TimePlayedAsUSSR);
	query.Bind(&params[4], &playerstats.m_TimePlayedAsNATO);
	query.Bind(&params[5], &playerstats.m_TimePlayedAsInfantry);
	query.Bind(&params[6], &playerstats.m_TimePlayedAsAir);
	query.Bind(&params[7], &playerstats.m_TimePlayedAsArmor);
	query.Bind(&params[8], &playerstats.m_TimePlayedAsSupport);
	query.Bind(&params[9], &playerstats.m_ScoreTotal);
	query.Bind(&params[10], &playerstats.m_ScoreAsInfantry);
	query.Bind(&params[11], &playerstats.m_ScoreAsSupport);
	query.Bind(&params[12], &playerstats.m_ScoreAsArmor);
	query.Bind(&params[13], &playerstats.m_ScoreAsAir);
	query.Bind(&params[14], &playerstats.m_ScoreByDamagingEnemies);
	query.Bind(&params[15], &playerstats.m_ScoreByRepairing);
	query.Bind(&params[16], &playerstats.m_ScoreByUsingTacticalAids);
	query.Bind(&params[17], &playerstats.m_ScoreByCommandPointCaptures);
	query.Bind(&params[18], &playerstats.m_ScoreByFortifying);
	query.Bind(&params[19], &playerstats.m_ScoreLostByKillingFriendly);
	query.Bind(&params[20], &playerstats.m_NumberOfUnitsKilled);
	query.Bind(&params[21], &playerstats.m_NumberOfUnitsLost);
	query.Bind(&params[22], &playerstats.m_NumberOfCommandPointCaptures);
	query.Bind(&params[23], &playerstats.m_NumberOfReinforcementPointsSpent);
	query.Bind(&params[24], &playerstats.m_NumberOfTacticalAidPointsSpent);
	query.Bind(&params[25], &playerstats.m_NumberOfNukesDeployed);
	query.Bind(&params[26], &playerstats.m_NumberOfTacticalAidCriticalHits);
	query.Bind(&params[27], &playerstats.m_NumberOfRoleChanges);
	query.Bind(&params[28], &playerstats.m_TimeTotalMatchLength);
	query.Bind(&params[29], &playerstats.m_MatchType);
	query.Bind(&params[30], &playerstats.m_MatchWon);
	query.Bind(&params[31], &playerstats.m_MatchLost);
	query.Bind(&params[32], &playerstats.m_MatchWasFlawlessVictory);
	query.Bind(&params[33], &playerstats.m_BestData);

	if (!query.StmtExecute(params))
		DatabaseLog("InsertPlayerMatchStats() failed:");

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::DeletePlayerLadder()
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "DELETE FROM %s", TABLENAME[PLAYER_LADDER_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);

	if (!query.StmtExecute())
		DatabaseLog("DeletePlayerLadder() failed:");

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::InsertPlayerLadderItem(const uint id, const uint profileid, const uint ladderscore)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "INSERT INTO %s (id, profileid, ladderscore) VALUES (?, ?, ?)", this->TABLENAME[PLAYER_LADDER_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);

	MYSQL_BIND params[3];
	memset(params, 0, sizeof(params));

	query.Bind(&params[0], &id);
	query.Bind(&params[1], &profileid);
	query.Bind(&params[2], &ladderscore);

	if (!query.StmtExecute(params))
		DatabaseLog("InsertPlayerLadderItem() failed:");

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::GeneratePlayerLadderData(const uint datematchplayed)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "SELECT a.profileid, CEIL(SUM(IF(a.matchwon = 1, a.scoretotal * 1.5, a.scoretotal))) AS ladderscore "
				 "FROM (SELECT * FROM %s WHERE datematchplayed >= ? - 1814400) AS a "
				 "WHERE (SELECT COUNT(id) FROM %s AS b WHERE b.profileid = a.profileid AND b.scoretotal >= a.scoretotal AND b.datematchplayed >= ? - 1814400 AND a.datematchplayed >= ? - 1814400) <= 20 "
				 "GROUP BY a.profileid "
				 "ORDER BY ladderscore DESC", TABLENAME[PLAYER_MATCHSTATS_TABLE], TABLENAME[PLAYER_MATCHSTATS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);

	MYSQL_BIND params[3], results[2];
	memset(params, 0, sizeof(params));
	memset(results, 0, sizeof(results));

	uint id=0, profileid=0, ladderscore=0;

	query.Bind(&params[0], &datematchplayed);
	query.Bind(&params[1], &datematchplayed);
	query.Bind(&params[2], &datematchplayed);

	query.Bind(&results[0], &profileid);
	query.Bind(&results[1], &ladderscore);

	if (!query.StmtExecute(params, results))
		DatabaseLog("GeneratePlayerLadderData() failed:");
	else
	{
		ulong count = (ulong)query.StmtNumRows();

		if (count > 0)
		{
			while(query.StmtFetch())
			{
				if (ladderscore > 0)
				{
					if (!InsertPlayerLadderItem(++id, profileid, ladderscore))
						return false;
				}
			}
		}
	}

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::BuildPlayerLeaderboard(const uint datematchplayed)
{
	BeginTransaction();

	if (!DeletePlayerLadder())
	{
		RollbackTransaction();
		return false;
	}

	// todo: this needs to be done daily at midnight
	uint daterange = ROUND_TIME(datematchplayed);

	// this will take longer as more matches are played
	if (!GeneratePlayerLadderData(daterange))
	{
		RollbackTransaction();
		return false;
	}

	CommitTransaction();

	return true;
}

bool MySQLDatabase::UpdateProfileMedals(const uint profileId, const size_t Count, MMG_Stats::Medal medals[])
{
	uint buffer[256];
	memset(buffer, 0, sizeof(buffer));

	MMG_BitWriter<unsigned int> writer(buffer, sizeof(buffer) * 8);

	for (uint i = 0; i < Count; i++)
	{
		writer.WriteBits(medals[i].level, 2);
		writer.WriteBits(medals[i].stars, 2);
	}

	if (!UpdateProfileMedalsRawData(profileId, buffer, sizeof(buffer)))
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileBadges(const uint profileId, const size_t Count, MMG_Stats::Badge badges[])
{
	uint buffer[256];
	memset(buffer, 0, sizeof(buffer));

	MMG_BitWriter<unsigned int> writer(buffer, sizeof(buffer) * 8);

	for (uint i = 0; i < Count; i++)
	{
		writer.WriteBits(badges[i].level, 2);
		writer.WriteBits(badges[i].stars, 2);
	}

	if (!UpdateProfileBadgesRawData(profileId, buffer, sizeof(buffer)))
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileMedalsRawData(const uint profileId, voidptr_t Data, ulong Length)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET medaldata = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2];
	memset(params, 0, sizeof(params));

	query.BindBlob(&params[0], Data, Length);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateProfileMedalsRawData() failed:");

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileBadgesRawData(const uint profileId, voidptr_t Data, ulong Length)
{
	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET badgedata = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[2];
	memset(params, 0, sizeof(params));

	query.BindBlob(&params[0], Data, Length);
	query.Bind(&params[1], &profileId);

	if(!query.StmtExecute(params))
		DatabaseLog("UpdateProfileBadgesRawData() failed:");

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::UpdateProfileMatchStats(const uint profileId, const uint datematchplayed, const MMG_Stats::PlayerMatchStats *playerstats)
{
	char SQL[8192];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "UPDATE %s SET "
				 "lastmatchplayed = ?, "
				 "scoretotal = scoretotal + ?, "
				 "scoreasinfantry = scoreasinfantry + ?, "
				 "highscoreasinfantry = IF (? > highscoreasinfantry, ?, highscoreasinfantry), "
				 "scoreassupport = scoreassupport + ?, "
				 "highscoreassupport = IF (? > highscoreassupport, ?, highscoreassupport), "
				 "scoreasarmor = scoreasarmor + ?, "
				 "highscoreasarmor = IF (? > highscoreasarmor, ?, highscoreasarmor), "
				 "scoreasair = scoreasair + ?, "
				 "highscoreasair = IF (? > highscoreasair, ?, highscoreasair), "
				 "scorebydamagingenemies = scorebydamagingenemies + ?, "
				 "scorebyusingtacticalaid = scorebyusingtacticalaid + ?, "
				 "scorebycapturingcommandpoints = scorebycapturingcommandpoints + ?, "
				 "scorebyrepairing = scorebyrepairing + ?, "
				 "scorebyfortifying = scorebyfortifying + ?, "
				 "scorelostbykillingfriendly = scorelostbykillingfriendly + ?, "
				 "highestscore = IF (? > highestscore, ?, highestscore), "
				 "timetotalmatchlength = timetotalmatchlength + ?, "
				 "timeplayedasusa = timeplayedasusa + ?, "
				 "timeplayedasussr = timeplayedasussr + ?, "
				 "timeplayedasnato = timeplayedasnato + ?, "
				 "timeplayedasinfantry = timeplayedasinfantry + ?, "
				 "timeplayedassupport = timeplayedassupport + ?, "
				 "timeplayedasarmor = timeplayedasarmor + ?, "
				 "timeplayedasair = timeplayedasair + ?, "
				 "numberofmatches = numberofmatches + 1, "
				 "numberofmatcheswon = numberofmatcheswon + ?, "
				 "numberofmatcheslost = numberofmatcheslost + ?, "
				 "numberofassaultmatches = IF (? = 1, numberofassaultmatches + 1, numberofassaultmatches), "
				 "numberofassaultmatcheswon = IF (? = 1 AND ? > 0, numberofassaultmatcheswon + 1, numberofassaultmatcheswon), "
				 "currentassaultwinstreak = IF (? = 1 AND ? > 0, currentassaultwinstreak + 1, 0), "
				 "numberofdominationmatches = IF (? = 0, numberofdominationmatches + 1, numberofdominationmatches), "
				 "numberofdominationmatcheswon = IF (? = 0 AND ? > 0, numberofdominationmatcheswon + 1, numberofdominationmatcheswon), "
				 "currentdominationwinstreak = IF (? = 0 AND ? > 0, currentdominationwinstreak + 1, 0), "
				 "numberoftugofwarmatches = IF (? = 2, numberoftugofwarmatches + 1, numberoftugofwarmatches), "
				 "numberoftugofwarmatcheswon = IF (? = 2 AND ? > 0, numberoftugofwarmatcheswon + 1, numberoftugofwarmatcheswon), "
				 "currenttugofwarwinstreak = IF (? = 2 AND ? > 0, currenttugofwarwinstreak + 1, 0), "
				 "currentwinningstreak = IF (? > 0, currentwinningstreak + 1, 0), "
				 "bestwinningstreak = IF (currentwinningstreak > bestwinningstreak, currentwinningstreak, bestwinningstreak), "
				 "numberofmatcheswonbytotaldomination = IF (? = 0 AND ? > 0, numberofmatcheswonbytotaldomination + 1, numberofmatcheswonbytotaldomination), "
				 "currentdominationperfectstreak = IF (? = 0 AND ? > 0, currentdominationperfectstreak + 1, 0), "
				 "numberofperfectdefendsinassaultmatch = IF (? = 1 AND ? > 0, numberofperfectdefendsinassaultmatch + 1, numberofperfectdefendsinassaultmatch), "
				 "currentassaultperfectstreak = IF (? = 1 AND ? > 0, currentassaultperfectstreak + 1, 0), "
				 "numberofperfectpushesintugofwarmatch = IF (? = 2 AND ? > 0, numberofperfectpushesintugofwarmatch + 1, numberofperfectpushesintugofwarmatch), "
				 "currenttugofwarperfectstreak = IF (? = 2 AND ? > 0, currenttugofwarperfectstreak + 1, 0), "
				 "numberofunitskilled = numberofunitskilled + ?, "
				 "numberofunitslost = numberofunitslost + ?, "
				 "numberofcommandpointcaptures = numberofcommandpointcaptures + ?, "
				 "numberofreinforcementpointsspent = numberofreinforcementpointsspent + ?, "
				 "numberoftacticalaidpointsspent = numberoftacticalaidpointsspent + ?, "
				 "numberofnukesdeployed = numberofnukesdeployed + ?, "
				 "currentnukesdeployedstreak = IF (? > 0, currentnukesdeployedstreak + 1, 0), "
				 "numberoftacticalaidcriticalhits = numberoftacticalaidcriticalhits + ?, "
				 "numberoftimesbestplayer = IF (? > 0, numberoftimesbestplayer + 1, numberoftimesbestplayer), "
				 "currentbestplayerstreak = IF (? > 0, currentbestplayerstreak + 1, 0), "
				 "numberoftimesbestinfantry = IF (? > 0, numberoftimesbestinfantry + 1, numberoftimesbestinfantry), "
				 "currentbestinfantrystreak = IF (? > 0, currentbestinfantrystreak + 1, 0), "
				 "numberoftimesbestsupport = IF (? > 0, numberoftimesbestsupport + 1, numberoftimesbestsupport), "
				 "currentbestsupportstreak = IF (? > 0, currentbestsupportstreak + 1, 0), "
				 "numberoftimesbestair = IF (? > 0, numberoftimesbestair + 1, numberoftimesbestair), "
				 "currentbestairstreak = IF (? > 0, currentbestairstreak + 1, 0), "
				 "numberoftimesbestarmor = IF (? > 0, numberoftimesbestarmor + 1, numberoftimesbestarmor), "
				 "currentbestarmorstreak = IF (? > 0, currentbestarmorstreak + 1, 0) "
				 "WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[79];
	memset(params, 0, sizeof(params));

	uint i = 0;
	uchar bestplayer = 0;
	uchar bestinfantry = 0;
	uchar bestsupport = 0;
	uchar bestair = 0;
	uchar bestarmor = 0;

	if (playerstats->m_BestData & 0x01)
		bestplayer = 1;

	if (playerstats->m_BestData & 0x02)
		bestinfantry = 1;

	if (playerstats->m_BestData & 0x04)
		bestsupport = 1;

	if (playerstats->m_BestData & 0x08)
		bestair = 1;

	if (playerstats->m_BestData & 0x10)
		bestarmor = 1;

	// NOTE: saving buffer_type short to column type INT

	// lastmatchplayed
	query.Bind(&params[i++], &datematchplayed);

	// scoretotal
	query.Bind(&params[i++], &playerstats->m_ScoreTotal);

	// scoreasinfantry
	query.Bind(&params[i++], &playerstats->m_ScoreAsInfantry);

	// highscoreasinfantry
	query.Bind(&params[i++], &playerstats->m_ScoreAsInfantry);
	query.Bind(&params[i++], &playerstats->m_ScoreAsInfantry);

	// scoreassupport
	query.Bind(&params[i++], &playerstats->m_ScoreAsSupport);

	// highscoreassupport
	query.Bind(&params[i++], &playerstats->m_ScoreAsSupport);
	query.Bind(&params[i++], &playerstats->m_ScoreAsSupport);

	// scoreasarmor
	query.Bind(&params[i++], &playerstats->m_ScoreAsArmor);

	// highscoreasarmor
	query.Bind(&params[i++], &playerstats->m_ScoreAsArmor);
	query.Bind(&params[i++], &playerstats->m_ScoreAsArmor);

	// scoreasair
	query.Bind(&params[i++], &playerstats->m_ScoreAsAir);

	// highscoreasair
	query.Bind(&params[i++], &playerstats->m_ScoreAsAir);
	query.Bind(&params[i++], &playerstats->m_ScoreAsAir);

	// scorebydamagingenemies
	query.Bind(&params[i++], &playerstats->m_ScoreByDamagingEnemies);

	// scorebyusingtacticalaid
	query.Bind(&params[i++], &playerstats->m_ScoreByUsingTacticalAids);

	// scorebycapturingcommandpoints
	query.Bind(&params[i++], &playerstats->m_ScoreByCommandPointCaptures);

	// scorebyrepairing
	query.Bind(&params[i++], &playerstats->m_ScoreByRepairing);

	// scorebyfortifying
	query.Bind(&params[i++], &playerstats->m_ScoreByFortifying);

	// scorelostbykillingfriendly
	query.Bind(&params[i++], &playerstats->m_ScoreLostByKillingFriendly);

	// highestscore
	query.Bind(&params[i++], &playerstats->m_ScoreTotal);
	query.Bind(&params[i++], &playerstats->m_ScoreTotal);

	// timetotalmatchlength
	query.Bind(&params[i++], &playerstats->m_TimeTotalMatchLength);

	// timeplayedasusa
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsUSA);

	// timeplayedasussr
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsUSSR);

	// timeplayedasnato
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsNATO);

	// timeplayedasinfantry
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsInfantry);

	// timeplayedassupport
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsSupport);

	// timeplayedasarmor
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsArmor);

	// timeplayedasair
	query.Bind(&params[i++], &playerstats->m_TimePlayedAsAir);

	// numberofmatcheswon
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// numberofmatcheslost
	query.Bind(&params[i++], &playerstats->m_MatchLost);

	// numberofassaultmatches
	query.Bind(&params[i++], &playerstats->m_MatchType);

	// numberofassaultmatcheswon
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// currentassaultwinstreak
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// numberofdominationmatches
	query.Bind(&params[i++], &playerstats->m_MatchType);

	// numberofdominationmatcheswon
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// currentdominationwinstreak
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// numberoftugofwarmatches
	query.Bind(&params[i++], &playerstats->m_MatchType);

	// numberoftugofwarmatcheswon
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// currenttugofwarwinstreak
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// currentwinningstreak
	query.Bind(&params[i++], &playerstats->m_MatchWon);

	// numberofmatcheswonbytotaldomination
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWasFlawlessVictory);

	// currentdominationperfectstreak
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWasFlawlessVictory);

	// numberofperfectdefendsinassaultmatch
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWasFlawlessVictory);

	// currentassaultperfectstreak
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWasFlawlessVictory);

	// numberofperfectpushesintugofwarmatch
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWasFlawlessVictory);

	// currenttugofwarperfectstreak
	query.Bind(&params[i++], &playerstats->m_MatchType);
	query.Bind(&params[i++], &playerstats->m_MatchWasFlawlessVictory);

	// numberofunitskilled
	query.Bind(&params[i++], &playerstats->m_NumberOfUnitsKilled);

	// numberofunitslost
	query.Bind(&params[i++], &playerstats->m_NumberOfUnitsLost);

	// numberofcommandpointcaptures
	query.Bind(&params[i++], &playerstats->m_NumberOfCommandPointCaptures);

	// numberofreinforcementpointsspent
	query.Bind(&params[i++], &playerstats->m_NumberOfReinforcementPointsSpent);

	// numberoftacticalaidpointsspent
	query.Bind(&params[i++], &playerstats->m_NumberOfTacticalAidPointsSpent);

	// numberofnukesdeployed
	query.Bind(&params[i++], &playerstats->m_NumberOfNukesDeployed);

	// currentnukesdeployedstreak
	query.Bind(&params[i++], &playerstats->m_NumberOfNukesDeployed);

	// numberoftacticalaidcriticalhits
	query.Bind(&params[i++], &playerstats->m_NumberOfTacticalAidCriticalHits);

	// numberoftimesbestplayer
	query.Bind(&params[i++], &bestplayer);

	// currentbestplayerstreak
	query.Bind(&params[i++], &bestplayer);

	// numberoftimesbestinfantry
	query.Bind(&params[i++], &bestinfantry);

	// currentbestinfantrystreak
	query.Bind(&params[i++], &bestinfantry);

	// numberoftimesbestsupport
	query.Bind(&params[i++], &bestsupport);

	// currentbestsupportstreak
	query.Bind(&params[i++], &bestsupport);

	// numberoftimesbestair
	query.Bind(&params[i++], &bestair);

	// currentbestairstreak
	query.Bind(&params[i++], &bestair);

	// numberoftimesbestarmor
	query.Bind(&params[i++], &bestarmor);

	// currentbestarmorstreak
	query.Bind(&params[i++], &bestarmor);

	// id = profileId
	query.Bind(&params[i++], &playerstats->m_ProfileId);

	if (profileId != playerstats->m_ProfileId)
		return false;

	if (!query.StmtExecute(params))
		DatabaseLog("UpdateProfileMatchStats() failed:");

	if (!query.Success())
		return false;

	return true;
}

bool MySQLDatabase::ProcessMatchStatistics(const uint datematchplayed, const uint Count, const MMG_Stats::PlayerMatchStats playermatchstats[])
{
	if (!TestDatabase())
	{
		EmergencyMassgateDisconnect();
		return false;
	}

	BeginTransaction();

	for (uint i = 0; i < Count; i++)
	{
		const MMG_Stats::PlayerMatchStats *const matchStats = &playermatchstats[i];
		const uint profileId = matchStats->m_ProfileId;

		MMG_Stats::Medal medals[19];
		MMG_Stats::Badge badges[14];
		MMG_Stats::PlayerStatsRsp playerStats;
		MMG_Stats::ExtraPlayerStats extraStats;
		uint medalreadbuffer[256], badgereadbuffer[256];
		memset(medalreadbuffer, 0, sizeof(medalreadbuffer));
		memset(badgereadbuffer, 0, sizeof(badgereadbuffer));

		// for leaderboards
		if (!InsertPlayerMatchStats(datematchplayed, *matchStats))
		{
			DatabaseLog("could not save match statistics");
			RollbackTransaction();
			return false;
		}

		// for profile page stats
		if (!UpdateProfileMatchStats(profileId, datematchplayed, matchStats))
		{
			DatabaseLog("could not update player statistics");
			RollbackTransaction();
			return false;
		}

		if (!QueryProfileMedalsRawData(profileId, medalreadbuffer, sizeof(medalreadbuffer)) || !QueryProfileBadgesRawData(profileId, badgereadbuffer, sizeof(badgereadbuffer))
			|| !QueryProfileStats(profileId, &playerStats) || !QueryProfileExtraStats(profileId, &extraStats))
		{
			DatabaseLog("could not retrieve player statistics");
			RollbackTransaction();
			return false;
		}

		MMG_BitReader<unsigned int> medalreader(medalreadbuffer, sizeof(medalreadbuffer) * 8);
		MMG_BitReader<unsigned int> badgereader(badgereadbuffer, sizeof(badgereadbuffer) * 8);

		for (uint j = 0; j < 19; j++)
		{
			medals[j].level = medalreader.ReadBits(2);
			medals[j].stars = medalreader.ReadBits(2);
		}

		for (uint j = 0; j < 14; j++)
		{
			badges[j].level = badgereader.ReadBits(2);
			badges[j].stars = badgereader.ReadBits(2);
		}

		// infantry skill medal
		if ((matchStats->m_BestData & 0x02) && medals[0].level == 0)
		{
			if (extraStats.m_NumberOfTimesBestInfantry >= 1)
			{
				medals[0].level = 1;
				medals[0].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x02) && medals[0].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[0].stars < 3 && extraStats.m_NumberOfTimesBestInfantry >= req[medals[0].stars])
				medals[0].stars++;

			if (extraStats.m_NumberOfTimesBestInfantry >= 10)
			{
				medals[0].level = 2;
				medals[0].stars = 0;
			}

			if (extraStats.m_CurrentBestInfantryStreak >= 10)
			{
				medals[0].level = 3;
				medals[0].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x02) && medals[0].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[0].stars < 3 && extraStats.m_NumberOfTimesBestInfantry >= req[medals[0].stars])
				medals[0].stars++;

			if (extraStats.m_CurrentBestInfantryStreak >= 10)
			{
				medals[0].level = 3;
				medals[0].stars = 0;
			}
		}

		// air skill medal
		if ((matchStats->m_BestData & 0x08) && medals[1].level == 0)
		{
			if (extraStats.m_NumberOfTimesBestAir >= 1)
			{
				medals[1].level = 1;
				medals[1].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x08) && medals[1].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[1].stars < 3 && extraStats.m_NumberOfTimesBestAir >= req[medals[1].stars])
				medals[1].stars++;

			if (extraStats.m_NumberOfTimesBestAir >= 10)
			{
				medals[1].level = 2;
				medals[1].stars = 0;
			}

			if (extraStats.m_CurrentBestAirStreak >= 10)
			{
				medals[1].level = 3;
				medals[1].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x08) && medals[1].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[1].stars < 3 && extraStats.m_NumberOfTimesBestAir >= req[medals[1].stars])
				medals[1].stars++;

			if (extraStats.m_CurrentBestAirStreak >= 10)
			{
				medals[1].level = 3;
				medals[1].stars = 0;
			}
		}

		// armor skill medal
		if ((matchStats->m_BestData & 0x10) && medals[2].level == 0)
		{
			if (extraStats.m_NumberOfTimesBestArmor >= 1)
			{
				medals[2].level = 1;
				medals[2].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x10) && medals[2].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[2].stars < 3 && extraStats.m_NumberOfTimesBestArmor >= req[medals[2].stars])
				medals[2].stars++;

			if (extraStats.m_NumberOfTimesBestArmor >= 10)
			{
				medals[2].level = 2;
				medals[2].stars = 0;
			}

			if (extraStats.m_CurrentBestArmorStreak >= 10)
			{
				medals[2].level = 3;
				medals[2].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x10) && medals[2].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[2].stars < 3 && extraStats.m_NumberOfTimesBestArmor >= req[medals[2].stars])
				medals[2].stars++;

			if (extraStats.m_CurrentBestArmorStreak >= 10)
			{
				medals[2].level = 3;
				medals[2].stars = 0;
			}
		}

		// support skill medal
		if ((matchStats->m_BestData & 0x04) && medals[3].level == 0)
		{
			if (extraStats.m_NumberOfTimesBestSupport >= 1)
			{
				medals[3].level = 1;
				medals[3].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x04) && medals[3].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[3].stars < 3 && extraStats.m_NumberOfTimesBestSupport >= req[medals[3].stars])
				medals[3].stars++;

			if (extraStats.m_NumberOfTimesBestSupport >= 10)
			{
				medals[3].level = 2;
				medals[3].stars = 0;
			}

			if (extraStats.m_CurrentBestSupportStreak >= 10)
			{
				medals[3].level = 3;
				medals[3].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x04) && medals[3].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[3].stars < 3 && extraStats.m_NumberOfTimesBestSupport >= req[medals[3].stars])
				medals[3].stars++;

			if (extraStats.m_CurrentBestSupportStreak >= 10)
			{
				medals[3].level = 3;
				medals[3].stars = 0;
			}
		}

		// high score skill medal
		if ((matchStats->m_BestData & 0x01) && medals[4].level == 0)
		{
			if (extraStats.m_NumberOfTimesBestPlayer >= 1)
			{
				medals[4].level = 1;
				medals[4].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x01) && medals[4].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[4].stars < 3 && extraStats.m_NumberOfTimesBestPlayer >= req[medals[4].stars])
				medals[4].stars++;

			if (extraStats.m_NumberOfTimesBestPlayer >= 10)
			{
				medals[4].level = 2;
				medals[4].stars = 0;
			}

			if (extraStats.m_CurrentBestPlayerStreak >= 10)
			{
				medals[4].level = 3;
				medals[4].stars = 0;
			}
		}
		else if ((matchStats->m_BestData & 0x01) && medals[4].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[4].stars < 3 && extraStats.m_NumberOfTimesBestPlayer >= req[medals[4].stars])
				medals[4].stars++;

			if (extraStats.m_CurrentBestPlayerStreak >= 10)
			{
				medals[4].level = 3;
				medals[4].stars = 0;
			}
		}

		// tactical aid combat medal
		if (medals[5].level == 0)
		{
			if (playerStats.m_NumberOfTacticalAidCriticalHits >= 25)
			{
				medals[5].level = 1;
				medals[5].stars = 0;
			}

			if (matchStats->m_NumberOfTacticalAidCriticalHits >= 15)
			{
				medals[5].level = 3;
				medals[5].stars = 0;
			}
		}
		else if (medals[5].level == 1)
		{
			uint req[] = {50, 75, 100};

			if (medals[5].stars < 3 && playerStats.m_NumberOfTacticalAidCriticalHits >= req[medals[5].stars])
				medals[5].stars++;

			if (playerStats.m_NumberOfTacticalAidCriticalHits >= 125)
			{
				medals[5].level = 2;
				medals[5].stars = 0;
			}

			if (matchStats->m_NumberOfTacticalAidCriticalHits >= 15)
			{
				medals[5].level = 3;
				medals[5].stars = 0;
			}
		}
		else if (medals[5].level == 2)
		{
			uint req[] = {250, 375, 500};

			if (medals[5].stars < 3 && playerStats.m_NumberOfTacticalAidCriticalHits >= req[medals[5].stars])
				medals[5].stars++;

			if (matchStats->m_NumberOfTacticalAidCriticalHits >= 15)
			{
				medals[5].level = 3;
				medals[5].stars = 0;
			}
		}

		// infantry combat medal
		if (medals[6].level == 0)
		{
			if (matchStats->m_ScoreAsInfantry >= 500)
			{
				medals[6].level = 1;
				medals[6].stars = 0;
			}

			if (matchStats->m_ScoreAsInfantry >= 1200)
			{
				medals[6].level = 2;
				medals[6].stars = 0;
			}

			if (matchStats->m_ScoreAsInfantry >= 2000)
			{
				medals[6].level = 3;
				medals[6].stars = 0;
			}
		}
		else if (medals[6].level == 1)
		{
			if (medals[6].stars < 3 && matchStats->m_ScoreAsInfantry >= 500)
				medals[6].stars++;

			if (matchStats->m_ScoreAsInfantry >= 1200)
			{
				medals[6].level = 2;
				medals[6].stars = 0;
			}

			if (matchStats->m_ScoreAsInfantry >= 2000)
			{
				medals[6].level = 3;
				medals[6].stars = 0;
			}
		}
		else if (medals[6].level == 2)
		{
			if (medals[6].stars < 3 && matchStats->m_ScoreAsInfantry >= 1200)
				medals[6].stars++;

			if (matchStats->m_ScoreAsInfantry >= 2000)
			{
				medals[6].level = 3;
				medals[6].stars = 0;
			}
		}

		// air combat medal
		if (medals[7].level == 0)
		{
			if (matchStats->m_ScoreAsAir >= 500)
			{
				medals[7].level = 1;
				medals[7].stars = 0;
			}

			if (matchStats->m_ScoreAsAir >= 1200)
			{
				medals[7].level = 2;
				medals[7].stars = 0;
			}

			if (matchStats->m_ScoreAsAir >= 2000)
			{
				medals[7].level = 3;
				medals[7].stars = 0;
			}
		}
		else if (medals[7].level == 1)
		{
			if (medals[7].stars < 3 && matchStats->m_ScoreAsAir >= 500)
				medals[7].stars++;

			if (matchStats->m_ScoreAsAir >= 1200)
			{
				medals[7].level = 2;
				medals[7].stars = 0;
			}

			if (matchStats->m_ScoreAsAir >= 2000)
			{
				medals[7].level = 3;
				medals[7].stars = 0;
			}
		}
		else if (medals[7].level == 2)
		{
			if (medals[7].stars < 3 && matchStats->m_ScoreAsAir >= 1200)
				medals[7].stars++;

			if (matchStats->m_ScoreAsAir >= 2000)
			{
				medals[7].level = 3;
				medals[7].stars = 0;
			}
		}

		//  armor combat medal
		if (medals[8].level == 0)
		{
			if (matchStats->m_ScoreAsArmor >= 500)
			{
				medals[8].level = 1;
				medals[8].stars = 0;
			}

			if (matchStats->m_ScoreAsArmor >= 1200)
			{
				medals[8].level = 2;
				medals[8].stars = 0;
			}

			if (matchStats->m_ScoreAsArmor >= 2000)
			{
				medals[8].level = 3;
				medals[8].stars = 0;
			}
		}
		else if (medals[8].level == 1)
		{
			if (medals[8].stars < 3 && matchStats->m_ScoreAsArmor >= 500)
				medals[8].stars++;

			if (matchStats->m_ScoreAsArmor >= 1200)
			{
				medals[8].level = 2;
				medals[8].stars = 0;
			}

			if (matchStats->m_ScoreAsArmor >= 2000)
			{
				medals[8].level = 3;
				medals[8].stars = 0;
			}
		}
		else if (medals[8].level == 2)
		{
			if (medals[8].stars < 3 && matchStats->m_ScoreAsArmor >= 1200)
				medals[8].stars++;

			if (matchStats->m_ScoreAsArmor >= 2000)
			{
				medals[8].level = 3;
				medals[8].stars = 0;
			}
		}

		// support combat medal
		if (medals[9].level == 0)
		{
			if (matchStats->m_ScoreAsSupport >= 500)
			{
				medals[9].level = 1;
				medals[9].stars = 0;
			}

			if (matchStats->m_ScoreAsSupport >= 1200)
			{
				medals[9].level = 2;
				medals[9].stars = 0;
			}

			if (matchStats->m_ScoreAsSupport >= 2000)
			{
				medals[9].level = 3;
				medals[9].stars = 0;
			}
		}
		else if (medals[9].level == 1)
		{
			if (medals[9].stars < 3 && matchStats->m_ScoreAsSupport >= 500)
				medals[9].stars++;

			if (matchStats->m_ScoreAsSupport >= 1200)
			{
				medals[9].level = 2;
				medals[9].stars = 0;
			}

			if (matchStats->m_ScoreAsSupport >= 2000)
			{
				medals[9].level = 3;
				medals[9].stars = 0;
			}
		}
		else if (medals[9].level == 2)
		{
			if (medals[9].stars < 3 && matchStats->m_ScoreAsSupport >= 1200)
				medals[9].stars++;

			if (matchStats->m_ScoreAsSupport >= 2000)
			{
				medals[9].level = 3;
				medals[9].stars = 0;
			}
		}

		// winning streak match medal
		if (medals[10].level == 0)
		{
			if (playerStats.m_CurrentWinningStreak >= 5)
			{
				medals[10].level = 1;
				medals[10].stars = 0;
			}
		}
		else if (medals[10].level == 1)
		{
			if (medals[10].stars < 3 && playerStats.m_CurrentWinningStreak > 0 && playerStats.m_CurrentWinningStreak % 5 == 0)
				medals[10].stars++;

			if (playerStats.m_CurrentWinningStreak >= 10)
			{
				medals[10].level = 2;
				medals[10].stars = 0;
			}
		}
		else if (medals[10].level == 2)
		{
			if (medals[10].stars < 3 && playerStats.m_CurrentWinningStreak > 0 && playerStats.m_CurrentWinningStreak % 10 == 0)
				medals[10].stars++;

			if (playerStats.m_CurrentWinningStreak >= 20)
			{
				medals[10].level = 3;
				medals[10].stars = 0;
			}
		}

		// domination specialist
		if (medals[11].level == 0)
		{
			if (playerStats.m_NumberOfDominationMatchesWon >= 10)
			{
				medals[11].level = 1;
				medals[11].stars = 0;
			}

			if (extraStats.m_CurrentDominationWinStreak >= 10)
			{
				medals[11].level = 3;
				medals[11].stars = 0;
			}
		}
		else if (medals[11].level == 1)
		{
			uint req[] = {20, 30, 40};

			if (medals[11].stars < 3 && playerStats.m_NumberOfDominationMatchesWon >= req[medals[11].stars])
				medals[11].stars++;

			if (playerStats.m_NumberOfDominationMatchesWon >= 100)
			{
				medals[11].level = 2;
				medals[11].stars = 0;
			}

			if (extraStats.m_CurrentDominationWinStreak >= 10)
			{
				medals[11].level = 3;
				medals[11].stars = 0;
			}
		}
		else if (medals[11].level == 2)
		{
			uint req[] = {200, 300, 400};

			if (medals[11].stars < 3 && playerStats.m_NumberOfDominationMatchesWon >= req[medals[11].stars])
				medals[11].stars++;

			if (extraStats.m_CurrentDominationWinStreak >= 10)
			{
				medals[11].level = 3;
				medals[11].stars = 0;
			}
		}

		// domination excellency
		if (medals[12].level == 0)
		{
			if (playerStats.m_NumberOfMatchesWonByTotalDomination >= 1)
			{
				medals[12].level = 1;
				medals[12].stars = 0;
			}
		}
		else if (medals[12].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[12].stars < 3 && playerStats.m_NumberOfMatchesWonByTotalDomination >= req[medals[12].stars])
				medals[12].stars++;

			if (playerStats.m_NumberOfMatchesWonByTotalDomination >= 10)
			{
				medals[12].level = 2;
				medals[12].stars = 0;
			}

			if (extraStats.m_CurrentDominationPerfectStreak >= 5)
			{
				medals[12].level = 3;
				medals[12].stars = 0;
			}
		}
		else if (medals[12].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[12].stars < 3 && playerStats.m_NumberOfMatchesWonByTotalDomination >= req[medals[12].stars])
				medals[12].stars++;

			if (extraStats.m_CurrentDominationPerfectStreak >= 5)
			{
				medals[12].level = 3;
				medals[12].stars = 0;
			}
		}

		// assault specialist
		if (medals[13].level == 0)
		{
			if (playerStats.m_NumberOfAssaultMatchesWon >= 10)
			{
				medals[13].level = 1;
				medals[13].stars = 0;
			}

			if (extraStats.m_CurrentAssaultWinStreak >= 10)
			{
				medals[13].level = 3;
				medals[13].stars = 0;
			}
		}
		else if (medals[13].level == 1)
		{
			uint req[] = {20, 30, 40};

			if (medals[13].stars < 3 && playerStats.m_NumberOfAssaultMatchesWon >= req[medals[13].stars])
				medals[13].stars++;

			if (playerStats.m_NumberOfAssaultMatchesWon >= 100)
			{
				medals[13].level = 2;
				medals[13].stars = 0;
			}

			if (extraStats.m_CurrentAssaultWinStreak >= 10)
			{
				medals[13].level = 3;
				medals[13].stars = 0;
			}
		}
		else if (medals[13].level == 2)
		{
			uint req[] = {200, 300, 400};

			if (medals[13].stars < 3 && playerStats.m_NumberOfAssaultMatchesWon >= req[medals[13].stars])
				medals[13].stars++;

			if (extraStats.m_CurrentAssaultWinStreak >= 10)
			{
				medals[13].level = 3;
				medals[13].stars = 0;
			}
		}

		// assault excellency
		if (medals[14].level == 0)
		{
			if (playerStats.m_NumberOfPerfectDefendsInAssaultMatch >= 1)
			{
				medals[14].level = 1;
				medals[14].stars = 0;
			}
		}
		else if (medals[14].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[14].stars < 3 && playerStats.m_NumberOfPerfectDefendsInAssaultMatch >= req[medals[14].stars])
				medals[14].stars++;

			if (playerStats.m_NumberOfPerfectDefendsInAssaultMatch >= 10)
			{
				medals[14].level = 2;
				medals[14].stars = 0;
			}

			if (extraStats.m_CurrentAssaultPerfectStreak >= 5)
			{
				medals[14].level = 3;
				medals[14].stars = 0;
			}
		}
		else if (medals[14].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[14].stars < 3 && playerStats.m_NumberOfPerfectDefendsInAssaultMatch >= req[medals[14].stars])
				medals[14].stars++;

			if (extraStats.m_CurrentAssaultPerfectStreak >= 5)
			{
				medals[14].level = 3;
				medals[14].stars = 0;
			}
		}

		// tug of war specialist
		if (medals[15].level == 0)
		{
			if (playerStats.m_NumberOfTugOfWarMatchesWon >= 10)
			{
				medals[15].level = 1;
				medals[15].stars = 0;
			}

			if (extraStats.m_CurrentTugOfWarWinStreak >= 10)
			{
				medals[15].level = 3;
				medals[15].stars = 0;
			}
		}
		else if (medals[15].level == 1)
		{
			uint req[] = {20, 30, 40};

			if (medals[15].stars < 3 && playerStats.m_NumberOfTugOfWarMatchesWon >= req[medals[15].stars])
				medals[15].stars++;

			if (playerStats.m_NumberOfTugOfWarMatchesWon >= 100)
			{
				medals[15].level = 2;
				medals[15].stars = 0;
			}

			if (extraStats.m_CurrentTugOfWarWinStreak >= 10)
			{
				medals[15].level = 3;
				medals[15].stars = 0;
			}
		}
		else if (medals[15].level == 2)
		{
			uint req[] = {200, 300, 400};

			if (medals[15].stars < 3 && playerStats.m_NumberOfTugOfWarMatchesWon >= req[medals[15].stars])
				medals[15].stars++;

			if (extraStats.m_CurrentTugOfWarWinStreak >= 10)
			{
				medals[15].level = 3;
				medals[15].stars = 0;
			}
		}

		// tug of war excellency
		if (medals[16].level == 0)
		{
			if (playerStats.m_NumberOfPerfectPushesInTugOfWarMatch >= 1)
			{
				medals[16].level = 1;
				medals[16].stars = 0;
			}
		}
		else if (medals[16].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[16].stars < 3 && playerStats.m_NumberOfPerfectPushesInTugOfWarMatch >= req[medals[16].stars])
				medals[16].stars++;

			if (playerStats.m_NumberOfPerfectPushesInTugOfWarMatch >= 10)
			{
				medals[16].level = 2;
				medals[16].stars = 0;
			}

			if (extraStats.m_CurrentTugOfWarPerfectStreak >= 5)
			{
				medals[16].level = 3;
				medals[16].stars = 0;
			}
		}
		else if (medals[16].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[16].stars < 3 && playerStats.m_NumberOfPerfectPushesInTugOfWarMatch >= req[medals[16].stars])
				medals[16].stars++;

			if (extraStats.m_CurrentTugOfWarPerfectStreak >= 5)
			{
				medals[16].level = 3;
				medals[16].stars = 0;
			}
		}

		// nuclear specialist
		if (medals[17].level == 0)
		{
			if (playerStats.m_NumberOfNukesDeployed >= 1)
			{
				medals[17].level = 1;
				medals[17].stars = 0;
			}
		}
		else if (medals[17].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (medals[17].stars < 3 && playerStats.m_NumberOfNukesDeployed >= req[medals[17].stars])
				medals[17].stars++;

			if (playerStats.m_NumberOfNukesDeployed >= 10)
			{
				medals[17].level = 2;
				medals[17].stars = 0;
			}

			if (extraStats.m_CurrentNukesDeployedStreak >= 8)
			{
				medals[17].level = 3;
				medals[17].stars = 0;
			}
		}
		else if (medals[17].level == 2)
		{
			uint req[] = {20, 30, 40};

			if (medals[17].stars < 3 && playerStats.m_NumberOfNukesDeployed >= req[medals[17].stars])
				medals[17].stars++;

			if (extraStats.m_CurrentNukesDeployedStreak >= 8)
			{
				medals[17].level = 3;
				medals[17].stars = 0;
			}
		}

		// infantry specialist badge
		if (badges[0].level == 0)
		{
			if (playerStats.m_ScoreAsInfantry >= 8000)
			{
				badges[0].level = 1;
				badges[0].stars = 0;
			}
		}
		else if (badges[0].level == 1)
		{
			uint req[] = {16000, 24000, 32000};

			if (badges[0].stars < 3 && playerStats.m_ScoreAsInfantry >= req[badges[0].stars])
				badges[0].stars++;

			if (playerStats.m_ScoreAsInfantry >= 50000)
			{
				badges[0].level = 2;
				badges[0].stars = 0;
			}
		}
		else if (badges[0].level == 2)
		{
			uint req[] = {100000, 150000, 200000};

			if (badges[0].stars < 3 && playerStats.m_ScoreAsInfantry >= req[badges[0].stars])
				badges[0].stars++;

			if (playerStats.m_ScoreAsInfantry >= 400000)
			{
				badges[0].level = 3;
				badges[0].stars = 0;
			}
		}

		// air specialist badge
		if (badges[1].level == 0)
		{
			if (playerStats.m_ScoreAsAir >= 8000)
			{
				badges[1].level = 1;
				badges[1].stars = 0;
			}
		}
		else if (badges[1].level == 1)
		{
			uint req[] = {16000, 24000, 32000};

			if (badges[1].stars < 3 && playerStats.m_ScoreAsAir >= req[badges[1].stars])
				badges[1].stars++;

			if (playerStats.m_ScoreAsAir >= 50000)
			{
				badges[1].level = 2;
				badges[1].stars = 0;
			}
		}
		else if (badges[1].level == 2)
		{
			uint req[] = {100000, 150000, 200000};

			if (badges[1].stars < 3 && playerStats.m_ScoreAsAir >= req[badges[1].stars])
				badges[1].stars++;

			if (playerStats.m_ScoreAsAir >= 400000)
			{
				badges[1].level = 3;
				badges[1].stars = 0;
			}
		}

		// armor specialist badge
		if (badges[2].level == 0)
		{
			if (playerStats.m_ScoreAsArmor >= 8000)
			{
				badges[2].level = 1;
				badges[2].stars = 0;
			}
		}
		else if (badges[2].level == 1)
		{
			uint req[] = {16000, 24000, 32000};

			if (badges[2].stars < 3 && playerStats.m_ScoreAsArmor >= req[badges[2].stars])
				badges[2].stars++;

			if (playerStats.m_ScoreAsArmor >= 50000)
			{
				badges[2].level = 2;
				badges[2].stars = 0;
			}
		}
		else if (badges[2].level == 2)
		{
			uint req[] = {100000, 150000, 200000};

			if (badges[2].stars < 3 && playerStats.m_ScoreAsArmor >= req[badges[2].stars])
				badges[2].stars++;

			if (playerStats.m_ScoreAsArmor >= 400000)
			{
				badges[2].level = 3;
				badges[2].stars = 0;
			}
		}

		// support specialist badge
		if (badges[3].level == 0)
		{
			if (playerStats.m_ScoreAsSupport >= 8000)
			{
				badges[3].level = 1;
				badges[3].stars = 0;
			}
		}
		else if (badges[3].level == 1)
		{
			uint req[] = {16000, 24000, 32000};

			if (badges[3].stars < 3 && playerStats.m_ScoreAsSupport >= req[badges[3].stars])
				badges[3].stars++;

			if (playerStats.m_ScoreAsSupport >= 50000)
			{
				badges[3].level = 2;
				badges[3].stars = 0;
			}
		}
		else if (badges[3].level == 2)
		{
			uint req[] = {100000, 150000, 200000};

			if (badges[3].stars < 3 && playerStats.m_ScoreAsSupport >= req[badges[3].stars])
				badges[3].stars++;

			if (playerStats.m_ScoreAsSupport >= 400000)
			{
				badges[3].level = 3;
				badges[3].stars = 0;
			}
		}

		// score achievement badge
		if (badges[4].level == 0)
		{
			if (playerStats.m_ScoreTotal >= 10000)
			{
				badges[4].level = 1;
				badges[4].stars = 0;
			}
		}
		else if (badges[4].level == 1)
		{
			uint req[] = {20000, 30000, 40000};

			if (badges[4].stars < 3 && playerStats.m_ScoreTotal >= req[badges[4].stars])
				badges[4].stars++;

			if (playerStats.m_ScoreTotal >= 100000)
			{
				badges[4].level = 2;
				badges[4].stars = 0;
			}
		}
		else if (badges[4].level == 2)
		{
			uint req[] = {200000, 300000, 400000};

			if (badges[4].stars < 3 && playerStats.m_ScoreTotal >= req[badges[4].stars])
				badges[4].stars++;

			if (playerStats.m_ScoreTotal >= 800000)
			{
				badges[4].level = 3;
				badges[4].stars = 0;
			}
		}

		// command point achievement badge
		if (badges[5].level == 0)
		{
			if (extraStats.m_NumberOfCommandPointCaptures >= 25)
			{
				badges[5].level = 1;
				badges[5].stars = 0;
			}
		}
		else if (badges[5].level == 1)
		{
			uint req[] = {25, 50, 75};

			if (badges[5].stars < 3 && extraStats.m_NumberOfCommandPointCaptures >= req[badges[5].stars])
				badges[5].stars++;

			if (extraStats.m_NumberOfCommandPointCaptures >= 250)
			{
				badges[5].level = 2;
				badges[5].stars = 0;
			}
		}
		else if (badges[5].level == 2)
		{
			uint req[] = {500, 750, 1000};

			if (badges[5].stars < 3 && extraStats.m_NumberOfCommandPointCaptures >= req[badges[5].stars])
				badges[5].stars++;

			if (extraStats.m_NumberOfCommandPointCaptures >= 1200)
			{
				badges[5].level = 3;
				badges[5].stars = 0;
			}
		}

		// fortification achievement badge
		if (badges[6].level == 0)
		{
			if (playerStats.m_ScoreByFortifying >= 500)
			{
				badges[6].level = 1;
				badges[6].stars = 0;
			}
		}
		else if (badges[6].level == 1)
		{
			uint req[] = {1000, 1500, 2000};

			if (badges[6].stars < 3 && playerStats.m_ScoreByFortifying >= req[badges[6].stars])
				badges[6].stars++;

			if (playerStats.m_ScoreByFortifying >= 3500)
			{
				badges[6].level = 2;
				badges[6].stars = 0;
			}
		}
		else if (badges[6].level == 2)
		{
			uint req[] = {7000, 10500, 14000};

			if (badges[6].stars < 3 && playerStats.m_ScoreByFortifying >= req[badges[6].stars])
				badges[6].stars++;

			if (playerStats.m_ScoreByFortifying >= 15000)
			{
				badges[6].level = 3;
				badges[6].stars = 0;
			}
		}
		
		// match achievement badge
		if (badges[8].level == 0)
		{
			if (playerStats.m_NumberOfMatches >= 10)
			{
				badges[8].level = 1;
				badges[8].stars = 0;
			}
		}
		else if (badges[8].level == 1)
		{
			uint req[] = {20, 30, 40};

			if (badges[8].stars < 3 && playerStats.m_NumberOfMatches >= req[badges[8].stars])
				badges[8].stars++;

			if (playerStats.m_NumberOfMatches >= 200)
			{
				badges[8].level = 2;
				badges[8].stars = 0;
			}
		}
		else if (badges[8].level == 2)
		{
			uint req[] = {400, 600, 800};

			if (badges[8].stars < 3 && playerStats.m_NumberOfMatches >= req[badges[8].stars])
				badges[8].stars++;

			if (playerStats.m_NumberOfMatches >= 1500)
			{
				badges[8].level = 3;
				badges[8].stars = 0;
			}
		}

		// US achievement badge
		if (badges[9].level == 0)
		{
			if (playerStats.m_TimePlayedAsUSA / 3600 >= 1)
			{
				badges[9].level = 1;
				badges[9].stars = 0;
			}
		}
		else if (badges[9].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (badges[9].stars < 3 && playerStats.m_TimePlayedAsUSA / 3600 >= req[badges[9].stars])
				badges[9].stars++;

			if (playerStats.m_TimePlayedAsUSA / 3600 >= 24)
			{
				badges[9].level = 2;
				badges[9].stars = 0;
			}
		}
		else if (badges[9].level == 2)
		{
			uint req[] = {48, 72, 96};

			if (badges[9].stars < 3 && playerStats.m_TimePlayedAsUSA / 3600 >= req[badges[9].stars])
				badges[9].stars++;

			if (playerStats.m_TimePlayedAsUSA / 3600 >= 100)
			{
				badges[9].level = 3;
				badges[9].stars = 0;
			}
		}
		
		// USSR achievement badge
		if (badges[10].level == 0)
		{
			if (playerStats.m_TimePlayedAsUSSR / 3600 >= 1)
			{
				badges[10].level = 1;
				badges[10].stars = 0;
			}
		}
		else if (badges[10].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (badges[10].stars < 3 && playerStats.m_TimePlayedAsUSSR / 3600 >= req[badges[10].stars])
				badges[10].stars++;

			if (playerStats.m_TimePlayedAsUSSR / 3600 >= 24)
			{
				badges[10].level = 2;
				badges[10].stars = 0;
			}
		}
		else if (badges[10].level == 2)
		{
			uint req[] = {48, 72, 96};

			if (badges[10].stars < 3 && playerStats.m_TimePlayedAsUSSR / 3600 >= req[badges[10].stars])
				badges[10].stars++;

			if (playerStats.m_TimePlayedAsUSSR / 3600 >= 100)
			{
				badges[10].level = 3;
				badges[10].stars = 0;
			}
		}
		
		// NATO achievement badge
		if (badges[11].level == 0)
		{
			if (playerStats.m_TimePlayedAsNATO / 3600 >= 1)
			{
				badges[11].level = 1;
				badges[11].stars = 0;
			}
		}
		else if (badges[11].level == 1)
		{
			uint req[] = {2, 3, 4};

			if (badges[11].stars < 3 && playerStats.m_TimePlayedAsNATO / 3600 >= req[badges[11].stars])
				badges[11].stars++;

			if (playerStats.m_TimePlayedAsNATO / 3600 >= 24)
			{
				badges[11].level = 2;
				badges[11].stars = 0;
			}
		}
		else if (badges[11].level == 2)
		{
			uint req[] = {48, 72, 96};

			if (badges[11].stars < 3 && playerStats.m_TimePlayedAsNATO / 3600 >= req[badges[11].stars])
				badges[11].stars++;

			if (playerStats.m_TimePlayedAsNATO / 3600 >= 100)
			{
				badges[11].level = 3;
				badges[11].stars = 0;
			}
		}

		// highly decorated medal
		if (medals[18].level == 0)
		{
			if (medals[0].level > 0 && medals[1].level > 0 && medals[2].level > 0 && medals[3].level > 0 && medals[4].level > 0
				&& medals[5].level > 0 && medals[6].level > 0 && medals[7].level > 0 && medals[8].level > 0 && medals[9].level > 0
				&& medals[10].level > 0 && medals[11].level > 0 && medals[12].level > 0 && medals[13].level > 0 && medals[14].level > 0
				&& medals[15].level > 0 && medals[16].level > 0 && medals[17].level > 0
				&& badges[0].level > 0 && badges[1].level > 0 && badges[2].level > 0 && badges[3].level > 0 && badges[4].level > 0
				&& badges[5].level > 0 && badges[6].level > 0 && badges[7].level > 0 && badges[8].level > 0 && badges[9].level > 0
				&& badges[10].level > 0 && badges[11].level > 0 && badges[13].level > 0)
			{
				medals[18].level = 1;
				medals[18].stars = 0;
			}
		}

		uint medalwritebuffer[256], badgewritebuffer[256];
		memset(medalwritebuffer, 0, sizeof(medalwritebuffer));
		memset(badgewritebuffer, 0, sizeof(badgewritebuffer));

		MMG_BitWriter<unsigned int> medalwriter(medalwritebuffer, sizeof(medalwritebuffer) * 8);
		MMG_BitWriter<unsigned int> badgewriter(badgewritebuffer, sizeof(badgewritebuffer) * 8);

		for (uint j = 0; j < 19; j++)
		{
			medalwriter.WriteBits(medals[j].level, 2);
			medalwriter.WriteBits(medals[j].stars, 2);
		}

		for (uint j = 0; j < 14; j++)
		{
			badgewriter.WriteBits(badges[j].level, 2);
			badgewriter.WriteBits(badges[j].stars, 2);
		}

		if (memcmp(medalreadbuffer, medalwritebuffer, sizeof(medalwritebuffer)))
		{
			if (!UpdateProfileMedalsRawData(profileId, medalwritebuffer, sizeof(medalwritebuffer)))
			{
				DatabaseLog("could not update player medals");
				RollbackTransaction();
				return false;
			}
		}

		if (memcmp(badgereadbuffer, badgewritebuffer, sizeof(badgewritebuffer)))
		{
			if (!UpdateProfileBadgesRawData(profileId, badgewritebuffer, sizeof(badgewritebuffer)))
			{
				DatabaseLog("could not update player badges");
				RollbackTransaction();
				return false;
			}
		}
	}

	CommitTransaction();

	return true;
}

bool MySQLDatabase::CalculatePlayerRanks(const uint Count, const uint profileIds[])
{
	for (uint i = 0; i < Count; i++)
	{
		uint profileId = profileIds[i];
		MMG_Messaging::ProfileStateObserver tempObserver;
		MMG_Profile profile;
		MMG_Stats::PlayerStatsRsp playerStats;
		uint ladderPosition = 0;
		uint ladderCount = 0;

		if (!QueryProfileName(profileId, &profile))
			return false;

		if (!QueryProfileStats(profileId, &playerStats))
			return false;

		if (!QueryProfileLadderPosition(profileId, &ladderPosition))
			return false;

		if (!QueryPlayerLadderCount(&ladderCount))
			return false;

		uint totalScore = playerStats.m_ScoreTotal;
		uint perc = 100 - (uint)floor(((float)ladderPosition * 100 / ladderCount) + 0.5f);
		uchar rank = profile.m_Rank;

		if (rank == 0 && totalScore >= 600)						rank++;
		if (rank == 1 && totalScore >= 2000)					rank++;
		if (rank == 2 && totalScore >= 5000)					rank++;
		if (rank == 3 && totalScore >= 9000)					rank++;
		if (rank == 4 && totalScore >= 15000)					rank++;
		if (rank == 5 && totalScore >= 25000)					rank++;
		if (rank == 6 && totalScore >= 40000)					rank++;
		if (rank == 7 && totalScore >= 58000)					rank++;
		if (rank == 8 && totalScore >= 77000)					rank++;
		if (rank == 9 && totalScore >= 110000 && perc >= 33)	rank++;
		if (rank == 10 && totalScore >= 145000 && perc >= 55)	rank++;
		if (rank == 11 && totalScore >= 180000 && perc >= 70)	rank++;
		if (rank == 12 && totalScore >= 215000 && perc >= 80)	rank++;
		if (rank == 13 && totalScore >= 260000 && perc >= 87)	rank++;
		if (rank == 14 && totalScore >= 310000 && perc >= 91)	rank++;
		if (rank == 15 && totalScore >= 365000 && perc >= 95)	rank++;
		if (rank == 16 && totalScore >= 430000 && perc >= 98)	rank++;
		if (rank == 17 && totalScore >= 500000 && perc >= 99)	rank++;

		if (profile.m_Rank != rank)
		{
			SvClient *onlinePlayer = SvClientManager::ourInstance->FindPlayerByProfileId(profileId);
			if (onlinePlayer)
			{
				// todo:
				// scoresheet doesnt display new rank at end of round, but will show new
				// rank with the tab button after the scoresheet is hidden
				onlinePlayer->GetProfile()->setRank(rank);
			}
			else
			{
				// sets rank in database and updates all online players with new profile
				profile.addObserver(&tempObserver);
				profile.setRank(rank);
			}
		}
	}

	return true;
}

bool MySQLDatabase::InsertAcquaintances(const uint datematchplayed, const uint Count, const uint profileIds[])
{
	BeginTransaction();

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	sprintf(SQL, "INSERT INTO %s (dateplayedwith, profileid, acquaintanceprofileid, numberoftimesplayed) VALUES (?, ?, ?, 1) ON DUPLICATE KEY UPDATE dateplayedwith = VALUES(dateplayedwith), numberoftimesplayed = numberoftimesplayed + 1", TABLENAME[ACQUAINTANCES_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[3];
	memset(params, 0, sizeof(params));

	uint profileId = 0;
	uint acquaintanceId = 0;

	query.Bind(&params[0], &datematchplayed);
	query.Bind(&params[1], &profileId);
	query.Bind(&params[2], &acquaintanceId);

	for (uint i = 0; i < Count; i++)
	{
		for (uint j = 0; j < Count; j++)
		{
			if (profileIds[i] != profileIds[j])
			{

				profileId = profileIds[i];
				acquaintanceId = profileIds[j];

				if (!query.StmtExecute(params))
				{
					DatabaseLog("InsertAcquaintances() failed:");
					RollbackTransaction();
					return false;
				}
			}
		}
	}

	CommitTransaction();
	return true;
}