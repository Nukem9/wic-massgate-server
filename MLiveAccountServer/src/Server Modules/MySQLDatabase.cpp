#define _CRT_SECURE_NO_WARNINGS
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

	strncpy(this->TABLENAME[SERVERKEYS_TABLE],		strstr(settings[4], "=") + 1, sizeof(this->TABLENAME[SERVERKEYS_TABLE]));
	strncpy(this->TABLENAME[ACCOUNTS_TABLE],		strstr(settings[5], "=") + 1, sizeof(this->TABLENAME[ACCOUNTS_TABLE]));
	strncpy(this->TABLENAME[CDKEYS_TABLE],			strstr(settings[6], "=") + 1, sizeof(this->TABLENAME[CDKEYS_TABLE]));
	strncpy(this->TABLENAME[PROFILES_TABLE],		strstr(settings[7], "=") + 1, sizeof(this->TABLENAME[PROFILES_TABLE]));
	strncpy(this->TABLENAME[PROFILE_GB_TABLE],		strstr(settings[8], "=") + 1, sizeof(this->TABLENAME[PROFILE_GB_TABLE]));
	strncpy(this->TABLENAME[FRIENDS_TABLE],			strstr(settings[9], "=") + 1, sizeof(this->TABLENAME[FRIENDS_TABLE]));
	strncpy(this->TABLENAME[IGNORED_TABLE],			strstr(settings[10], "=") + 1, sizeof(this->TABLENAME[IGNORED_TABLE]));
	strncpy(this->TABLENAME[MESSAGES_TABLE],		strstr(settings[11], "=") + 1, sizeof(this->TABLENAME[MESSAGES_TABLE]));
	strncpy(this->TABLENAME[ABUSEREPORTS_TABLE],	strstr(settings[12], "=") + 1, sizeof(this->TABLENAME[ABUSEREPORTS_TABLE]));
	strncpy(this->TABLENAME[CLANS_TABLE],			strstr(settings[13], "=") + 1, sizeof(this->TABLENAME[CLANS_TABLE]));
	strncpy(this->TABLENAME[CLAN_GB_TABLE],			strstr(settings[14], "=") + 1, sizeof(this->TABLENAME[CLAN_GB_TABLE]));

	return true;
}

bool MySQLDatabase::ConnectDatabase()
{
	//create the connection object
	this->m_Connection = mysql_init(NULL);

	//set the connection options
	mysql_options(this->m_Connection, MYSQL_OPT_RECONNECT, &this->reconnect);
	//mysql_options(this->m_Connection, MYSQL_OPT_BIND, this->bind_interface);
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

	sprintf(SQL, "INSERT INTO %s (email, password, country, realcountry, emailgamerelated, acceptsemail, membersince, ispreorder, numfriendsrecruited, activeprofileid, isbanned) VALUES (?, ?, ?, ?, ?, ?, ?, 0, 0, 0, 0)", TABLENAME[ACCOUNTS_TABLE]);

	MySQLQuery query(this->m_Connection, SQL);
	MYSQL_BIND params[7];
	memset(params, 0, sizeof(params));

	ulong emailLength = strlen(email);
	ulong passLength = strlen(password);
	ulong countryLength = strlen(country);
	ulong realCountryLength = strlen(realcountry);
	time_t local_timestamp = time(NULL);
	uint membersince = local_timestamp;

	query.Bind(&params[0], email, &emailLength);
	query.Bind(&params[1], password, &passLength);
	query.Bind(&params[2], country, &countryLength);
	query.Bind(&params[3], realcountry, &realCountryLength);
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

	ulong emailLength = strlen(email);
	ulong passLength = ARRAYSIZE(password);
	uint id, activeprofileid;
	uchar isbanned;

	query.Bind(&param[0], email, &emailLength);

	query.Bind(&results[0], &id);
	query.Bind(&results[1], password, &passLength);
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

	ulong countryLength = strlen(realcountry);

	query.Bind(&param[0], realcountry, &countryLength);
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
	sprintf(SQL1, "INSERT INTO %s (accountid, name, rank, clanid, rankinclan, isdeleted, commoptions, membersince, lastlogindate, motto, homepage, medaldata, badgedata, lastmatchplayed, scoretotal, scoreasinfantry, highscoreasinfantry, scoreassupport, highscoreassupport, scoreasarmor, highscoreasarmor, scoreasair, highscoreasair, scorebydamagingenemies, scorebyusingtacticalaid, scorebycapturingcommandpoints, scorebyrepairing, scorebyfortifying, scorelostbykillingfriendly, highestscore, timetotalmatchlength, timeplayedasusa, timeplayedasussr, timeplayedasnato, timeplayedasinfantry, timeplayedassupport, timeplayedasarmor, timeplayedasair, numberofmatches, numberofmatcheswon, numberofmatcheslost, numberofassaultmatches, numberofassaultmatcheswon, currentassaultwinstreak, numberofdominationmatches, numberofdominationmatcheswon, currentdominationwinstreak, numberoftugofwarmatches, numberoftugofwarmatcheswon, currenttugofwarwinstreak, currentwinningstreak, bestwinningstreak, numberofmatcheswonbytotaldomination, currentdominationperfectstreak, numberofperfectdefendsinassaultmatch, currentassaultperfectstreak, numberofperfectpushesintugofwarmatch, currenttugofwarperfectstreak, numberofunitskilled, numberofunitslost, numberofcommandpointcaptures, numberofreinforcementpointsspent, numberoftacticalaidpointsspent, numberofnukesdeployed, currentnukesdeployedstreak, numberoftacticalaidcriticalhits, numberoftimesbestplayer, currentbestplayerstreak, numberoftimesbestinfantry, currentbestinfantrystreak, numberoftimesbestsupport, currentbestsupportstreak, numberoftimesbestair, currentbestairstreak, numberoftimesbestarmor, currentbestarmorstreak) VALUES (?, ?, 0, 0, 0, 0, 992, ?, 0, NULL, NULL, ?, ?, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query1(this->m_Connection, SQL1);
	
	// prepared statement binding structures
	MYSQL_BIND params1[5];

	// initialize (zero) bind structures
	memset(params1, 0, sizeof(params1));

	// query specific variables
	ulong nameLength = wcslen(name);
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
		preorderBadge.stars = 0;

		recruitBadge.level = numFriendsRecruited > 0 ? 1 : recruitBadge.level;
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
	query1.Bind(&params1[1], name, &nameLength);
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

bool MySQLDatabase::UpdatePreorderRecruitBadges(const uint accountId, const uint profileId)
{
	MMG_Stats::Badge badges[14];
	uchar isPreorder = 0;
	uint numFriendsRecruited = 0;
	bool changed = false;

	if (!QueryProfileBadges(profileId, 14, badges))
		return false;

	if (!QueryPreorderNumRecruited(accountId, &isPreorder, &numFriendsRecruited))
		return false;

	if (badges[12].level < 1)
	{
		badges[12].level = isPreorder > 0 ? 1 : 0;
		badges[12].stars = 0;
		changed = true;
	}

	if (badges[13].level < 3)
	{
		badges[13].level = numFriendsRecruited > 0 ? 1 : badges[13].level;
		badges[13].level = numFriendsRecruited > 4 ? 2 : badges[13].level;
		badges[13].level = numFriendsRecruited > 9 ? 3 : badges[13].level;
		badges[13].stars = 0;
		changed = true;
	}

	if (changed)
	{
		if (!UpdateProfileBadges(profileId, 14, badges))
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

	if (!UpdatePreorderRecruitBadges(accountId, profileId))
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

bool MySQLDatabase::QueryFriends(const uint profileId, uint *dstProfileCount, uint *friendIds)
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
	sprintf(SQL, "SELECT friendprofileid FROM %s WHERE profileid = ? ORDER BY id ASC LIMIT 100", TABLENAME[FRIENDS_TABLE]);

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
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("profile id(%u), %u friends found", profileId, count);
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
	sprintf(SQL, "SELECT id FROM %s WHERE id <> ? AND lastlogindate > (UNIX_TIMESTAMP(NOW()) - 604800) LIMIT 64", TABLENAME[PROFILES_TABLE]);

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

bool MySQLDatabase::QueryIgnoredProfiles(const uint profileId, uint *dstProfileCount, uint *ignoredIds)
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
	sprintf(SQL, "SELECT ignoredprofileid FROM %s WHERE profileid = ? ORDER BY id ASC LIMIT 64", TABLENAME[IGNORED_TABLE]);

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
		*dstProfileCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("profile id(%u), %u ignored profiles found", profileId, count);
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
			//DatabaseLog("profile id(%u) found", profileId);

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

bool MySQLDatabase::SearchProfileName(const wchar_t* const name, uint *dstCount, uint *profileIds)
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
	sprintf(SQL, "SELECT id FROM %s WHERE isdeleted = 0 AND name LIKE CONCAT('%%',?,'%%') ORDER BY id ASC LIMIT 100", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND param[1], result[1];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(result, 0, sizeof(result));

	// query specific variables
	uint id;
	ulong nameLength = wcslen(name);

	// bind parameters to prepared statement
	query.Bind(&param[0], name, &nameLength);

	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(param, result))
	{
		DatabaseLog("SearchProfileName() failed:");
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

bool MySQLDatabase::SearchClanName(const wchar_t* const name, uint *dstCount, MMG_Clan::Description *clans)
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
	sprintf(SQL, "SELECT id, clanname, clantag FROM %s WHERE clanname LIKE CONCAT('%%',?,'%%') OR shortclanname LIKE CONCAT('%%',?,'%%') ORDER BY id ASC LIMIT 100", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[2], results[3];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	uint id;
	wchar_t clanname[WIC_CLANNAME_MAX_LENGTH];
	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	memset(clanname, 0, sizeof(clanname));
	memset(clantag, 0, sizeof(clantag));

	ulong clannameLength = ARRAYSIZE(clanname);
	ulong clantagLength = ARRAYSIZE(clantag);

	ulong nameLength = wcslen(name);

	// bind parameters to prepared statement
	query.Bind(&param[0], name, &nameLength);
	query.Bind(&param[1], name, &nameLength);
	
	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], clanname, &clannameLength);
	query.Bind(&results[2], clantag, &clantagLength);

	// execute prepared statement
	if(!query.StmtExecute(param, results))
	{
		DatabaseLog("SearchClanName() failed:");
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
	sprintf(SQL, "UPDATE %s SET motto = ?, homepage = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

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
		*dstMessageCount = 0;
	}
	else
	{
		ulong count = (ulong)query.StmtNumRows();
		DatabaseLog("found %u pending messages for profile id(%u)", count, profileId);
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
	sprintf(SQL, "INSERT INTO %s (senderaccountid, senderprofileid, sendername, reportedaccountid, reportedprofileid, reportedname, report, datereported, actiontaken, comments) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 0, NULL)", TABLENAME[ABUSEREPORTS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[8];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	ulong senderNameLength = wcslen(senderProfile.m_Name);
	ulong reportedNameLength = wcslen(flaggedProfile.m_Name);
	ulong reportLength = wcslen(report);

	time_t local_timestamp = time(NULL);
	//struct tm* gtime = gmtime(&local_timestamp);
	//time_t utc_timestamp = mktime(gtime);

	uint datereported = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &senderAccountId);
	query.Bind(&params[1], &senderProfile.m_ProfileId);
	query.Bind(&params[2], senderProfile.m_Name, &senderNameLength);
	query.Bind(&params[3], &flaggedAccountId);
	query.Bind(&params[4], &flaggedProfile.m_ProfileId);

	if (reportedNameLength)
		query.Bind(&params[5], flaggedProfile.m_Name, &reportedNameLength);
	else
		query.BindNull(&params[5]);
	
	query.Bind(&params[6], report, &reportLength);
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
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

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
	ulong msgLength = ARRAYSIZE(message);

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
	query.Bind(&results[5], message, &msgLength);

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
	ulong msgLength = wcslen(entry->m_Message);
	time_t local_timestamp = time(NULL);
	
	entry->m_Timestamp = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &entry->m_Timestamp);
	query.Bind(&params[2], &requestId);
	query.Bind(&params[3], &entry->m_ProfileId);
	query.Bind(&params[4], entry->m_Message, &msgLength);
	
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
	ulong clannameLength = wcslen(clanname);
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], clanname, &clannameLength);
	
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
	ulong clantagLength = wcslen(clantag);
	uint id;

	// bind parameters to prepared statement
	query.Bind(&param[0], clantag, &clantagLength);
	
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

	// build sql query using table names defined in settings file
	sprintf(SQL, "INSERT INTO %s (clanname, clantag, shortclanname, displaytag, playeroftheweekid, motto, motd, homepage) VALUES (?, ?, ?, ?, ?, NULL, NULL, NULL)", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND params1[5];

	// initialize (zero) bind structures
	memset(params1, 0, sizeof(params1));

	// query specific variables
	wchar_t temptags[WIC_CLANTAG_MAX_LENGTH];
	memset(temptags, 0, sizeof(temptags));

	ulong clannameLength = wcslen(clanname);
	ulong clantagLength = wcslen(clantag);
	ulong displayTagLength = strlen(displayTag);

	if (strcmp(displayTag, "[C]P") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"[%ws]", clantag);
	else if (strcmp(displayTag, "P[C]") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"[%ws]", clantag);
	else if (strcmp(displayTag, "C^P") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"%ws^", clantag);
	else if (strcmp(displayTag, "-=C=-P") == 0)
		swprintf(temptags, WIC_CLANTAG_MAX_LENGTH, L"-=%ws=-", clantag);
	else
		assert(false);

	ulong temptagsLength = wcslen(temptags);

	// bind parameters to prepared statement
	query.Bind(&params1[0], clanname, &clannameLength);
	query.Bind(&params1[1], temptags, &temptagsLength);
	query.Bind(&params1[2], clantag, &clantagLength);
	query.Bind(&params1[3], displayTag, &displayTagLength);
	uint aZero = 0; // temporary
	query.Bind(&params1[4], &aZero);
	
	// execute prepared statement
	if (!query.StmtExecute(params1))
	{
		DatabaseLog("Create clan failed.");
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

bool MySQLDatabase::UpdatePlayerClanId(const uint profileId, const uint clanId)
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
	sprintf(SQL, "UPDATE %s SET clanid = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &clanId);
	query.Bind(&params[1], &profileId);

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("UpdatePlayerClanId() failed: profileid(%u)", profileId);
	else
		DatabaseLog("UpdatePlayerClanId() success: profileid(%u)", profileId);

	if (!query.Success())
	{
		this->RollbackTransaction();
		return false;
	}

	// commit the transaction
	this->CommitTransaction();

	return true;
}

bool MySQLDatabase::UpdatePlayerClanRank(const uint profileId, const uchar rankInClan)
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
	sprintf(SQL, "UPDATE %s SET rankinclan = ? WHERE id = ? LIMIT 1", TABLENAME[PROFILES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// bind parameters to prepared statement
	query.Bind(&params[0], &rankInClan);
	query.Bind(&params[1], &profileId);

	// execute prepared statement
	if(!query.StmtExecute(params))
		DatabaseLog("UpdatePlayerClanRank() failed: profileid(%u)", profileId);
	else
		DatabaseLog("UpdatePlayerClanRank() success: profileid(%u)", profileId);

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
	sprintf(SQL, "DELETE FROM %s WHERE (senderprofileid = ? OR recipientprofileid = ?) AND message LIKE ?", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[3];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|%u%%", clanId);

	ulong msgLength = wcslen(message);

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &profileId);
	query.Bind(&params[2], message, &msgLength);

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
	sprintf(SQL, "DELETE FROM %s WHERE (senderprofileid = ? OR recipientprofileid = ?) AND message LIKE ?", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[3];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clms|%%");

	ulong msgLength = wcslen(message);

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);
	query.Bind(&params[1], &profileId);
	query.Bind(&params[2], message, &msgLength);

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
	sprintf(SQL2, "DELETE FROM %s WHERE message LIKE ?", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query2(this->m_Connection, SQL2);

	// prepared statement binding structures
	MYSQL_BIND params2[1];

	// initialize (zero) bind structures
	memset(params2, 0, sizeof(params2));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|%u%%", clanId);

	ulong msgLength = wcslen(message);

	// bind parameters to prepared statement
	query2.Bind(&params2[0], message, &msgLength);

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
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

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

	ulong clannameLength = ARRAYSIZE(clanname);
	ulong clantagLength = ARRAYSIZE(clantag);
	
	uint id, playeroftheweekid;

	wchar_t clanmotto[WIC_MOTTO_MAX_LENGTH];
	wchar_t clanmotd[WIC_MOTD_MAX_LENGTH];
	wchar_t clanhomepage[WIC_HOMEPAGE_MAX_LENGTH];
	memset(clanmotto, 0, sizeof(clanmotto));
	memset(clanmotd, 0, sizeof(clanmotd));
	memset(clanhomepage, 0, sizeof(clanhomepage));
	ulong clanmottoLength = ARRAYSIZE(clanmotto);
	ulong clanmotdLength = ARRAYSIZE(clanmotd);
	ulong clanhomepageLength = ARRAYSIZE(clanhomepage);

	// bind parameters to prepared statement
	query1.Bind(&param1[0], &clanId);

	// bind results
	query1.Bind(&results1[0], &id);
	query1.Bind(&results1[1], clanname, &clannameLength);
	query1.Bind(&results1[2], clantag, &clantagLength);
	query1.Bind(&results1[3], &playeroftheweekid);
	query1.Bind(&results1[4], clanmotto, &clanmottoLength);
	query1.Bind(&results1[5], clanmotd, &clanmotdLength);
	query1.Bind(&results1[6], clanhomepage, &clanhomepageLength);

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
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

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

	ulong clannameLength = ARRAYSIZE(clanname);
	ulong clantagLength = ARRAYSIZE(clantag);

	// bind parameters to prepared statement
	query.Bind(&param[0], &clanId);
	
	// bind results
	query.Bind(&results[0], &id);
	query.Bind(&results[1], clanname, &clannameLength);
	query.Bind(&results[2], clantag, &clantagLength);

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

bool MySQLDatabase::QueryClanTag(const uint clanId, wchar_t *shortclanname, char *displaytag)
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
	sprintf(SQL, "SELECT shortclanname, displaytag FROM %s WHERE id = ? LIMIT 1", TABLENAME[CLANS_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);
	
	// prepared statement binding structures
	MYSQL_BIND param[1], results[2];

	// initialize (zero) bind structures
	memset(param, 0, sizeof(param));
	memset(results, 0, sizeof(results));

	// query specific variables
	wchar_t clantag[WIC_CLANTAG_MAX_LENGTH];
	char disptag[WIC_CLANTAG_MAX_LENGTH];
	memset(clantag, 0, sizeof(clantag));
	memset(disptag, 0, sizeof(disptag));

	ulong clantagLength = ARRAYSIZE(clantag);
	ulong disptagLength = ARRAYSIZE(disptag);

	// bind parameters to prepared statement
	query.Bind(&param[0], &clanId);
	
	// bind results
	query.Bind(&results[0], clantag, &clantagLength);
	query.Bind(&results[1], disptag, &disptagLength);

	// execute prepared statement
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
			//DatabaseLog("");

			shortclanname = L"";
			displaytag = "";
		}
		else
		{
			//DatabaseLog("");
			
			wcsncpy(shortclanname, clantag, WIC_CLANTAG_MAX_LENGTH);
			strncpy(displaytag, disptag, WIC_CLANTAG_MAX_LENGTH);
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

	QueryClanTag(profile->m_ClanId, clantag, disptag);

	wchar_t oldNameBuffer[WIC_NAME_MAX_LENGTH];
	memset(oldNameBuffer, 0, sizeof(oldNameBuffer));

	// store old name in temp location
	wcsncpy(oldNameBuffer, profile->m_Name, WIC_NAME_MAX_LENGTH);
	memset(profile->m_Name, 0, sizeof(profile->m_Name));

	wchar_t fullNameBuffer[WIC_NAME_MAX_LENGTH];
	memset(fullNameBuffer, 0, sizeof(fullNameBuffer));

	// craft new name, using old name
	if (strcmp(disptag, "[C]P") == 0)
		swprintf(fullNameBuffer, WIC_NAME_MAX_LENGTH, L"[%ws]%ws", clantag, oldNameBuffer);
	else if (strcmp(disptag, "P[C]") == 0)
		swprintf(fullNameBuffer, WIC_NAME_MAX_LENGTH, L"%ws[%ws]", oldNameBuffer, clantag);
	else if (strcmp(disptag, "C^P") == 0)
		swprintf(fullNameBuffer, WIC_NAME_MAX_LENGTH, L"%ws^%ws", clantag, oldNameBuffer);
	else if (strcmp(disptag, "-=C=-P") == 0)
		swprintf(fullNameBuffer, WIC_NAME_MAX_LENGTH, L"-=%ws=-%ws", clantag, oldNameBuffer);
	else
		assert(false);

	// copy new name into the profile name
	wcsncpy(profile->m_Name, fullNameBuffer, WIC_NAME_MAX_LENGTH);

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

	// query specific variables
	ulong mottoLength = wcslen(motto);
	ulong motdLength = wcslen(motd);
	ulong homepageLength = wcslen(homepage);

	// bind parameters to prepared statement
	query.Bind(&params[0], &profileId);

	// if either string is empty BindNull
	if (mottoLength)
		query.Bind(&params[1], motto, &mottoLength);
	else
		query.BindNull(&params[1]);

	if (motdLength)
		query.Bind(&params[2], motd, &motdLength);
	else
		query.BindNull(&params[2]);

	if (homepageLength)
		query.Bind(&params[3], homepage, &homepageLength);
	else
		query.BindNull(&params[3]);

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
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	char SQL[4096];
	memset(SQL, 0, sizeof(SQL));

	// build sql query using table names defined in settings file
	sprintf(SQL, "SELECT id FROM %s WHERE recipientprofileid = ? AND message LIKE ? LIMIT 1", TABLENAME[MESSAGES_TABLE]);

	// prepared statement wrapper object
	MySQLQuery query(this->m_Connection, SQL);

	// prepared statement binding structures
	MYSQL_BIND params[2], result[1];

	// initialize (zero) bind structures
	memset(params, 0, sizeof(params));
	memset(result, 0, sizeof(result));

	// query specific variables
	wchar_t message[WIC_INSTANTMSG_MAX_LENGTH];
	memset(message, 0, sizeof(message));

	swprintf(message, WIC_INSTANTMSG_MAX_LENGTH, L"|clan|%%");	// in SQL % is wildcard used with LIKE, use %% to escape

	ulong msgLength = wcslen(message);
	uint id;

	// bind parameters to prepared statement
	query.Bind(&params[0], &inviteeProfileId);
	query.Bind(&params[1], message, &msgLength);
	
	// bind results
	query.Bind(&result[0], &id);

	// execute prepared statement
	if(!query.StmtExecute(params, result))
	{
		DatabaseLog("CheckIfInvitedAlready() query failed.");
		*dstId = 0;
	}
	else
	{
		if (!query.StmtFetch())
		{
			//DatabaseLog("");
			*dstId = 0;
		}
		else
		{
			//DatabaseLog("");
			*dstId = id;
		}
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
	ulong msgLength = ARRAYSIZE(message);

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
	query.Bind(&results[5], message, &msgLength);

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
	ulong msgLength = wcslen(entry->m_Message);
	time_t local_timestamp = time(NULL);
	
	entry->m_Timestamp = local_timestamp;

	// bind parameters to prepared statement
	query.Bind(&params1[0], &clanId);
	query.Bind(&params1[1], &entry->m_Timestamp);
	query.Bind(&params1[2], &requestId);
	query.Bind(&params1[3], &entry->m_ProfileId);
	query.Bind(&params1[4], entry->m_Message, &msgLength);
	
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
	query.Bind(&results[1], &scoreTotal);
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
			playerstats->m_ScoreTotal = scoreTotal - scoreLost;
		}
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
		DatabaseLog("VerifyServerKey() query failed:");
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

bool MySQLDatabase::SavePlayerMatchStats(const uint datematchplayed, MMG_Stats::PlayerMatchStats *playerstats)
{
	// test the connection before proceeding, disconnects everyone on fail
	if (!this->TestDatabase())
	{
		this->EmergencyMassgateDisconnect();
		return false;
	}

	BeginTransaction();

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
				 "numberoftimesbestarmor  = IF (? > 0, numberoftimesbestarmor + 1, numberoftimesbestarmor), "
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

	if (!query.StmtExecute(params))
		DatabaseLog("SavePlayerMatchStats() query failed:");

	if (!query.Success())
	{
		RollbackTransaction();
		return false;
	}

	CommitTransaction();

	return true;
}